#ifndef _SPHERE_FIELD_H_
#define _SPHERE_FIELD_H_ 1

#include <vector>
#include <memory>
#include <utility>
#include "glm/glm.hpp"
#include "Variant.h"


class SphereField{
    public:
        // Sphere being mapped onto
        glm::vec3 center;
        float radius;

        int quality ; // number of subdivisions in coordinate generation
        // raw color data in 4D texture is a byte array where index is (ty*width + tx) * 3 ( +0 = r, +1 = g, +2 = b);
        int texture_width ;
        Variant texture ; 



    SphereField(glm::vec3 sphere_center, float sphere_radius, int q);

    // Add a ray to the field (overwriting any in the same location)
    void setRay(const glm::vec3 &p, const glm::vec3 &v, const glm::vec3 &color);

    // get the color of a ray
    glm::vec3 getColor(const glm::vec3 &p, const glm::vec3 &v);

    // get the texture coordinates of a given ray byu mapping its start and end onto the sphere
    std::pair<int, int> getTextureCoordinates(const glm::vec3 &p, const glm::vec3 &v);

    // returns a texture coordinate mapping the given point o nthe sphere to a single number
    int getTextureCoordinate(const glm::vec3 &r);

    // Get a ray that would generate the given texture coordinates
    std::pair<glm::vec3,glm::vec3> getRayFromTextureCoordinates(const int tx, const int ty);

    // Get the points on the sphere where the ray intesects
    std::pair<glm::vec3,glm::vec3> getRaySphereHits(const glm::vec3 &p, const glm::vec3 &v);

};
#endif // #ifndef _SPHERE_FIELD_H_