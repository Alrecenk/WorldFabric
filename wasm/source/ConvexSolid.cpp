
#include "ConvexSolid.h"

using glm::vec3 ;
using glm::mat4 ;
using glm::quat ;
using std::vector ;

ConvexSolid::ConvexSolid(){
    type = 2 ; // TODO make some constants or something
}

ConvexSolid::ConvexSolid(std::string shape, float m, glm::vec3 p, glm::quat r){
    shape_name = shape ;
    mass = m ;
    position = p ;
    velocity = vec3(0,0,0);
    orientation = r ;
    angular_velocity = vec3(0,0,0);
    type = 2 ; // TODO make some constants or something
    radius = 0 ;
    if(shape_library != nullptr){
        ConvexShape* shape = shape_library->getShape(shape_name);
        if(shape != nullptr){
            for(int k=0;k<shape->vertex.size;k++){
                float r = glm::length(shape->vertex[k]);
                if(r > radius){
                    radius = r ;
                }
            }
        }else{
            printf("ConvexSolid initialized without shape (%s)!\n", shape_name.c_str());
        }
    }else{
        printf("ConvexSolid initialized without shape library!\n");
    }
}

ConvexSolid::ConvexSolid(glm::vec3 nposition, float nradius, float nmass, string nshape_name, glm::vec3 nvelocity, glm::quat norientatation, glm::vec3 nangular_velocity){
    position = nposition;
    radius = nradius;
    shape_name = nshape_name;
    velocity = nvelocity;
    orientation = norientation;
    angular_velocity = nangular_velocity;
    mass = nmass;
}

ConvexSolid::~ConvexSolid(){
    
}

// Serialize this object, so it can be efficiently moved between timelines
std::map<std::string,Variant> ConvexSolid::serialize() const {
    map<string,Variant> serial;
    serial["p"] = Variant(position);
    serial["r"] = Variant(radius);
    serial["n"] = Variant(shape_name);
    serial["v"] = Variant(velocity);
    serial["o"] = Variant(orientation);
    serial["w"] = Variant(angular_velocity);
    serial["m"] = Variant(mass);
    return serial;
}

// Set this object to data generated by its serialize method
void ConvexSolid::set(std::map<std::string,Variant>& serialized){
    position = serialized["p"].getVec3();
    radius = serialized["r"].getFloat();
    shape_name = serialized["n"].getString();
    velocity = serialized["v"].getVec3();
    orientation = serialized["o"].getQuaternion();
    angular_velocity = serialized["w"].getVec3();
    mass = serialized["m"].getFloat();
}


// Override this to provide an efficient deep copy of this object
// If not overridden serialize and set will be used to copy your object (which will be inefficent)
std::unique_ptr<TObject> ConvexSolid::deepCopy(){
    return std::make_unique<ConvexSolid>(position, radius, mass, shape_name, velocity, orientatation, angular_velocity);
}

// Override this function to provide logic for interpolation after rollback or extrapolation for slowly updating objects
// If not overridden getObserved returns the raw value of the object
std::unique_ptr<TObject> ConvexSolid::getObserved(double time, const std::weak_ptr<TObject> last_observed, double last_time){
    return deepCopy();
}

// Returns the matrix mapping the shape's local points into world space
glm::mat4 ConvexSolid::getTransform(){
    mat4 transform = mat4(1) ;
    transform = glm::translate(transform, position);
    transform *= glm::mat4_cast(orientation);
    return transform ;
}

// Steps this solid forward by the given amount of time
void ConvexSolid::move(double dt){
    position += velocity*dt;
    float da = glm::length(angular_velocity) * dt ;
    quat dr = glm::angleAxis(da, angular_velocity);
    orientation *= dr;
}

// Checks if there is a collision between this solid and another
// Returns the minimal projection vector to move this object to no longer collide
// If there was a collision the second element will be the point of collision
// If there is not a collision return (0,0,0) for both vectors.
std::pair<glm::vec3, glm::vec3> ConvexSolid::checkCollision(ConvexSolid& other){

}

// Given an object that does collide this returns the change to velocity and angular_velocity 
// that should be applied to this for a completely elastic collision
std::pair<glm::vec3, glm::vec3> ConvexSolid::getCollisionImpulse(ConvexSolid& other){

}

