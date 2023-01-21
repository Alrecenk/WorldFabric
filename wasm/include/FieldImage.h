#ifndef _FIELD_IMAGE_H_
#define _FIELD_IMAGE_H_ 1

#include <vector>
#include <memory>
#include <utility>
#include "glm/glm.hpp"
#include "Variant.h"


class FieldImage{
    public:
        glm::vec3 position;
        glm::vec3 normal;
        glm::mat2x3 to_pixel ; // matrix mapping 3D points on image plane to image coordinates
        int width, height, channels ;
        Variant texture ; 
        Variant point_texture ; 

    // position and normal of image location
    // matrix mapping 3D rays to pixel locations is built using 4 examples
    FieldImage(glm::vec3 p, glm::vec3 n , const std::vector<std::pair<glm::vec3, glm::vec2>> &v_to_pixel);

    // Moves the image data into this field (destroys the variant passed in)
    void moveImage(Variant& image_data, Variant& point_data, int w, int h, int c) ;

    // Returns distance between origin of image and closest point on a ray
    float rayDistance(const glm::vec3 &p, const glm::vec3 &v);

    // get the color of a ray
    glm::vec3 getColor(const glm::vec3 &p, const glm::vec3 &v);

    glm::vec2 getTextureCoordinates(const glm::vec3 &v);

    glm::vec3 getPoint(int tx, int ty);

    glm::vec3 getColor(int tx, int ty);

    float pointLineDistance(const glm::vec3 &p, const glm::vec3 &v, const glm::vec3 &x);

};
#endif // #ifndef _FIELD_IMAGE_H_