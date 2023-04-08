#include "Hologram.h"

#include <stdio.h>
#include <math.h>

using glm::vec3 ;
using glm::mat4;
using glm::mat3;
using glm::vec4;
using glm::vec2;

    
// position of upper left corner of image, how pixel coordinates convert to 3D space and normal of image plane (typically facing toward camera)
Hologram::Hologram(glm::vec3 view_point, float view_radius){
    vantage = view_point ;
    vantage_radius = view_radius;
}

// Adds a new panel for determining depth of view rays
void Hologram::addPanel(const HologramPanel& p){
    panel.push_back(p);
}

// Constructs and adds a new panel
void Hologram::addPanel(glm::vec3 zero, glm::vec3 xperpixel, glm::vec3 yperpixel , glm::vec3 z_normal, std::vector<std::vector<float>>& depth, int block_size){
    panel.emplace_back(zero, xperpixel, yperpixel, z_normal);
    panel[panel.size()-1].setDepth(depth);
    panel[panel.size()-1].buildBlockImage(block_size);
}

// Adds a new image view which will be reprojected to produce output colors
void Hologram::addView(const HologramView& v){
    view.push_back(v);
}

// Constructs and adds a new view
// Note: image_data will be moved and passed in Variant will be destroyed
void Hologram::addView(glm::vec3 p, glm::vec3 n , const std::vector<std::pair<glm::vec3, glm::vec2>> &v_to_pixel, Variant& image_data, int w, int h){
    view.emplace_back(p,n,v_to_pixel);
    view[view.size()-1].moveImage(image_data, w, h);
}

// get the point a ray would hit by intersecting it with the appropriate panel
glm::vec3 Hologram::getPoint(const glm::vec3 &p, const glm::vec3 &v){
    int best_panel = 0 ;
    float best_score = panel[0].scoreAlignment(p,v);
    for(int k=1;k<panel.size();k++){
        float score = panel[k].scoreAlignment(p,v);
        if(score < best_score){ 
            best_score = score ;
            best_panel = k ;
        }
    }
    float t = panel[best_panel].rayTrace(p,v);
    if(t >=0){
        return p + v*t ;
    }else{
        return vec3(0,0,0);
    }
}

// get the color of a ray
glm::vec3 Hologram::getColor(const glm::vec3 &p, const glm::vec3 &v){
    vec3 intersect = getPoint(p,v);
    int best_view = 0 ;
    float best_score = view[0].scoreAlignment(p, intersect);
    for(int k=1;k<view.size();k++){
        float score = view[k].scoreAlignment(p, intersect);
        if(score < best_score){ 
            best_score = score ;
            best_view= k ;
        }
    }
    return view[best_view].getColor(intersect);
}