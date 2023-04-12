#include "Hologram.h"

#include <stdio.h>
#include <math.h>
#include <queue>

using glm::vec3 ;
using glm::mat4;
using glm::mat3;
using glm::vec4;
using glm::vec2;
using std::string;
using std::map;
using std::vector;

Hologram::Hologram(){
    vantage = vec3(0,0,0);
    vantage_radius = 1;
}

// position of upper left corner of image, how pixel coordinates convert to 3D space and normal of image plane (typically facing toward camera)
Hologram::Hologram(glm::vec3 view_point, float view_radius){
    vantage = view_point ;
    vantage_radius = view_radius;
}

// Adds a new panel for determining depth of view rays
void Hologram::addPanel(const HologramPanel& p){
    panel.push_back(p);
    printf("Panel added!\n");
}

// Constructs and adds a new panel
void Hologram::addPanel(glm::vec3 zero, glm::vec3 xperpixel, glm::vec3 yperpixel , glm::vec3 z_normal, std::vector<std::vector<float>>& depth, int block_size){
    panel.emplace_back(zero, xperpixel, yperpixel, z_normal);
    panel[panel.size()-1].setDepth(depth);
    panel[panel.size()-1].buildBlockImage(block_size);
    printf("Panel added!\n");
}

// Adds a new image view which will be reprojected to produce output colors
void Hologram::addView(const HologramView& v){
    view.push_back(v);
    printf("View added!\n");
}

// Constructs and adds a new view
// Note: image_data will be moved and passed in Variant will be destroyed
void Hologram::addView(glm::vec3 p, glm::vec3 n , const std::vector<std::pair<glm::vec3, glm::vec2>> &v_to_pixel, Variant& image_data, int w, int h, int c){
    view.emplace_back(p,n,v_to_pixel);
    view[view.size()-1].moveImage(image_data, w, h, c);
    printf("View added!\n");
}

// get the point a ray would hit by intersecting it with the appropriate panel
glm::vec3 Hologram::getPoint(const glm::vec3 &p, const glm::vec3 &v){
    std::priority_queue<std::pair<float, int>> best_panels ;
    int max_panel_blend = 3 ;
    for(int k=0;k<panel.size();k++){
        float score = panel[k].scoreAlignment(p,v);
        best_panels.push(std::pair<float, int>(score,k)) ; // we wan the worst at the top so we pop off the worst
        if(best_panels.size()>max_panel_blend){
            best_panels.pop();
        }
    }

    vec3 total_point = vec3(0,0,0) ;
    float total_weight=0;
    // weight of best hi and best miss 
    float best_miss = 0 ;
    float best_hit = 0 ;
    while(!best_panels.empty()){
        std::pair<float, int> bp = best_panels.top() ;
        best_panels.pop();
        // 1 is optimal alignment, 0 is perpendicular
        float start_blend = 0.5f;
        float alignment = -bp.first ;
        if(alignment > start_blend){ // pretty close to start blending

            float weight = (alignment-start_blend)*(alignment-start_blend) ;
            vec3 point = panel[bp.second].rayTrace(p,v) ;
            // don't blend a miss with a hit
            if(point.x == 0.0f && point.y == 0.0f && point.z == 0.0f){
                best_miss = fmax(weight,best_miss) ;
            }else{
                best_hit = fmax(weight,best_hit);
                total_point += point * weight;
                total_weight += weight;
            }
        }
    }

    // if we got no valid panels or betsscoring panel was a miss
    if(total_weight <= 0.0f || best_miss > best_hit){
        return vec3(0.0f,0.0f,0.0f); // then it was a miss
    }else{
        return total_point/total_weight; // blend hits
    }

}

// get the color of a ray
glm::vec3 Hologram::getColor(const glm::vec3 &p, const glm::vec3 &v){
    vec3 intersect = getPoint(p,v);
    if(intersect.x == 0.0f && intersect.y == 0.0f && intersect.z == 0.0f){
        return intersect ;
    }

    std::priority_queue<std::pair<float, int>> best_views ;
    int max_view_blend = 3 ;
    for(int k=0;k<view.size();k++){
        float w = view[k].blendWeight(p, intersect);
        if(w > 0){
            //if(print)printf("panel check %d score: %f\n", k, score);
            best_views.push(std::pair<float, int>(-w,k)) ; // we want the worst at the top so we pop off the worst
            if(best_views.size()>max_view_blend){
                best_views.pop();
            }
        }
    }

    vec3 total_color= vec3(0,0,0) ;
    float total_weight=0;
    while(!best_views.empty()){
        
        std::pair<float, int> bv = best_views.top() ;
        best_views.pop();
        float weight = -bv.first ;
        vec3 color = view[bv.second].getColor(intersect);
        total_color += color * weight;
        total_weight += weight;
            
    }

    if(total_weight <= 0.0f){
        return vec3(0.0f,0.0f,0.0f); // then it was a miss
    }else{
        return total_color/total_weight; // blend hits
    }
    
}

// Serialize this object
Variant Hologram::serialize(){
    map<string,Variant> serial;
    serial["vantage"] = Variant(vantage);
    serial["radius"] = Variant(vantage_radius);
    
    vector<Variant> panels;
    for(int k=0;k<panel.size();k++){
        panels.push_back(panel[k].serialize());
    }
    serial["panels"] = Variant(panels);
    
    vector<Variant> views;
    for(int k=0;k<view.size();k++){
        views.push_back(view[k].serialize());
    }

    serial["views"] = Variant(views);
    
    //Variant serialized = Variant(serial) ;
    //serialized.printFormatted();
    
    //return serialized;
    return Variant(serial) ;
    
}

// Set this object to data generated by its serialize method
void Hologram::set(Variant& serialized){
    //serialized.printFormatted();
    map<string,Variant> serial = serialized.getObject();
    vantage = serial["vantage"].getVec3();
    vantage_radius = serial["radius"].getFloat();
    
    vector<Variant> panels = serial["panels"].getVariantArray();

    panel = vector<HologramPanel>();
    for(int k=0;k<panels.size();k++){
        panel.emplace_back(panels[k]);
    }
    
    vector<Variant> views = serial["views"].getVariantArray();
    view = vector<HologramView>();
    for(int k=0;k<views.size();k++){
        view.emplace_back(views[k]);
    }
    
    
}