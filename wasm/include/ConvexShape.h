#ifndef _CONVEX_SHAPE_H_
#define _CONVEX_SHAPE_H_ 1

#include "TObject.h"
#include "Polygon.h"
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/ext.hpp"
#include <vector>



class ConvexShape : public TObject{
  public:

    std::vector<glm::vec3> vertex ;
    std::vector<std::vector<int>> face ;

    ConvexShape();

    ConvexShape(const std::vector<glm::vec3>& vertices, const std::vector<std::vector<int>>& faces);
    ConvexShape(const std::vector<glm::dvec3>& vertices, const std::vector<std::vector<int>>& faces);

    ConvexShape(std::vector<Polygon>& polygons);

    ~ConvexShape() override;

    // Serialize this object, so it can be efficiently moved between timelines
    std::map<std::string,Variant> serialize() const override;

    // Set this object to data generated by its serialize method
    void set(std::map<std::string,Variant>& serialized) override;

    // Override this to provide an efficient deep copy of this object
    // If not overridden serialize and set will be used to copy your object (which will be inefficent)
    std::unique_ptr<TObject> deepCopy() override;

    // Override this function to provide logic for interpolation after rollback or extrapolation for slowly updating objects
    // If not overridden getObserved returns the raw value of the object
    std::unique_ptr<TObject> getObserved(double time, const std::weak_ptr<TObject> last_observed, double last_time) override;

    // Return the center of mass of this shape
    glm::vec3 getCentroid();

    // Moves this shape so the origin aligns with the centroid and returns the move that was made
    glm::vec3 centerOnCentroid();

    float getVolume();

    // Returns the inertia tensor of the entire shape about the origin
    glm::mat3 getInertia(const float mass);

    // return the volume of the given tetrahedron
    static float computeTetraVolume(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d);

    // Returns the center of mass of the given tetrahedron
    static glm::vec3 computeTetraCentroid(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d);

    // Returns the inertia tensor of the given tetrahedron about the origin assuming a uniform density of 1
    static glm::mat3 computeTetraInertia(const float mass, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d);

    // Returns a shaoe for an axis aligned bounding box
    static ConvexShape makeAxisAlignedBox(glm::vec3 min, glm::vec3 max);

    //Alternate form that always centers on the origin
    static ConvexShape makeAxisAlignedBox(glm::vec3 size);

    // Returns a shape for a sphere with 8*(4^detail) triangles
    static ConvexShape makeSphere(glm::vec3 center, float radius, int detail);

    // Helper method for sphere extrapolation
    // Averages two points then pushes result to lie on the sphere.
    static glm::vec3 makeSpherePoint(glm::vec3 a, glm::vec3 b, glm::vec3 center, float radius);

    // Returns a shape for a cylinder with center of ends and A and B
    static ConvexShape makeCylinder(glm::vec3 A, glm::vec3 B, float radius, int sides);

    // Returns a shape for a Tetrahedron with the given points
    static ConvexShape makeTetra(glm::vec3 A, glm::vec3 B, glm::vec3 C, glm::vec3 D);


    std::vector<Polygon> getPolygons();

};
#endif // #ifndef _CONVEX_SHAPE_H_