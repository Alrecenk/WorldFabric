#ifndef _HOLOGRAM_PANEL_H_
#define _HOLOGRAM_PANEL_H_ 1

#include <vector>
#include <memory>
#include <utility>
#include "glm/glm.hpp"
#include "Variant.h"

class HologramPanel{
    public:
        glm::vec3 zero_position; // 3D position of pixel 0,0
        glm::vec3 X;// change in 3D space when moving one pixel in the x direction
        glm::vec3 Y; // change in 3D space when moving one pixel in the y direction;
        glm::vec3 Z; // change in 3D space when moving one unit in the depth direction (facing toward the camera)
        int width, height ; // dimensions of the texture and depth data
        Variant depth_texture ; //byte array with index = num_channels * (width * y + x) + channel


        // maps depth channel values to actual depths (which will be multiplied by Z)
        // 0 is a transparent pixel, so dpeth_map zero isn't used
        //depth_map[1] should always be 0, and values should be increasing to depth[255]
        //depth_map[255] is the max, which makes it the closest depth to the screen because z faces camera
        std::vector<float> depth_map ; 
        

        int block_size = 0;
        int bwidth, bheight;
        Variant block_depth ;


        int ray_steps = 0 ;
        int ray_calls = 0 ;
        int block_steps = 0;


        // position of upper left corner of image, how pixel coordinates convert to 3D space and normal of image plane (typically facing toward camera)
        HologramPanel(glm::vec3 zero, glm::vec3 xperpixel, glm::vec3 yperpixel, glm::vec3 z_normal);

        // sets depths and depth map by k-means clustering
        //Adjusts zero position so limited precision depths efficiently cover the whole range
        void setDepth(std::vector<std::vector<float>>& depth);

        // returns the position where the given ray first intersects the bounding box of this panel
        // or returns p if p is inside the panel already
        // returns 0,0,0 if the ray does not intersect the box TODO remove magic number
        glm::vec3 getFirstPointInBox(const glm::vec3 &p, const glm::vec3 &v);

        // builds the blockdepth image for use with getFirstBlockHit
        void buildBlockImage(int size);

        // returns the position where the given ray first intersects a block
        // or returns p if p is inside a block already
        // returns 0,0,0 if the ray does not intersect the box TODO remove magic number
        glm::vec3 getFirstPointInBlock(const glm::vec3 &p, const glm::vec3 &v);

        // Returns the t for ray p+v*t  of the first pixel the given ray hits.
        // returns -1 if no hit
        // This function steps 1 pixel at a time, so you'll want to use other methods to step the ray closer to its intersection point before calling this
        float firstPixelHit(const glm::vec3 &p, const glm::vec3 &v);

        // returns the t value where p +v*t would hit the panel
        // returns -1 if the ray does not hit the panel
        float rayTrace(const glm::vec3 &p, const glm::vec3 &v);

        // returns t value where ray (p + v*t) intersects a plane (N*x + d = 0)
        static float rayPlaneIntersect(const glm::vec3 &p, const glm::vec3 &v, const glm::vec3 &N, const float d) ;

        // Scores how well this panel works with the given ray (lower is better)
        float scoreAlignment(const glm::vec3 &p, const glm::vec3 &v);

};
#endif // #ifndef _DHOLOGRAM_PANEL_H_