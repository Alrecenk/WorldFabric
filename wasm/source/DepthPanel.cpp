#include "DepthPanel.h"

#include <stdio.h>
#include <math.h>

using glm::vec3 ;
using glm::mat4;
using glm::mat3;
using glm::vec4;
using glm::vec2;
using std::vector;

// position of upper left corner of image, how pixel coordinates convert to 3D space and normal of image plane (typically facing toward camera)
DepthPanel::DepthPanel(glm::vec3 zero, glm::vec3 xperpixel, glm::vec3 yperpixel , glm::vec3 z_normal){
    zero_position = zero ;
    X = xperpixel;
    Y = yperpixel;
    Z = z_normal;
    /*
    printf( "zero: %f, %f, %f\n", zero.x, zero.y, zero.z);
    printf( "X: %f, %f, %f\n", X.x, X.y, X.z);
    printf( "Y: %f, %f, %f\n", Y.x, Y.y, Y.z);
    printf( "Z: %f, %f, %f\n", Z.x, Z.y, Z.z);
    */
}

// Moves the image data into this field (destroys the variant passed in)
// w,h defines dimension of image data bytes
// The image bytes should be packed with the number of channels and arranged as defined in const expressions in DephPanel.h
// depth values are indices into a depth_map 
//(depth=0 is transparent, depth[1]= 0 increasing to depth[255] = max depth /closest to expected view position)
void DepthPanel::moveImage(Variant& image_data, int w, int h, std::vector<float> depths){
    width = w ;
    height = h ;
    texture = std::move(image_data);
    depth_map = depths ;
}


// returns the position where the given ray first intersects the bounding box of this panel
// or returns p if p is inside the panel already
// returns 0,0,0 if the ray does not intersect the box TODO remove magic number
glm::vec3 DepthPanel::getFirstPointInBox(const glm::vec3 &p, const glm::vec3 &v){
   
    // Get t value intersection with the 6 sides of the bounding box
    glm:vec3 N = X ;
    float d = - glm::dot(N, zero_position);
    float t1 = rayPlaneIntersect(p,v,N,d);
    d = - glm::dot(N, zero_position + X*(float)width);
    float t2 = rayPlaneIntersect(p,v,N,d);
    float xmint = fmin(t1,t2);
    float xmaxt = fmax(t1,t2);

    N = Y ;
    d = - glm::dot(N, zero_position);
    t1 = rayPlaneIntersect(p,v,N,d);
    d = - glm::dot(N, zero_position + Y*(float)height);
    t2 = rayPlaneIntersect(p,v,N,d);
    float ymint = fmin(t1,t2);
    float ymaxt = fmax(t1,t2);

    N = Z ;
    d = - glm::dot(N, zero_position);
    t1 = rayPlaneIntersect(p,v,N,d);
    d = - glm::dot(N, zero_position + Z*depth_map[255]);
    t2 = rayPlaneIntersect(p,v,N,d);
    float zmint = fmin(t1,t2);
    float zmaxt = fmax(t1,t2);

    // get range of t intersection
    float mint = fmax(fmax(xmint,ymint),zmint) ;
    float maxt = fmin(fmin(xmaxt,ymaxt),zmaxt) ;
    /*
    printf("p: %f, %f, %f, v %f,%f,%f\n", p.x,p.y,p.z,v.x,v.y,v.z); 
    printf("Xmint:%f, Xmaxt:%f\n", xmint, xmaxt);
    printf("Ymint:%f, Ymaxt:%f\n", ymint, ymaxt);
    printf("Zmint:%f, Zmaxt:%f\n", zmint, zmaxt);
    printf("mint:%f, maxt:%f\n", mint, maxt);
    */
    if(maxt < mint || maxt < 0){ // doesn't hit box
        return vec3(0,0,0); 
    }
    float t = fmax(mint,0) ; // if started inside, then use that
    return p + v * t; // return enter p

}

// Returns the x,y coordinates of the first pixel column the given ray hits.
// returns -1,-1 if there is no hit
// This function steps 1 pixel at a time, so you'll want to use other methods to step the ray closer to its intersection point before calling this
std::pair<int,int> DepthPanel::firstPixelHit(const glm::vec3 &p, const glm::vec3 &v){


    float first_x = glm::dot(p-zero_position, X)/glm::dot(X,X);
    float first_y = glm::dot(p-zero_position, Y)/glm::dot(Y,Y);
    float first_z = glm::dot(p-zero_position, Z)/glm::dot(Z,Z);

    //return std::pair<int,int>((int)first_x,(int)first_y);
    

    // get t value on sides of pixel column by intersecting ray with planes of pixel boundary
    float left_t = rayPlaneIntersect(p,v, X, - glm::dot(X, zero_position + X*floor(first_x)));
    float right_t = rayPlaneIntersect(p,v, X, - glm::dot(X, zero_position + X*(floor(first_x)+1)));
    float top_t = rayPlaneIntersect(p,v, Y, - glm::dot(Y, zero_position + Y*floor(first_y)));
    float bottom_t = rayPlaneIntersect(p,v, Y, - glm::dot(Y, zero_position + Y*(floor(first_y)+1)));

    // how ray t changes with movement in texture coordinates
    float tperx = right_t-left_t ;
    float tpery = bottom_t-top_t ;
    float zpert = glm::dot(v, Z)/glm::dot(Z,Z) ;

    int xstep  ;
    float nextx ;
    if(right_t > left_t){
        xstep = 1 ;
        nextx = right_t ;
    }else{
        xstep = -1;
        nextx = left_t ;
    }

    int ystep  ;
    float nexty ;
    if(bottom_t > top_t){
        ystep = 1 ;
        nexty = bottom_t ;
    }else{
        ystep = -1;
        nexty = top_t ;
    }

    // TODO there's a bunch of rounding wierdness here, so the result isn't perfect
    int x = (int)first_x ;
    int y = (int)first_y ;
    float t = 0 ;
    float z = first_z ;
    byte* image = texture.getByteArray();

    

    // loop until z is in range or past the edge
    float max_z = depth_map[255]+0.001f ;
    int depth = image[(y * width + x)*channels + depth_channel] ;
    while(z < max_z && x >=0 && x < width && y >=0 && y < height && z > depth_map[depth] && (depth > 0 || z >0)){
        // step horizontally or vertically based on which you would hit next
        float next_t = nextx < nexty ? nextx : nexty ;
        z += zpert*(next_t-t) ;
        if(z <= depth_map[depth]){ // if we would hit the bar before exiting
            break ; // exit without stepping
        }
        t = next_t ;
        if(nextx < nexty){
            nextx += tperx*xstep;
            x += xstep;
        }else{
            nexty += tpery*ystep;
            y += ystep;
        }
        depth = image[(y * width + x)*channels + depth_channel] ;
    }

    /*
    printf("x0:%f y0:%f  z0: %f \n", first_x, first_y, first_z); 
    printf("tperx: %f, tpery: %f, zpert: %f\n", tperx, tpery, zpert); 
    printf("xyf:%d,%d  z: %f  \n", x,y, z); 
    printf("t: %f, max_z:%f , depth: %d, depth value :%f\n", t,  max_z, depth, depth_map[image[(y * width + x)*channels + depth_channel]]); 
    */

    if(z < max_z && x >=0 && x < width && y >=0 && y < height && depth > 0){
        
        return std::pair<int,int>(x,y);
    }else{
        return std::pair<int,int>(-1,-1);
    }


    
}

// get the color of a ray
glm::vec3 DepthPanel::getColor(const glm::vec3 &p, const glm::vec3 &v){
    vec3 enter_p = getFirstPointInBox(p,v);
    if(enter_p.x == 0.0f && enter_p.y == 0.0f && enter_p.z == 0.0f){
        return enter_p;
    }
    //return vec3(1,1,1);
    
    std::pair<int,int> uv = firstPixelHit(enter_p,v);
    
    if(uv.first <0 || uv.second < 0){
        return vec3(0,0,0);
    }
    return getColor(uv.first, uv.second);
    
}

// given coordinates into the texture, return the 3D point of the surface
glm::vec3 DepthPanel::getPoint(int tx, int ty){
    int i = (ty * width + tx)*channels;
    byte* image = texture.getByteArray();
    float depth = depth_map[image[i+depth_channel]] ;
    return zero_position + X*(float)tx + Y*(float)ty + Z*depth ;
}

// given coordinates into the texture, return a 0 to 1, rgb color vector
glm::vec3 DepthPanel::getColor(int tx, int ty){
    int i = (ty * width + tx)*channels;
    byte* image = texture.getByteArray();
    return glm::vec3(image[i+red_channel]/255.0f, image[i+green_channel]/255.0f, image[i+blue_channel]/255.0f);
}

DepthPanel DepthPanel::generateTestPanel(glm::vec3 center, float radius, glm::vec3 normal){
   
    int width = 512, height = 512;
    float border=0.1;
    
    // Make a basis where panel Y roughly matches real world Y
    vec3 Y(-0.002,1,0.01);
    vec3 X = glm::cross(normal, Y);
    Y = glm::cross(normal, X);
    vec3 Z = glm::normalize(normal);
    // X and Y are per pixel, so, we want the image to cover a radius witha border
    X = glm::normalize(X) * (float)(radius*2*(1+border)/width);
    Y = glm::normalize(Y) * (float)(radius*2*(1+border)/height);
 
    glm::vec3 zero = center - X*(float)(width/2) - Y*(float)(height/2) ;
    //printf("d\n");
    DepthPanel result (zero, X, Y , Z);
    //printf("e\n");
    
    
    Variant texture ;
    texture.makeFillableByteArray(width*height*channels);
    byte* image_bytes = texture.getByteArray();
    //printf("f\n");
    for(int x = 0 ; x < width; x++){
        for(int y = 0 ; y < height; y++){
            vec3 p = zero + X*(float)x +Y*(float)y ; // get 3D point on backplate
            vec3 rv = p-center;
            float r = sqrt(glm::dot(rv,rv));
            image_bytes[channels * ( width * y + x) + red_channel] = (byte)(255*x/width) ;
            image_bytes[channels * ( width * y + x) + green_channel] = (byte)(255*y/height) ;
            image_bytes[channels * ( width * y + x) + blue_channel] = (byte)(0) ;
            image_bytes[channels * ( width * y + x) + depth_channel] = r < radius ? (255-253*r/radius) : 0 ;
            if(r > radius){
                image_bytes[channels * ( width * y + x) + blue_channel] = (byte)(255) ;
            }
        }
    }
    //printf("g\n");

    vector<float> depth_map;
    depth_map.reserve(256);
    for(int k=0;k<256;k++){
        depth_map.push_back((k-1)*0.003f); // [1] needs to be 0 
    }
    result.moveImage(texture, width,height, depth_map);
    //printf("h\n");
    return result ;
}

// returns t value where ray (p + v*t) intersects a plane (N*x + d = 0)
float DepthPanel::rayPlaneIntersect(const glm::vec3 &p, const glm::vec3 &v, const glm::vec3 &N, const float d){
    return (-d - glm::dot(N,p)) / glm::dot(N,v) ;
}
