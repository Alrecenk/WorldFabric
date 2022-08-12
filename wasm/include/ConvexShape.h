#ifndef _CONEX_SHAPE_H_
#define _CONEX_SHAPE_H_ 1

#include "glm/vec3.hpp"
#include <vector>


class ConvexShape {
  public:
    struct Face{
        glm::vec3 normal ; // normal facing out
        float d ; // N dot x + d = 0 
        std:: vector<int> index ; // indices of included points
    };

    std::vector<glm::vec3> vertex ;

    // Return the center of mass of this shape
    glm::vec3 getCentroid() const;

    // Moves this shape so the origin aligns with the centroid and returns the move that was made
    glm::vec3 centerOnCentroid();

    float getVolume() const;

    // Returns the inertia tensor of the entire shape about the origin
    glm::mat3 getInertia() const;

    // Returns the inertia tensor of the given tetrahedron about the origin
    static glm::mat3 computeTetraInertia(const std::vector<glm::vec3>& points);

};
#endif // #ifndef _CONEX_SHAPE_H_