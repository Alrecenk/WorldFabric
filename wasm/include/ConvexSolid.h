#ifndef _CONVEX_SOLID_H_
#define _CONVEX_SOLID_H_ 1

#include "TObject.h"
#include "ConvexShape.h"
#include "glm/vec3.hpp"
#include <string>

class ConvexSolid : public TObject{

    public:
        int shape_id ;
        glm::vec3 velocity;
        glm::quat orientation;
        glm::vec3 angular_velocity;
        float last_set_time = 0 ;
        float mass;
        bool moveable = true;

        //temporary computed variables
        std::vector<glm::vec3> world_vertex;
        std::vector<std::pair<glm::vec3, float>> world_plane;
        int status = 0 ;// for debug render 0 = no collison, 1 = sphere collision, 2 = full collision

        glm::mat3 inertia ;
        glm::mat3 inverse_inertia ;

        ConvexSolid();

        ConvexSolid(int shape_id, float mass, glm::vec3 p, glm::quat orientation);

        ConvexSolid(glm::vec3 nposition, float nradius, float nmass, int nshape_id, glm::vec3 nvelocity, glm::quat norientation, glm::vec3 nangular_velocity);
    
        ~ConvexSolid() override;

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

        // Returns the matrix mapping the shape's local points into world space
        glm::mat4 getTransform();

        // Steps this solid forward by the given amount of time
        void move(double dt);

        void computeWorldPlanes(std::shared_ptr<ConvexShape> shape);

        void computeInertia(std::shared_ptr<ConvexShape> shape);

        // Checks if there is a collision between this solid and another
        // Assumes both solids have computed up to date world planes
        // Returns the minimal projection vector to move this object to no longer collide
        // If there was a collision the second element will be the point of collision, and third will be normal
        // If there is not a collision returns empty vector(0,0,0) for both vectors.
        std::vector<glm::vec3> checkCollision(std::shared_ptr<ConvexSolid> other);

        // Given an object that does collide with the collision point and normal
        // return the impulse to be applied to this object to resolve the collision (negative should be applied to other)
        glm::vec3 getCollisionImpulse(std::shared_ptr<ConvexSolid> other,const glm::vec3& collision_point, const glm::vec3& collision_normal, double elasticity);

        // apply an impulse (momentum change) at the given point in world cooredinates
        void applyImpulse(const glm::vec3& impulse, const glm::vec3& point);
      
};
#endif // #ifndef _CONVEX_SOLID_H_