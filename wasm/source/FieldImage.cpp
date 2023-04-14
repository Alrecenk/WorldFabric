#include "FieldImage.h"

#include <stdio.h>
#include <math.h>

using glm::vec3 ;
using glm::mat4;
using glm::mat3;
using glm::vec4;
using glm::vec2;

// position and normal of image location
// matrix mapping 3D rays to pixel locations is built using 4 examples
FieldImage::FieldImage(glm::vec3 p, glm::vec3 n, const std::vector<std::pair<glm::vec3, glm::vec2>> &v_to_pixel){
    position = p ;
    normal = n ;

    mat3 vset ;
    vec3 xset;
    vec3 yset ;
    for(int k=0;k<3;k++){
        vec3 v = v_to_pixel[k].first;
        vec2 x = v_to_pixel[k].second;
        v = v / glm::dot(v,normal);// project to plane of image if not
        //printf("k : %d V: ( %f, %f, %f) - > X (%f, %f)\n", k, v.x, v.y, v.z, x.x, x.y);

        vset[k][0] = v.x;
        vset[k][1] = v.y;
        vset[k][2] = v.z;

        xset[k] = x.x;
        yset[k] = x.y;
    }
    /*
    printf("vset:\n");
    for(int k=0;k<3;k++){
        printf("[%f %f %f]\n", vset[k][0], vset[k][1], vset[k][2]);
    }
    printf("x :   y:\n");
    for(int k=0;k<3;k++){
        printf("[%f]   [%f]\n", xset[k], yset[k]);
    }
    */

    mat3 iv  = glm::transpose(glm::inverse(vset));

    /*
    printf("iv:\n");
    for(int k=0;k<3;k++){
        printf("[%f %f %f]\n", iv[k][0], iv[k][1], iv[k][2]);
    }
    */

    vec3 x_row = iv*xset;
    vec3 y_row = iv*yset;
    to_pixel[0] = x_row;
    to_pixel[1] = y_row ;

    /*
    printf("to_pixel:\n");
    printf("[%f %f %f]\n", to_pixel[0][0], to_pixel[0][1], to_pixel[0][2]);
    printf("[%f %f %f]\n", to_pixel[1][0], to_pixel[1][1], to_pixel[1][2]);
    */
    /*
    for(int k=0;k<4;k++){
        vec3 v = v_to_pixel[k].first;
        vec2 x = v_to_pixel[k].second;
        v = v / glm::dot(v,normal);// project to plane of image if not

        //vec2 nx = to_pixel * v ;
        vec2 nx = v * to_pixel ;
        printf(" %d to pixel  x: %f - > %f  y: %f - > %f \n", k, x.x, nx.x, x.y, nx.y) ;
    }
    */

}

// Moves the image data into this field (destroys the variant passed in)
void FieldImage::moveImage(Variant& image_data, Variant& point_data, int w, int h, int c){
    width = w ;
    height = h ;
    channels = c ;
    texture = std::move(image_data);
    point_texture = std::move(point_data);
}

// Returns distance between origin of image and closest point on a ray
float FieldImage::rayDistance(const glm::vec3 &p, const glm::vec3 &v){
    return pointLineDistance(p, v, position);
}

// get the color of a ray
glm::vec3 FieldImage::getColor(const glm::vec3 &p, const glm::vec3 &v){
    vec2 tex = getTextureCoordinates(v);
    int tx = (int)tex.x ;
    int ty = (int)tex.y ;
    if(tx>= 6 && ty >= 6 && tx < width-6 && ty < height-6){

        // walk along the neighboring surface to find a point cvloser to intersecting with the ray
        
        int iter = 10 ;
        int btx = tx;
        int bty = ty ;
        float best_dist = pointLineDistance(p, v, getPoint(tx,ty));
        int step = 5 ;
        for(int dx = -3; dx <= 3; dx++){
            for(int dy = -3; dy <= 3; dy++){
                float dist = pointLineDistance(p, v, getPoint(tx+dx*step,ty+dy*step));
                if(dist < best_dist){
                    btx = tx+dx*step;
                    bty = ty+dy*step;
                    best_dist = dist ;
                }
            }
        }
        tx = btx;
        ty = bty;
        
        
        while(iter > 0){
            bool any = false;
            float dist = pointLineDistance(p, v, getPoint(tx+1,ty));
            if(dist < best_dist){
                tx = tx+1;
                best_dist = dist ;
                any = true;
            }
            dist = pointLineDistance(p, v, getPoint(tx-1,ty));
            if(dist < best_dist){
                tx = tx-1;
                best_dist = dist ;
                any = true;
            }
            dist = pointLineDistance(p, v, getPoint(tx,ty+1));
            if(dist < best_dist){
                ty = ty+1;
                best_dist = dist ;
                any = true;
            }
            dist = pointLineDistance(p, v, getPoint(tx,ty-1));
            if(dist < best_dist){
                ty = ty-1;
                best_dist = dist ;
                any = true;
            }
            if(any){
                iter--;
            }else{
                iter = 0 ;
            }
        }
        


        return getColor(tx,ty);
    }else{
        return glm::vec3(0, 0, 0);
    }
}

glm::vec2 FieldImage::getTextureCoordinates(const glm::vec3 &v){
    vec3 iv = v / glm::dot(v,normal); // project to plane of image 
    return iv * to_pixel ;
}

glm::vec3 FieldImage::getPoint(int tx, int ty){
    int i = (ty * width + tx)*3;
    float* image = point_texture.getFloatArray();
    return glm::vec3(image[i], image[i+1], image[i+2]);
}

glm::vec3 FieldImage::getColor(int tx, int ty){
    int i = (ty * width + tx)*channels;
    byte* image = texture.getByteArray();
    return glm::vec3(image[i]/255.0f, image[i+1]/255.0f, image[i+2]/255.0f);
}

float FieldImage::pointLineDistance(const glm::vec3 &p, const glm::vec3 &v, const glm::vec3 &x){
    vec3 diff = v * (glm::dot(x-p,v)/glm::dot(v,v)) + p - x;
    return glm::dot(diff,diff);
}