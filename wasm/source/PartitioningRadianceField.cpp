#include "PartitioningRadianceField.h"

#include <stdio.h>

using glm::vec3 ;
using std::vector ;

PartitioningRadianceField::PartitioningRadianceField(int depth){
    field.resize(pow(2,depth)-1);
    for(int k=0;k<field.size();k++){
        auto& node = field[k] ;
        node.locked = false;
        node.sharp = false;
        if(firstChild(k) >= field.size()){
            node.leaf = true;
            node.value = (float)(k%2); // set bottom layer to hard 0/1 splits
        }else{
            node.leaf = false;
            node.N1.x = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
            node.N1.y = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
            node.N1.z = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
            node.d1 = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;

            node.N2.x = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
            node.N2.y = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
            node.N2.z = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
            node.d2 = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
        }
    }

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
    float ehx = exp(h*x);
    return ehx / (ehx+1.0f);
}

float PartitioningRadianceField::sigmoidprime(float x, float h){
    float ehx = exp(h*x);
    float ehxo = ehx+1.0f;
    return h * ehx / (ehxo*ehxo);
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
        float s = N2v * node.d1 - N1v * node.d2 + (N2v + 1.0f) * N1p - (N1v+1.0f) * N2p ;
        float r = sigmoid(s, node.h);
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
        accumulateGradient(0, gradient, p,v, 2 * (ry-y) );
}

// accumulates the gradient for a given node and training point
// p,v is the ray mapping to value y, mult is the multiplier passed from the parent node
void PartitioningRadianceField::accumulateGradient(int n, std::vector<float> &gradient, vec3 &p, vec3 &v, float mult){
    printf("Gradient not implemented yet!\n");


    Partition &node = field[n];
    if(node.leaf){
        if(!node.locked){
            gradient[n*8] += mult ;
        }
    }else{
        float N1p = glm::dot(node.N1, p);
        float N1v = glm::dot(node.N1, v);
        float N2p = glm::dot(node.N2, p);
        float N2v = glm::dot(node.N2, v);
        float s = N2v * node.d1 - N1v * node.d2 + (N2v + 1.0f) * N1p - (N1v+1.0f) * N2p ;
        float r = sigmoid(s, node.h);
        if(node.sharp){
            if(r <= 0.5){
                return accumulateGradient(firstChild(n), gradient, p, v, mult);
            }else{
                return accumulateGradient(secondChild(n), gradient, p, v, mult);
            }
        }else{
            if(!node.locked){
                //TODO
            }
            
            accumulateGradient(firstChild(n), gradient, p, v, mult*r) ;
            accumulateGradient(secondChild(n), gradient, p, v, mult*(1.0f-r));
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

    return gradient ;
}