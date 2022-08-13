#ifndef _CONVEX_SHAPE_H_
#define _CONVEX_SHAPE_H_ 1

#include "glm/vec3.hpp"
#include <vector>


class ConvexShape {
  public:

    std::vector<glm::vec3> vertex ;
    std::vector<vector<int>> face ;

    ConvexShape(std::vector<vec3> &vertices, std::vector<vector<int>> &faces);

    // Return the center of mass of this shape
    glm::vec3 getCentroid() const;

    // Moves this shape so the origin aligns with the centroid and returns the move that was made
    glm::vec3 centerOnCentroid();

    float getVolume() const;

    // Returns the inertia tensor of the entire shape about the origin assuming a uniform density of 1
    glm::mat3 getInertia() const;

    // return the volume of the given tetrahedron
    static float computeTetraVolume(const vec3& a, const vec3& b, const vec3& c, const vec3& d);

    // Returns the center of mass of the given tetrahedron
    static vec3 computeTetraCentroid(const vec3& a, const vec3& b, const vec3& c, const vec3& d);

    // Returns the inertia tensor of the given tetrahedron about the origin assuming a uniform density of 1
    static glm::mat3 computeTetraInertia(const vec3& a, const vec3& b, const vec3& c, const vec3& d);

    // Returns a shaoe for an axis aligned bounding box
    static ConvexShape makeAxisAlignedBox(glm::vec3 min, glm::vec3 max);

    //Alternate form that always centers on the origin
    static ConvexShape makeAxisAlignedBox(glm::vec3 size);

    // Returns a shape for a sphere with 8*(4^detail) triangles
    static ConvexShape makeSphere(glm::vec3 center, float radius, int detail);

    // Helper method for sphere extrapolation
    // Averages two points then pushes result to lie on the sphere.
    static glm::vec3 makeSpherePoint(glm::vec3 a, glm::vec3 b, glm::vec3 center, float radius);

    // Returns a Mesh for a cylinder with center of ends and A and B
    static ConvexShape makeCylinder(glm::vec3 A, glm::vec3 B, float radius, int sides);



};
#endif // #ifndef _CONVEX_SHAPE_H_