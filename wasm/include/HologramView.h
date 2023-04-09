#ifndef _HOLOGRAM_VIEW_H_
#define _HOLOGRAM_VIEW_H_ 1

#include <vector>
#include <memory>
#include <utility>
#include "glm/glm.hpp"
#include "Variant.h"


class HologramView{
    public:
        glm::vec3 position;
        glm::vec3 normal;
        glm::mat2x3 to_pixel ; // matrix mapping 3D points on image plane to image coordinates
        int width, height ;
        int channels ;
        Variant texture ; 

        static constexpr int red_channel=0; // which channel is red
        static constexpr int green_channel=1; // which channel is green
        static constexpr int blue_channel=2; // which channel is blue
        static constexpr int depth_channel=3 ;// which channel is depth index

        // maps depth channel values to actual depths (which will be multiplied by Z)
        // 0 is a transparent pixel, so dpeth_map zero isn't used
        //depth_map[1] should always be 0, and values should be increasing to depth[255]
        //depth_map[255] is the max, which makes it the closest depth to the screen because z faces camera
        std::vector<float> depth_map ; 
    
        // position and normal of image location
        // matrix mapping 3D rays to pixel locations is built using 4 examples
        HologramView(glm::vec3 p, glm::vec3 n , const std::vector<std::pair<glm::vec3, glm::vec2>> &v_to_pixel);

        // Moves the image data into this field (destroys the variant passed in)
        void moveImage(Variant& image_data, int w, int h, int c) ;

        // get the color of a point this image can see
        glm::vec3 getColor(const glm::vec3 &p);

        glm::vec2 getTextureCoordinates(const glm::vec3 &v);

        glm::vec3 getColor(int tx, int ty);

        // sets depths and depth map by k-means clustering
        //Also adjusts zero position so depths efficiently cover the whole range
        void setDepth(std::vector<std::vector<float>>& depth);

        // return true if the given point is visible by this image
        bool checkVisibility(const glm::vec3 &p);

        //scores how well this image aligns for a ray starting at p0 and hitting a solid at intersect (lower is better)
        float scoreAlignment(const glm::vec3& p0, const glm::vec3& intersect);

        float blendWeight(const glm::vec3& p0, const glm::vec3& intersect);

};
#endif // #ifndef _HOLOGRAM_VIEW_H_