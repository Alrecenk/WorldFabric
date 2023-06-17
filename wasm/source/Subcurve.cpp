#include "Subcurve.h"

using std::map;
using glm::vec3;
using glm::vec2;
using std::vector;


Subcurve::Subcurve(int poly_degree){
    degree = poly_degree ;
}


void Subcurve::addTarget(float x, float y, float time){
    Target t ;
    t.x = x ;
    t.y = y ;
    t.time= time ;
    targets.push_back(t);
}

// Return the current x for this object
std::vector<float> Subcurve::getX(){
    vector<float> x ;
    for(int k=0;k<=degree;k++){
        x.push_back(poly[k].x);
        x.push_back(poly[k].y);
    }
    return x ;
}

// Set this object to a given x
void Subcurve::setX(std::vector<float> x){
    int j = 0 ;
    for(int k=0;k<=degree;k++){
        poly[k].x = x[j++];
        poly[k].y = x[j++];
    }
}

glm::vec2 Subcurve::valueNormalized(float t){
    float tp = 1.0;
    vec2 f(0.0,0.0);
    for(int k=0;k<=degree;k++){
        f += poly[k]*tp ;
        tp*=t;
    }
    return f ;
}

glm::vec2 Subcurve::value(float time){
    return valueNormalized((time-t0)/(t1-t0)) ;
}


// Returns the error to be minimized for the given input
double Subcurve::error(std::vector<float> x){
    setX(x);
    float error = 0 ;
    for(int k=0;k<targets.size();k++){
        Target target = targets[k];
        vec2 input(target.x,target.y);
        vec2 output = valueNormalized(target.t);
        vec2 diff = output-input ;
        error+=glm::dot(diff,diff) * target.weight ;
    }
    return error ; 
}

// Returns the gradient of error about a given input
std::vector<float> Subcurve::gradient(std::vector<float> x){
    vector<float> dx ;
    for(int k=0;k<2+degree*2;k++){
        dx.push_back(0);
    };
    
    for(int k=0;k<targets.size();k++){
        Target target = targets[k];
        vec2 input(target.x,target.y);
        float t = target.t;
        vec2 output = valueNormalized(t);
        vec2 diff = (output-input) * 2.0f * target.weight ;

        float tp = 1.0;
        int j = 0 ;
        for(int k=0;k<=degree;k++){
            dx[j++] += diff.x*tp;
            dx[j++] += diff.y*tp;
            tp*=t;
        }
    }

    return dx ;
}


float Subcurve::bestT(glm::vec2 target, float min_t, float max_t, int steps, int iter){
    // break into "steps" pieces and select closest
    float best_t = max_t ;// do end as default since we may not hit it in iteration
    vec2 diff = target-valueNormalized(best_t) ; 
    float best_dist = glm::dot(diff, diff);
    float step = (max_t-min_t)/steps;
    if(step < 0.0001f){
        return (min_t + max_t)*0.5f;
    }
    for(float t = min_t ; t <= max_t; t+= step){
        diff = target-valueNormalized(t) ;
        float dist = glm::dot(diff,diff);
        if(dist < best_dist){
            best_dist = dist ;
            best_t = t ;
        }
    }

    // binary search for a few iterations to get closer
    for(int i=0;i<iter;i++){
        step*=0.5f ;
        float t1 = fmin(best_t + step, max_t) ;
        float t2 = fmax(best_t - step, min_t) ;
        diff = target-valueNormalized(t1) ;
        float dist = glm::dot(diff,diff);
        if(dist < best_dist){
            best_dist = dist ;
            best_t = t1 ;
        }
        diff = target-valueNormalized(t2) ;
        dist = glm::dot(diff,diff);
        if(dist < best_dist){
            best_dist = dist ;
            best_t = t2 ;
        }
    }

    return best_t ;

}

// Uses targets to solve for the curve
void Subcurve::solve(){
    if(targets.size() <= degree){
        return ;
    }
    // normalize times for better numerical behavior
    t0 = std::numeric_limits<float>::max() ;
    t1 = -std::numeric_limits<float>::max() ;
    for(int k=0;k<targets.size();k++){
        t0 = fmin(t0,targets[k].time);
        t1 = fmax(t1,targets[k].time);
    }
    for(int k=0;k<targets.size();k++){
        targets[k].t = (targets[k].time - t0)/(t1-t0);
        //printf("t[%d] = %f << %f \n", k, targets[k].t, targets[k].time );
        targets[k].weight = 1.0;
    }
    // End points are stickier
    targets[0].weight = 10;
    targets[targets.size()-1].weight = 10;

    vector<float> x ;
    for(int k=0;k<2+degree*2;k++){
        x.push_back(0);
    };
    //printf("Starting error: %f\n", error(x));

/*
    vector<float> dg = gradient(x);
    vector<float> ndg = numericalGradient(x,0.01);
    
    for(int k=0;k<8;k++){
        printf("f'(0)[%d] : %f == %f\n", k, dg[k], ndg[k]);
    }*/


    x = OptimizationProblem::minimizeByLBFGS(x, 5, 20, 20, 0.0001, 0.0001);
    setX(x);

    //printf("Error after first curve: %f\n", error(x));

    /*
    dg = gradient(x);
    ndg = numericalGradient(x,0.01);

    for(int k=0;k<=degree;k++){
        printf("coef[%d] : %f, %f \n", k, poly[k].x, poly[k].y);
    }
    for(int k=0;k<8;k++){
        printf("f'(x)[%d] : %f == %f\n", k, dg[k], ndg[k]);
    }
    */

    float max_gap = fmax(0.1f, 2.0/targets.size()) ;
    for(int i=0;i<15;i++){
        for(int k=1;k<targets.size()-1;k++){
            float min_t = fmax(targets[k-1].t, targets[k+1].t-max_gap) ;
            float max_t = fmin(targets[k+1].t,targets[k-1].t+max_gap)  ;
            targets[k].t = bestT(vec2(targets[k].x,targets[k].y), min_t,max_t, 20, 5);
        }
        x = OptimizationProblem::minimizeByLBFGS(x, 5, 20, 20, 0.0001, 0.0001);
        setX(x);
    }

    //printf("Final error: %f\n", error(x));
}