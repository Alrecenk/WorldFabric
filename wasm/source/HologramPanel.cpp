#include "HologramPanel.h"

#include <stdio.h>
#include <math.h>

using glm::vec3 ;
using glm::mat4;
using glm::mat3;
using glm::vec4;
using glm::vec2;
using std::vector;

// position of upper left corner of image, how pixel coordinates convert to 3D space and normal of image plane (typically facing toward camera)
HologramPanel::HologramPanel(glm::vec3 zero, glm::vec3 xperpixel, glm::vec3 yperpixel , glm::vec3 z_normal){
    zero_position = zero ;
    X = xperpixel;
    Y = yperpixel;
    Z = z_normal;
}

// sets depths and depth map to cover a depth image given as floats of distances (toward camera) from image plane
// Also adjusts zero position so the minimum depth is at 0
// Depth with negative value are set as transparent
void HologramPanel::setDepth(vector<vector<float>>& depth){
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
    zero_position = zero_position + Z*min_depth ;
    max_depth -= min_depth ;
    // linearly map the depth values over the range
    depth_map.reserve(256);
    for(int k=0;k<256;k++){
        depth_map.push_back((k-1)*max_depth/254); // [1] needs to be 0 
    }
    // map depths to indices into new depth array
    byte* texture_bytes = depth_texture.getByteArray();
    for(int x=0;x<width;x++){
        for(int y=0;y<height;y++){
            depth[x][y] -= min_depth;
            float d = depth[x][y] ;
            int di = 0 ;
            if(d >= 0){
                di = 1 + (int)(d*254/max_depth) ;
                //printf("d:%f, di: %d\n", d, di);
                di = std::min(255,std::max(1,di));
            }
            texture_bytes[ width * y + x] = di ;
        }
    }
}


// returns the position where the given ray first intersects the bounding box of this panel
// or returns p if p is inside the panel already
// returns 0,0,0 if the ray does not intersect the box TODO remove magic number
glm::vec3 HologramPanel::getFirstPointInBox(const glm::vec3 &p, const glm::vec3 &v){
   
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

    if(maxt < mint || maxt < 0){ // doesn't hit box
        return vec3(0,0,0); 
    }
    float t = fmax(mint,0) ; // if started inside, then use that
    return p + v * t; // return enter p

}


// builds the blockdepth image for use with getFirstBlockHit
void HologramPanel::buildBlockImage(int size){
    block_size = size ;
    bwidth = width/size + 1;
    bheight = height/size + 1;
    block_depth.makeFillableByteArray(bwidth*bheight);
    byte* block_bytes = block_depth.getByteArray();
    for(int bx = 0; bx < bwidth; bx++){
        for(int by = 0; by < bheight;by++){
            block_bytes[bx+by*width] = 0 ;
        }
    }
    // each block ha the max of all pixels in it, so it will hit if anything in it would
    byte* texture_bytes = depth_texture.getByteArray();
    for(int x=0;x<width;x++){
        for(int y=0;y<height;y++){
            byte d = texture_bytes[width * y + x] ;
            int bx = x/block_size;
            int by = y/block_size;
            block_bytes[bx + by*bwidth] = std::max(block_bytes[bx + by*bwidth], d);
            //printf("b:%d, %d  d: %d, max: %d\n",bx,by,d, block_bytes[bx + by*bwidth]);
        }
    }

}

// returns the position where the given ray first intersects a block
// or returns p if p is inside a block already
// returns 0,0,0 if the ray does not intersect the box TODO remove magic number
glm::vec3 HologramPanel::getFirstPointInBlock(const glm::vec3 &p, const glm::vec3 &v){

    float first_x = glm::dot(p-zero_position, X)/glm::dot(X,X);
    float first_y = glm::dot(p-zero_position, Y)/glm::dot(Y,Y);
    float first_z = glm::dot(p-zero_position, Z)/glm::dot(Z,Z);

    // get t value on sides of pixel column by intersecting ray with planes of pixel boundary
    float left_t = rayPlaneIntersect(p,v, X, - glm::dot(X, zero_position + X*(floor(first_x/block_size)*block_size)));
    float right_t = rayPlaneIntersect(p,v, X, - glm::dot(X, zero_position + X*(floor(first_x/block_size)*block_size+block_size)));
    float top_t = rayPlaneIntersect(p,v, Y, - glm::dot(Y, zero_position + Y*(floor(first_y/block_size)*block_size)));
    float bottom_t = rayPlaneIntersect(p,v, Y, - glm::dot(Y, zero_position + Y*(floor(first_y/block_size)*block_size+block_size)));

    // how ray t changes with movement in texture coordinates
    float tperblockx = right_t-left_t ;
    float tperblocky = bottom_t-top_t ;
    float zpert = glm::dot(v, Z)/glm::dot(Z,Z) ;


    int xstep  ;
    float nextx ;
    if(right_t > left_t){
        xstep = 1;
        nextx = right_t ;
    }else{
        xstep = -1;
        nextx = left_t ;
    }

    int ystep  ;
    float nexty ;
    if(bottom_t > top_t){
        ystep = 1;
        nexty = bottom_t ;
    }else{
        ystep = -1;
        nexty = top_t ;
    }

    int bx = (int)(first_x/block_size) ;
    int by = (int)(first_y/block_size) ;
    float t = 0 ;
    float z = first_z ;
    byte* depth_image = block_depth.getByteArray();


    // loop until z is in range or past the edge
    float max_z = depth_map[255]+0.001f ;
    int depth = depth_image[by * bwidth + bx] ;
    int first_depth = depth ;
    int steps = 0 ;
  
    while(z < max_z && bx >= 0 && bx < bwidth && by >=0 && by < bheight && z > depth_map[depth] && (depth > 0 || z >0)){
        block_steps++;
        // step horizontally or vertically based on which you would hit next
        float next_t = nextx < nexty ? nextx : nexty ;
        z += zpert*(next_t-t) ;
        if(z <= depth_map[depth]){ // if we would hit the bar before exiting
            break ; // exit without stepping
        }
        t = next_t ;
        if(nextx < nexty){
            nextx += tperblockx*xstep;
            bx += xstep;
        }else{
            nexty += tperblocky*ystep;
            by += ystep;
        }
        depth = depth_image[by * bwidth + bx] ;
        steps++;
        //printf(" bx: %d, by:%d, depth: %d,  steps: %d\n", bx, by, depth, steps);
    }

    return p + v*t ;
}

// Returns the t for ray p+v*t  of the first pixel the given ray hits.
// returns -1 if no hit
// This function steps 1 pixel at a time, so you'll want to use other methods to step the ray closer to its intersection point before calling this
float HologramPanel::firstPixelHit(const glm::vec3 &p, const glm::vec3 &v){
    float first_x = glm::dot(p-zero_position, X)/glm::dot(X,X);
    float first_y = glm::dot(p-zero_position, Y)/glm::dot(Y,Y);
    float first_z = glm::dot(p-zero_position, Z)/glm::dot(Z,Z);

    // get t value on sides of pixel column by intersecting ray with planes of pixel boundary
    float left_t = rayPlaneIntersect(p,v, X, - glm::dot(X, zero_position + X*floor(first_x)));
    float right_t = rayPlaneIntersect(p,v, X, - glm::dot(X, zero_position + X*(floor(first_x)+1)));
    float top_t = rayPlaneIntersect(p,v, Y, - glm::dot(Y, zero_position + Y*floor(first_y)));
    float bottom_t = rayPlaneIntersect(p,v, Y, - glm::dot(Y, zero_position + Y*(floor(first_y)+1)));

    // how ray t changes with movement in texture coordinates
    float tperx = right_t-left_t ;
    float tpery = bottom_t-top_t ;
    float zpert = glm::dot(v, Z)/glm::dot(Z,Z) ;

    float max_z = depth_map[255]+0.001f ;
    //compute t that leaves the image outside loop so we only need to check one axis in the loop
    float exit_t = std::numeric_limits<float>::max() ;
    if(zpert > 0){
        exit_t = fmin(exit_t, (max_z-first_z)/zpert) ;
    }else{
        exit_t = fmin(exit_t, (0-first_z)/zpert) ;
    }

    if(tperx > 0){
        exit_t = fmin(exit_t, (width-first_x)*tperx);
    }else{
        exit_t = fmin(exit_t, (0-first_x)*tperx);
    }

    if(tpery > 0){
        exit_t = fmin(exit_t, (height-first_y)*tpery);
    }else{
        exit_t = fmin(exit_t, (0-first_y)*tpery);
    }

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

    int x = (int)first_x ;
    int y = (int)first_y ;
    float t = 0 ;
    float z = first_z ;
    byte* image = depth_texture.getByteArray();

    // loop until z is in range or past the edge
    int i = y * width + x ;
    int yistep = width*ystep;
    int xistep = xstep;
    int depth = image[i] ;
    float depth_value = depth_map[depth] ;
    float tperxstep = tperx*xstep ;
    float tperystep = tpery*ystep ;

    //while(z < max_z && x >=0 && x < width && y >=0 && y < height && z > depth_map[depth] && (depth > 0 || z >0)){
     while(t < exit_t && z > depth_value){ // while in box and not hitting a bar
        // step horizontally or vertically based on which you would hit next
        float next_t = nextx < nexty ? nextx : nexty ;
        z += zpert*(next_t-t) ;
        if(z > depth_value){ // if we wouldn't hit the bar before exiting this pixel
            // step to next pixel
            t = next_t ;
            if(nextx < nexty){
                nextx += tperxstep;
                x += xstep;
                i += xistep;
            }else{
                nexty += tperystep;
                y += ystep;
                i += yistep;
            }
            //fetch depth index and value
            depth = image[i] ; //i = (y * width + x)*channels + depth_channel ;
            depth_value = depth_map[depth] ;
        }
    }

    ray_steps += abs(x-(int)first_x) + abs(y-(int)first_y) ;// don't add to it in the loop, just computer by change afterwards


    if(z < max_z && x >=0 && x < width && y >=0 && y < height && depth > 0){
        return t;
    }else{
        return -1.0f;
    }
}

// returns the t value where p +v*t would hit the panel
// returns -1 if the ray does not hit the panel
float HologramPanel::rayTrace(const glm::vec3 &p, const glm::vec3 &v){
    vec3 enter_p = getFirstPointInBox(p,v);
    if(enter_p.x == 0.0f && enter_p.y == 0.0f && enter_p.z == 0.0f){
        return -1.0f ;
    }
    ray_calls++;
    
    if(block_size > 0){
        enter_p = getFirstPointInBlock(enter_p,v);
    }
    return firstPixelHit(enter_p,v);
}

// returns t value where ray (p + v*t) intersects a plane (N*x + d = 0)
float HologramPanel::rayPlaneIntersect(const glm::vec3 &p, const glm::vec3 &v, const glm::vec3 &N, const float d){
    return (-d - glm::dot(N,p)) / glm::dot(N,v) ;
}

// Scores how well this panel works with the given ray (lower is better)
float HologramPanel::scoreAlignment(const glm::vec3 &p, const glm::vec3 &v){
    float vv = glm::dot(v,v);
    float vn = glm::dot(v, Z);
    return -vn/sqrt(vv);
}