#include "PartitioningRadianceField.h"

#include <stdio.h>
#include <math.h>

using glm::vec3 ;
using std::vector ;

PartitioningRadianceField::PartitioningRadianceField(){

}

PartitioningRadianceField::PartitioningRadianceField(int depth){
    field.resize(pow(2,depth)-1);
    for(int k=0;k<field.size();k++){
        initializeNode(k);
    }

}

void PartitioningRadianceField::initializeNode(int k){
    auto& node = field[k] ;
    node.locked = false;
    node.sharp = false;
    node.leaf = firstChild(k) >= field.size() ;

    node.value = (float)(k%2); // set bottom layer to hard 0/1 splits

    node.N1.x = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
    node.N1.y = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
    node.N1.z = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
    node.d1 = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;

    node.N2.x = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
    node.N2.y = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
    node.N2.z = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
    node.d2 = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
}

// functions for navigating the tree
int PartitioningRadianceField::firstChild(int n){
    return 2*n + 1;
}
int PartitioningRadianceField::secondChild(int n){
    return 2*n + 2 ;
}
int PartitioningRadianceField::parent(int n){
    return (n-1)/ 2 ;
}

float PartitioningRadianceField::sigmoid(float x, float h){
    double ehx = exp(h*(double)x);
    if(isnan(ehx)){
        if(x > 0){
            return 1.0f;
        }else{
            return 0.0f ;
        }
    }
    float result = ehx / (ehx+1.0) ;
    if(isnan(result)){
        return 1.0f;
    }
    return result ;
}

float PartitioningRadianceField::sigmoidprime(float x, float h){
    double ehx = exp(h*(double)x);
    if(isnan(ehx)){
        return 0.0f ;
    }
    double ehxo = ehx+1.0f;
    float result = h * ehx / (ehxo*ehxo) ;
    if(isnan(result)){
        return 0 ;
    }
    return result;
}

// Returns the value for a given ray starting from the root
double PartitioningRadianceField::computeValue(vec3 &p, vec3 &v){
    return computeValue(0,p,v);
}

// Returns the value for a ray starting from the given node
double PartitioningRadianceField::computeValue(int n, vec3 &p, vec3 &v){
    Partition &node = field[n];
    if(node.leaf){
        return node.value ;
    }else{
        float N1p = glm::dot(node.N1, p);
        float N1v = glm::dot(node.N1, v);
        float N2p = glm::dot(node.N2, p);
        float N2v = glm::dot(node.N2, v);
        float s = N2v * node.d1 - N1v * node.d2 + (N2v + offset) * N1p - (N1v + offset) * N2p ;
        if(isnan(s)){
            printf("s is nan!\n");
        }
        float r = sigmoid(s, node.h);
        if(isnan(r)){
            printf("r is nan!\n");
        }
        if(node.sharp){
            if(r <= 0.5){
                return computeValue(firstChild(n), p, v);
            }else{
                return computeValue(secondChild(n), p, v);
            }
        }else{
            return r * computeValue(firstChild(n), p, v) + (1.0f-r) * computeValue(secondChild(n), p, v) ;
        }
    }
}

// gets the squared error of a given data point
float PartitioningRadianceField::getError(vec3 &p, vec3 &v, float y){
    float value = computeValue(p,v) ;
    return (value-y)*(value-y) ;
}

// adds the gradient from a given data point to an accumulating gradient
// p,v is the ray mapping to value y
void PartitioningRadianceField::accumulateGradient(std::vector<float> &gradient, vec3 &p, vec3 &v, float y){
        float ry = computeValue(p, v);
        // The error direction term from the outermost squared error uses the same mult as the scaling term recursing
        accumulateGradient(0, gradient, p,v, 2.0f * (ry-y) );
}

// accumulates the gradient for a given node and training point
// p,v is the ray mapping to value y, mult is the multiplier passed from the parent node
float PartitioningRadianceField::accumulateGradient(int n, std::vector<float> &gradient, vec3 &p, vec3 &v, float mult){

    Partition &node = field[n];
    if(node.leaf){
        if(!node.locked){
            gradient[n*8] += mult ;
        }
        return node.value ;
    }else{
        float N1p = glm::dot(node.N1, p);
        float N1v = glm::dot(node.N1, v);
        float N2p = glm::dot(node.N2, p);
        float N2v = glm::dot(node.N2, v);
        float s = N2v * node.d1 - N1v * node.d2 + (N2v + offset) * N1p - (N1v + offset) * N2p ;
        float r = sigmoid(s, node.h);
        if(node.sharp){
            if(r <= 0.5){
                return accumulateGradient(firstChild(n), gradient, p, v, mult);
            }else{
                return accumulateGradient(secondChild(n), gradient, p, v, mult);
            }
        }else{
            
            
            float a = accumulateGradient(firstChild(n), gradient, p, v, mult*r) ;
            float b = accumulateGradient(secondChild(n), gradient, p, v, mult*(1.0f-r));

            if(!node.locked){
                /*
                node.N1.x = x[8*k] ;
                node.N1.y = x[8*k+1] ;
                node.N1.z = x[8*k+2] ;
                node.d1 = x[8*k+3] ;
                node.N2.x = x[8*k+4] ;
                node.N2.y = x[8*k+5] ;
                node.N2.z = x[8*k+6] ;
                node.d2 = x[8*k+7] ;
                */
                mult *= (a-b) * sigmoidprime(s, node.h);
                //N1
                gradient[8*n] += mult * ( (-N2p-node.d2) * v.x + (N2v + offset) * p.x );
                gradient[8*n+1] += mult * ( (-N2p-node.d2) * v.y + (N2v + offset) * p.y );
                gradient[8*n+2] += mult * ( (-N2p-node.d2) * v.z + (N2v + offset) * p.z );
                //d1
                gradient[8*n+3] += mult * N2v ;

                //N2
                gradient[8*n+4] += mult * ( (node.d1+N1p) * v.x - (N1v + offset) * p.x );
                gradient[8*n+5] += mult * ( (node.d1+N1p) * v.y - (N1v + offset) * p.y );
                gradient[8*n+6] += mult * ( (node.d1+N1p) * v.z - (N1v + offset) * p.z );
                //d2
                gradient[8*n+7] += mult * -N1v ;

            }
            return r * a + (1.0f-r) * b ;
        }
    }

}


    // Return the current x for this object
std::vector<float> PartitioningRadianceField::getX(){
    vector<float> x;
    x.reserve(8*field.size());
    for(int k=0;k<field.size();k++){
        Partition& node = field[k] ;
        if(node.locked){
            x.push_back(0.0f);
            x.push_back(0.0f);
            x.push_back(0.0f);
            x.push_back(0.0f);
            x.push_back(0.0f);
            x.push_back(0.0f);
            x.push_back(0.0f);
            x.push_back(0.0f);
        }else if(node.leaf){
            x.push_back(node.value) ;
            // pad out to 8 floats to match branches
            x.push_back(0.0f);
            x.push_back(0.0f);
            x.push_back(0.0f);
            x.push_back(0.0f);
            x.push_back(0.0f);
            x.push_back(0.0f);
            x.push_back(0.0f);
        }else{
            x.push_back(node.N1.x) ;
            x.push_back(node.N1.y) ;
            x.push_back(node.N1.z) ;
            x.push_back(node.d1) ;
            x.push_back(node.N2.x) ;
            x.push_back(node.N2.y) ;
            x.push_back(node.N2.z) ;
            x.push_back(node.d2) ;
        }
    }
    return x ;
}

// Set this object to a given x
void PartitioningRadianceField::setX(std::vector<float> x){
    for(int k=0;k<field.size();k++){
        Partition& node = field[k] ;
        if(node.locked){
        }else if(node.leaf){
            node.value = x[8*k] ;
        }else{
            node.N1.x = x[8*k] ;
            node.N1.y = x[8*k+1] ;
            node.N1.z = x[8*k+2] ;
            node.d1 = x[8*k+3] ;
            node.N2.x = x[8*k+4] ;
            node.N2.y = x[8*k+5] ;
            node.N2.z = x[8*k+6] ;
            node.d2 = x[8*k+7] ;
        }
    }
}

// Adds a training ray to the data set tha twill be used for computing the OptimizationProblem error and gradient
void PartitioningRadianceField::addTrainingRay(vec3 &p, vec3 &v, float y){
    training_set.push_back({p,v,y});
}

// Returns the error to be minimized for the given input
double PartitioningRadianceField::error(std::vector<float> x){
    setX(x);
    float error = 0 ;
    for(auto& d : training_set){
        error += getError(d.p,d.v, d.y);
    }
    return error ;
}

// Returns the gradient of error about a given input
std::vector<float> PartitioningRadianceField::gradient(std::vector<float> x){

    std::vector<float> gradient ;
    gradient.resize(x.size());
    for(int k=0;k<x.size();k++){
        gradient[k] = 0.0 ;
    }

    setX(x);

    for(auto& d : training_set){
        accumulateGradient(gradient, d.p,d.v, d.y);
    }
    /*
    std::vector<float> ng = numericalGradient(x, 0.001);
    for(int k=0;k<ng.size();k++){
        printf("%d : %f == %f \n", k, gradient[k], ng[k]);
    }*/

    return gradient ;
    
}


void PartitioningRadianceField::lockRow(int row){
    for(int n= pow(2, row) - 1; n < pow(2, row+1) -1; n++){
        field[n].locked = true;
        field[n].sharp = true;
    }
}

void PartitioningRadianceField::unlockRow(int row){
    for(int n= pow(2, row) - 1; n < pow(2, row+1) -1; n++){
        field[n].locked = false;
        field[n].sharp = false;
    }
}
void PartitioningRadianceField::setHomotopy(int row, float h){
    for(int n = pow(2, row) - 1; n < pow(2, row+1) -1; n++){
        field[n].h = h;
    }
}

int PartitioningRadianceField::numRows(){
    return (int)log2(field.size()) +1;
}

void PartitioningRadianceField::addRow(){
    printf("Field size 1 : %d\n", (int)field.size());
    int depth = numRows()+1;
    field.resize(pow(2,depth)-1);
    printf("Field size 2 : %d\n", (int)field.size());
    int reset_start = pow(2, depth-2) - 1 ; // reset leaves and intialize all new nodes
    printf("reset start : %d\n", (int)reset_start);
    for(int k=field.size() -1;k>=reset_start;k--){
        initializeNode(k);
        if(field[k].leaf){
            field[k].value = field[parent(k)].value ;
        }
    }

}

void PartitioningRadianceField::setLeavestoTrainingAverage(double strength){
    vector<std::pair<float, int>> total;
    total.resize(field.size(), std::pair<float, int>(0,0));
    for(auto& d : training_set){
        int n = 0 ;
        
        while(!field[n].leaf){
            Partition& node = field[n];
            float N1p = glm::dot(node.N1, d.p);
            float N1v = glm::dot(node.N1, d.v);
            float N2p = glm::dot(node.N2, d.p);
            float N2v = glm::dot(node.N2, d.v);
            float s = N2v * node.d1 - N1v * node.d2 + (N2v + offset) * N1p - (N1v + offset) * N2p ;
            if(isnan(s)){
                printf("2 S is nan\n");
            }
            float r = sigmoid(s, node.h);
            if(isnan(r)){
                printf("2 r isnan\n");
            }
            if(r <= 0.5){
                n = firstChild(n);
            }else{
                n = secondChild(n);
            }
        }
        total[n].first += d.y ;
        total[n].second ++;

    }

    for(int n=0;n<total.size();n++){
        if(field[n].leaf){
            double new_value = total[n].first/total[n].second ;
            if(new_value  > 0.5){
                field[n].value  = strength  + (1.0f-strength)*field[n].value;
            }else{
                field[n].value = (1.0f-strength)*field[n].value;
            }
            //field[n].locked = true;
            //printf("%d : t = %f , amount = %d, result = %f\n", n, total[n].first, total[n].second, field[n].value);
        }
    }

}