#include "HologramView.h"

#include <stdio.h>
#include <math.h>

using glm::vec3 ;
using glm::mat4;
using glm::mat3;
using glm::vec4;
using glm::vec2;

// position and normal of image location
// matrix mapping 3D rays to pixel locations is built using 4 examples
HologramView::HologramView(glm::vec3 p, glm::vec3 n, const std::vector<std::pair<glm::vec3, glm::vec2>> &v_to_pixel){
    position = p ;
    normal = n ;

    mat3 vset ;
    vec3 xset;
    vec3 yset ;
    for(int k=0;k<3;k++){
        vec3 v = v_to_pixel[k].first;
        vec2 x = v_to_pixel[k].second;
        v = v / glm::dot(v,normal);// project to plane of image if not

        vset[k][0] = v.x;
        vset[k][1] = v.y;
        vset[k][2] = v.z;

        xset[k] = x.x;
        yset[k] = x.y;
    }
    mat3 iv  = glm::transpose(glm::inverse(vset));
    vec3 x_row = iv*xset;
    vec3 y_row = iv*yset;
    to_pixel[0] = x_row;
    to_pixel[1] = y_row ;
}

// Moves the image data into this field (destroys the variant passed in)
void HologramView::moveImage(Variant& image_data, int w, int h, int c){
    width = w ;
    height = h ;
    channels = c ;
    texture = std::move(image_data);
}

// get the color of a point this image can see
glm::vec3 HologramView::getColor(const glm::vec3 &p){
    vec3 v = p - position ;
    vec2 tex = getTextureCoordinates(v);
    int tx = (int)tex.x ;
    int ty = (int)tex.y ;
    //printf("View getColor v: %f, %f, %f  uv: %d, %d\n", v.x,v.y,v.z,tx,ty);
    if(tx>= 0 && ty >= 0 && tx < width && ty < height){
        return getColor(tx,ty);
    }else{
        return glm::vec3(0, 0, 0);
    }
}

// return texturecoordinates given a direction vector from the image's viewpoint
glm::vec2 HologramView::getTextureCoordinates(const glm::vec3 &v){
    vec3 iv = v / glm::dot(v,normal); // project to plane of image 
    return iv * to_pixel ;
}

glm::vec3 HologramView::getColor(int tx, int ty){
    int i = (ty * width + tx)*channels;
    byte* image = texture.getByteArray();
    return glm::vec3(image[i+red_channel]/255.0f, image[i+green_channel]/255.0f, image[i+blue_channel]/255.0f);
}


// sets depths and depth map by k-means clustering
//Also adjusts zero position so depths efficiently cover the whole range
void HologramView::setDepth(std::vector<std::vector<float>>& depth){
    float min_depth = std::numeric_limits<float>::max() ;
    float max_depth = -std::numeric_limits<float>::max() ;
    for(int x=0;x<width;x++){
        for(int y=0;y<height;y++){
            float d = depth[x][y];
            if(d >= 0){
                min_depth = fmin(min_depth, d);
            }
            max_depth = fmax(max_depth, d);
        }
    }
    
    // adjust zero point so min_depth is at Z = 0

    // linearly map the depth values over the range
    depth_map.reserve(256);
    for(int k=0;k<256;k++){
        depth_map.push_back(min_depth + (k-1)*(max_depth-min_depth)/254); // [1] needs to be 0 
    }
    // map depths to indices into new depth array
    byte* texture_bytes = texture.getByteArray();
    if(channels!=4){
        printf("Hologram view should have 4 channels, aborting depth set!\n"); 
        return ; 
    }
    for(int x=0;x<width;x++){
        for(int y=0;y<height;y++){
            float d = depth[x][y] ;
            int di = 0 ;
            if(d >= 0){
                di = 1 + (int)((d-min_depth)*254/(max_depth-min_depth)) ;
                //printf("d:%f, di: %d\n", d, di);
                di = std::min(255,std::max(1,di));
            }
            texture_bytes[channels * ( width * y + x) + depth_channel] = di ;
        }
    }
}

// return true if the given point is visible by this image
bool HologramView::checkVisibility(const glm::vec3 &p){
    vec3 v = p - position ;
    float depth = glm::dot(v,normal) ;
    if(depth < 0){ // behind camera
        return false;
    }
    vec2 tex = getTextureCoordinates(v);
    int tx = (int)tex.x ;
    int ty = (int)tex.y ;
    if(tx <= 0 || ty <= 0 || tx >= width || ty >= height){ // not in frame taken
        return false;
    }
    int i = (ty * width + tx)*channels  ;
    byte* image = texture.getByteArray();
    int di = image[i+depth_channel] ;
    if(di == 0 ){ // transparent pixel
        return true; // is visible cause we saw all the way through
    }
    float image_depth = depth_map[di];
    
    return depth <= image_depth*1.015f && depth >= image_depth*0.5f; // occlusion check

}

//scores how well this image aligns for a ray starting at p0 and hitting a solid at intersect (lower is better)
float HologramView::scoreAlignment(const glm::vec3& p0, const glm::vec3& intersect){
    vec3 v0 = p0-intersect;
    vec3 vi = position-intersect ;
    // minimize angle between ray to user view and ray to this image 
    return -glm::dot(v0,vi)/ sqrt(glm::dot(v0,v0) * glm::dot(vi,vi));
}

float HologramView::blendWeight(const glm::vec3& p0, const glm::vec3& intersect){
    vec3 v0 = p0-intersect;
    vec3 vi = position-intersect ;
    bool visible = checkVisibility(intersect);
    if(!visible){
        return 0 ;
    }
    // minimize angle between ray to user view and ray to this image 
    float cos = glm::dot(v0,vi)/ sqrt(glm::dot(v0,v0) * glm::dot(vi,vi));
    if(cos > 0){
        return 1/((1-cos+0.001f)*(1-cos+0.001f));
    }else{
        return 0 ; 
    }
}
