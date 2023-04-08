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
        int channels = 3 ;
        Variant texture ; 
    
        // position and normal of image location
        // matrix mapping 3D rays to pixel locations is built using 4 examples
        HologramView(glm::vec3 p, glm::vec3 n , const std::vector<std::pair<glm::vec3, glm::vec2>> &v_to_pixel);

        // Moves the image data into this field (destroys the variant passed in)
        void moveImage(Variant& image_data, int w, int h, int c) ;

        // get the color of a point this image can see
        glm::vec3 getColor(const glm::vec3 &p);

        glm::vec2 getTextureCoordinates(const glm::vec3 &v);

        glm::vec3 getColor(int tx, int ty);

        //scores how well this image aligns for a ray starting at p0 and hitting a solid at intersect (lower is better)
        float scoreAlignment(const glm::vec3& p0, const glm::vec3& intersect);

};
#endif // #ifndef _HOLOGRAM_VIEW_H_