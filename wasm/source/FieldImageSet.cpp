#include "FieldImageSet.h"

#include <stdio.h>
#include <math.h>

using glm::vec3 ;

FieldImageSet::FieldImageSet(){

}

void FieldImageSet::addImage(FieldImage& image){
    set.push_back(image); // TODO move images instead of copying
}

glm::vec3 FieldImageSet::getColor(const glm::vec3 &p, const glm::vec3 &v){
    FieldImage* best_image = nullptr;
    float best_dist = 9999.0f;
    for(FieldImage& image : set){
        float dist = image.rayDistance(p,v);
        if(dist < best_dist){
            best_dist = dist ;
            best_image = &image ;
        }
    }
    if(best_image != nullptr){
        return best_image->getColor(p,v);
    }
    return vec3(0,0,0);
}