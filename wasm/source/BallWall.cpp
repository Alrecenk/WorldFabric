#include "BallWall.h"

using std::map;
using std::string;
using std::vector;

BallWall::BallWall(){
    type = 2 ;
}

BallWall::BallWall(glm::vec3 box_min, glm::vec3 box_max){
    min = box_min ;
    max = box_max ;
    position = (min+max)*0.5f;
    radius = glm::length(min-max)*0.5f;
    type = 2 ;
}


BallWall::~BallWall() {}

// Serialize this object, so it can be efficiently moved between timelines
std::map<std::string,Variant> BallWall::serialize() const{
    map<string,Variant> serial;
    serial["min"] = Variant(min);
    serial["max"] = Variant(max);
    return serial;
}

// Set this object to data generated by its serialize method
void BallWall::set(std::map<std::string,Variant>& serial){
    min = serial["min"].getVec3();
    max = serial["max"].getVec3();
    position = (min+max)*0.5f;
    radius = glm::length(min-max)*0.5f;
}

// Override this to provide an efficient deep copy of this object
// If not overridden serialize and set will be used to copy your object (which will be inefficent)
std::unique_ptr<TObject> BallWall::deepCopy(){
    return std::make_unique<BallWall>(min, max);
}

// Override this function to provide logic for interpolation after rollback or extrapolation for slowly updating objects
// If not overridden getObserved returns the raw value of the object
std::unique_ptr<TObject> BallWall::getObserved(double time, const std::weak_ptr<TObject> last_observed, double last_time){
    return deepCopy();
}
