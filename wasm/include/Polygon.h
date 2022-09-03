#ifndef _POLYGON_H_
#define _POLYGON_H_ 1

#include <vector>
#include <memory>
#include <utility>
#include "glm/glm.hpp"

class Polygon{
    public:
        std::vector<glm::dvec3> p;
        static constexpr double EPSILON = 0.00001;

        Polygon();
        Polygon(std::vector<glm::dvec3> np);

        // Returns N and d such that N*x+d = 0 for plane and N points out of shape
        std::pair<glm::dvec3, double> getPlane() const ;

        // Returns two polygons of this polygon split on the given plane
        // Note: either polygon may be empty
        std::pair<Polygon, Polygon> splitOnPlane(const std::pair<glm::dvec3, double>& plane) const;

        std::vector<glm::dvec3> getPlaneIntersections(const std::pair<glm::dvec3, double>& plane) const;

        glm::dvec3 getCenter() const;

};
#endif // #ifndef _POLYGON_H_