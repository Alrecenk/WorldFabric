#ifndef _DEPTH_PANEL_H_
#define _DEPTH_PANEL_H_ 1

#include <vector>
#include <memory>
#include <utility>
#include "glm/glm.hpp"
#include "Variant.h"


class DepthPanel{
    public:
        glm::vec3 zero_position; // 3D position of pixel 0,0
        glm::vec3 X;// change in 3D space when moving one pixel in the x direction
        glm::vec3 Y; // change in 3D space when moving one pixel in the y direction;
        glm::vec3 Z; // change in 3D space when moving one unit in the depth direction (facing toward the camera)
        int width, height ; // dimensions of the texture and depth data
        Variant texture ; //byte array with index = num_channels * (width * y + x) + channel

        static constexpr int channels = 4 ;
        static constexpr int red_channel=0; // which channel is red
        static constexpr int green_channel=1; // which channel is green
        static constexpr int blue_channel=2; // which channel is blue
        static constexpr int depth_channel=3 ;// which channel is depth index

        // maps depth channel values to actual depths (which will be multiplied by Z)
        // 0 is a transparent pixel, so dpeth_map zero isn't used
        //depth_map[1] should always be 0, and values should be increasing to depth[255]
        //depth_map[255] is the max, which makes it the closest depth to the screen because z faces camera
        std::vector<float> depth_map ; 
        
        int ray_steps = 0 ;
        int ray_calls = 0 ;


    // position of upper left corner of image, how pixel coordinates convert to 3D space and normal of image plane (typically facing toward camera)
    DepthPanel(glm::vec3 zero, glm::vec3 xperpixel , glm::vec3 yperpixel , glm::vec3 z_normal);

    // Moves the image data into this field (destroys the variant passed in)
    // w,h,channels defines dimension of image data bytes
    // The image bytes should be packed with the number of channels and arranged as defined in const expressions in DephPanel.h
    // depth values are indices into a depth_map 
    //(depth=0 is transparent, depth[1]= 0 increasing to depth[255] = max depth /closest to expected view position)
    void moveImage(Variant& image_data, int w, int h, std::vector<float> depths) ;

    // Copies the image data into this field
    // w,h,channels defines dimension of image data bytes with R,G,B assumed to be the first 3 channels
    //Does not modify depth values (made ofr use with setDepth(float[][]);
    void moveImage(const Variant& image_data, int w, int h, int data_channels) ;

    // sets depths and depth map by k-means clustering
    //Also adjusts zero position so depths efficiently cover the whole range
    void setDepth(std::vector<std::vector<float>>& depth);

    // returns the position where the given ray first intersects the bounding box of this panel
    // or returns p if p is inside the panel already
    // returns 0,0,0 if the ray does not intersect the box TODO remove magic number
    glm::vec3 getFirstPointInBox(const glm::vec3 &p, const glm::vec3 &v);

    // Returns the x,y coordinates of the first pixel the given ray hits.
    // This function steps 1 pixel at a time, so you'll want to use other methods to step the ray closer to its intersection point before calling this
    std::pair<int,int> firstPixelHit(const glm::vec3 &p, const glm::vec3 &v);

    // get the color of a ray
    glm::vec3 getColor(const glm::vec3 &p, const glm::vec3 &v);

    // given coordinates into the texture, return the 3D point of the surface
    glm::vec3 getPoint(int tx, int ty);

    // given coordinates into the texture, return a 0 to 1, rgb color vector
    glm::vec3 getColor(int tx, int ty);

    static DepthPanel generateTestPanel(glm::vec3 center, float radius, glm::vec3 normal);

    // returns t value where ray (p + v*t) intersects a plane (N*x + d = 0)
    static float rayPlaneIntersect(const glm::vec3 &p, const glm::vec3 &v, const glm::vec3 &N, const float d) ;

};
#endif // #ifndef _DEPTH_PANEL_H_