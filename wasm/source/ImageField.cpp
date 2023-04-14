#include "ImageField.h"

#include <stdio.h>
#include <math.h>

using glm::dvec3 ;
using std::vector ;

ImageField::ImageField(){

}

ImageField::ImageField(int depth){
    field.resize(pow(2,depth)-1);
    for(int k=0;k<field.size();k++){
        initializeNode(k);
    }

}

void ImageField::initializeNode(int k){
    auto& node = field[k] ;
    node.leaf = firstChild(k) >= field.size() ;
    node.N.x = (rand() % 2000000) / 1000000.0 - 1.0 ;
    node.N.y = (rand() % 2000000) / 1000000.0 - 1.0 ;
    node.N = glm::normalize(node.N);
    
    node.d = (rand() % 2000000) / 1000000.0 - 1.0 ;

    node.N *= 10;
    node.d *= 10;
    node.color = glm::dvec3(random(),random(),random());
}

void ImageField::initializeNode(int k, const std::vector<TrainingPixel>& data){
    auto& node = field[k] ;
    node.leaf = firstChild(k) >= field.size() ;
    if(node.leaf){
        if(data.size() > 0){
            glm::dvec3 total ;
            for(auto& [x, y] : data){
                total += y ;
            }
            node.color = total/(double)data.size();
        }else{
            node.color = glm::dvec3(0.5f,0.5f,0.5f);
        }
    }else{
        
        if(k == 0){
            node.N.x = 100.0f ;
            node.N.y = 0.0f ;
        }else{
            auto& p = field[parent(k)];
            node.N.x = p.N.y;
            node.N.y = -p.N.x;
        }
        /*
        node.N.x = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
        node.N.y = (float) ((rand() % 2000000) / 1000000.0 - 1.0) ;
        node.N = glm::normalize(node.N);
        node.N *= 100.0 ;
        */
        double dt = 0 ;
        for(auto& [x, y]  : data){
            dt+= glm::dot(node.N, x);
        }
        node.d = -dt/data.size();

        // Split data and run on the children first
        std::vector<TrainingPixel> first_set ;
        std::vector<TrainingPixel> second_set ;
        for(auto& [x, y]  : data){
            double p = sigmoid(glm::dot(x,node.N) + node.d);
            if(p > 0.5){
                first_set.push_back({x,y});
            }else{
                second_set.push_back({x,y});
            }
        }
        //printf("%d: %d - > %d : %d, %d:%d\n", n, (int)data.size(), firstChild(n), (int)first_set.size(),secondChild(n), (int)second_set.size());
        initializeNode(firstChild(k), first_set);
        first_set = std::vector<TrainingPixel>(); // deallocate immediately
        initializeNode(secondChild(k), second_set);
        second_set = std::vector<TrainingPixel>(); // deallocate immediately
    }
}

// functions for navigating the tree
int ImageField::firstChild(int n){
    return 2*n + 1;
}
int ImageField::secondChild(int n){
    return 2*n + 2 ;
}
int ImageField::parent(int n){
    return (n-1)/ 2 ;
}

float ImageField::random(){
    return (float) ((rand() % 10000000) / 10000000.0) ;
}

double ImageField::sigmoid(double x){
    double ehx = exp((double)x);
    //float ehx = fastexp(x);
    
    if(isnan(ehx)){
        if(x > 0){
            return 1.0f;
        }else{
            return 0.0f ;
        }
    }
    
    double result = ehx / (ehx+1.0) ;
    if(isnan(result)){
        return 1.0f;
    }
    return result ;
}

double ImageField::sigmoidprime(double x){
    double ehx = exp((double)x);
    //float  ehx = fastexp(x);
    
    if(isnan(ehx)){
        return 0.0f ;
    }
    
    double ehxo = ehx+1.0f;
    double result = ehx / (ehxo*ehxo) ;
    if(isnan(result)){
        return 0 ;
    }
    return result;
}

int ImageField::numRows(){
    return (int)log2(field.size()) +1;
}

void ImageField::addRow(){
    printf("Field size 1 : %d\n", (int)field.size());
    int depth = numRows()+1;
    field.resize(pow(2,depth)-1);
    printf("Field size 2 : %d\n", (int)field.size());
    int reset_start = pow(2, depth-2) - 1 ; // reset leaves and intialize all new nodes
    printf("reset start : %d\n", (int)reset_start);
    for(int k=field.size() -1;k>=reset_start;k--){
        initializeNode(k);
        if(field[k].leaf){
            field[k].color = field[parent(k)].color;
        }
    }

}

// Returns the value for a given ray starting from the root
glm::dvec3 ImageField::value(const glm::dvec2 &x){
    return value(0, x);
}
    

// Returns the value for a ray starting from the given node
glm::dvec3 ImageField::value(int n, const glm::dvec2 &x){
    Node& node = field[n];
    if(node.leaf){
        return node.color ;
    }
    double p = sigmoid(glm::dot(x,node.N) + node.d);
    if(random() < p){
        return value(firstChild(n), x);
    }else{
        return value(secondChild(n), x);
    }
}

// Returns the value for a given ray starting from the root
glm::dvec3 ImageField::color(const glm::dvec2 &x){
    return color(0, x);
}
    

// Returns the value for a ray starting from the given node
glm::dvec3 ImageField::color(int n, const glm::dvec2 &x){
    Node& node = field[n];
    if(node.leaf){
        return node.color ;
    }
    double p = sigmoid(glm::dot(x,node.N) + node.d);
    if(0.4 + random()* 0.2 < p){
        return color(firstChild(n), x);
    }else{
        return color(secondChild(n), x);
    }
}

// Adds a training ray to the data set tha twill be used for computing the OptimizationProblem error and gradient
void ImageField::addTrainingPixel(const glm::dvec2 &x, const glm::dvec3 &y){
    training_set.push_back({x,y});
}

// Run a training step on the given node after running it on its children
void ImageField::train(int n, const std::vector<TrainingPixel>& data) {
    Node& node = field[n];
    if(node.leaf && data.size() > 0){
        glm::dvec3 total ;
        for(auto& [x, y] : data){
            total += y ;
        }
        node.color = total/(double)data.size();
    }else if(!node.leaf){
        // Split data and run on the children first
        std::vector<TrainingPixel> first_set ;
        std::vector<TrainingPixel> second_set ;
        for(auto& [x, y]  : data){
            double p = sigmoid(glm::dot(x,node.N) + node.d);
            if(random() < p){
                first_set.push_back({x,y});
            }else{
                second_set.push_back({x,y});
            }
        }
        //printf("%d: %d - > %d : %d, %d:%d\n", n, (int)data.size(), firstChild(n), (int)first_set.size(),secondChild(n), (int)second_set.size());
        train(firstChild(n), first_set);
        first_set = std::vector<TrainingPixel>(); // deallocate immediately
        train(secondChild(n), second_set);
        second_set = std::vector<TrainingPixel>(); // deallocate immediately

        // Determine which side of the branch plane you'd like each data point ot be on
        vector<TrainingChance> s_train ;
        for(auto& [x, y]  : data){
            //double po = sigmoid(glm::dot(x,node.N) + node.d);
            glm::dvec3 a = value(firstChild(n), x) - y;
            double ae = glm::dot(a,a);
            glm::dvec3 b = value(secondChild(n), x) - y;
            double be = glm::dot(b,b);
            double w ;
            double p ;
            if(ae < be){ // should go ot first child
                w = 2.0 * be/(ae+be) - 1.0; // 0 when equal error, 1 if ae is 0
                p = 1.0;
                //w*= po ;
            }else{ // should go to second child
                w = 2.0 * ae/(ae+be) - 1.0; // 0 when equal error, 1 if be is 0
                p = 0.0;
                //w*=(1.0-po) ;
            }
            if(w > 0.001){
                s_train.push_back({x, p, w});
            }
        }

        //adjust plane to match desired probabilities
        glm::dvec3 Nd0 = glm::dvec3(node.N.x, node.N.y,node.d);
        
        
        double e0 = error(s_train, Nd0);
        if(e0>0.0001f){
            glm::dvec3 g = gradient(s_train, Nd0);
            //float step = node.step*3.5f;
            double step = 0.2f/sqrt(glm::dot(g,g));
            glm::dvec3 Nd = Nd0 - step*g ;
            int iter = 15 ;
            double e2 = error(s_train, Nd) ;
            while(e2 > e0 && iter-- > 0){
                step *= 0.5;
                Nd = Nd0 - step*g ;
                e2 = error(s_train, Nd) ;
            }
            if(e2 < e0){
                node.N = glm::dvec2(Nd.x, Nd.y);
                node.d = Nd.z ;
            }
            node.step = step ;

            //printf (" e0 = %f for ND0 = (%f, %f, %f) \n e = %f for ND = (%f, %f, %f)\n", e0, Nd0.x, Nd0.y, Nd0.z, error(s_train, Nd), Nd.x, Nd.y, Nd.z);
        }
        
    }
}

// Run a training cycle on all nodes from leaves to root
void ImageField::train(int samples, int sample_size){
    double error_0 = 0 ;
    for(auto& [x, y]  : training_set){
        glm::dvec3 v = value(x) ;
        error_0 += glm::dot(v-y,v-y);
    }

    vector<TrainingPixel> set ;
    for(int k=0;k<samples;k++){
        set.clear();
        for(int j=0; j < sample_size;j++){
            set.push_back(training_set[rand()%training_set.size()]);
        }
        train(0,set);
    }

    double error_1 = 0 ;
    for(auto& [x, y]  : training_set){
        glm::dvec3 v = value(x) ;
        error_1 += glm::dot(v-y,v-y);
    }

    printf("Trained: %f - > %f\n", error_0, error_1);
}

// Run a training cycle on all nodes from leaves to root
void ImageField::train(){
    double error_0 = 0 ;
    for(auto& [x, y]  : training_set){
        glm::dvec3 v = value(x) ;
        error_0 += glm::dot(v-y,v-y);
    }

    train(0,training_set);

    double error_1 = 0 ;
    for(auto& [x, y]  : training_set){
        glm::dvec3 v = value(x) ;
        error_1 += glm::dot(v-y,v-y);
    }

    printf("Trained: %f - > %f\n", error_0, error_1);
}

glm::dvec2 ImageField::normalizeImagePosition(const int x, const int y, const int size){
    return glm::dvec2(x*2.0/size -1.0, y*2.0/size -1.0);
}

 // error for moving planes with probability points
double ImageField::error(std::vector<TrainingChance>& data, glm::dvec3 Nd){
    double error = 0 ;
    for(auto& [x, p , w] : data){
        glm::dvec3 x3 = glm::dvec3(x.x, x.y,1.0f) ;
        double value = sigmoid(glm::dot(Nd, x3)) ;
        //printf("value : %f  p: %f  weight: %f  error delta: %f \n", value, p, w, w*(value - p)*(value-p));
        error += w*(value - p)*(value-p) ;
    }
    return error ;
}

// N is first two components d is third component
glm::dvec3 ImageField::gradient(std::vector<TrainingChance>& data, glm::dvec3 Nd){
    glm::dvec3 gradient = glm::dvec3(0,0,0) ;
    for(auto& [x, p , w] : data){
        glm::dvec3 x3 = glm::dvec3(x.x, x.y,1.0f) ;
        double dot = glm::dot(Nd, x3) ;
        gradient += x3 * ( 2.0 * w * (sigmoid(dot)-p) * sigmoidprime(dot) ) ;
    }

    /*
    float e = error(data,Nd);
    glm::dvec3 ng = glm::dvec3(error(data, Nd + vec3(0.01,0,0)) - e, 
    error(data, Nd + vec3(0,0.01,0)) - e, 
    error(data, Nd + vec3(0,0,0.01)) - e) ;
    ng*=100.0f;

    printf("dx : %f == %f, dy = %f == %f, dz = %f == %f\n", gradient.x, ng.x, gradient.y, ng.y, gradient.z, ng.z);
    */
    return gradient ;
}

/*
void ImageField::addToCluster(Cluster &c, TrainingPixel &p){
            c.total.x += p.x;
            c.total.y += p.y;
            c.data.push_back(p);
        }

float ImageField::clusterDistance(Cluster &c, TrainingPixel &p){
    float m = 1.0f/c.data.size() ;
    return (c.total.x.x*m - p.x.x)*(c.total.x.x*m - p.x.x) +
        (c.total.x.y*m - p.x.y)*(c.total.x.y*m - p.x.y) +
        (c.total.y.x*m - p.y.x)*(c.total.y.x*m - p.y.x) +
        (c.total.y.y*m - p.y.y)*(c.total.y.y*m - p.y.y) +
        (c.total.y.z*m - p.y.z)*(c.total.y.z*m - p.y.z);
}
*/


/* fast floating point exp function
 * must initialize table with buildexptable before using
 * i = ay + b
 * a = 2^(mantissa bits) / ln(2)   ~ 12102203
 * b = (exponent bias) * 2^(mantissa bits) ~ 1065353216
 */

float ImageField::fastexp(const float x) const{
	const int temp = (int)(12102203 * x + 1065353216) ;
	return (*(const float*)(&temp))*expadjust[(temp>>15)&0xff] ;
}

//build correction table to improve result in region of interest
//if region of interest is large enough then improves result everywhere
void ImageField::buildexptable(double min, double max, double step){
	int amount[256] ;
    for(int k=0;k<256;k++){
        amount[k] = 0 ;
        expadjust[k] = 0.0f ;
    }
	//calculate what adjustments should have been for values in region
	for(double x=min; x < max;x+=step){
		double rexp = exp(x);
		int temp = (int)(12102203 * x + 1065353216) ;
		int index = (temp>>15)&0xff ;
	    double fexp = *(float*)(&temp);
		expadjust[index]+= rexp/fexp ;
		amount[index]++;
	}
	//average them out to get adjustment table
	for(int k=0;k<256;k++){
		expadjust[k]/=amount[k];
	}
}

