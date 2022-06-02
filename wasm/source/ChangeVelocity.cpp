#include "ChangeVelocity.h"
#include "MovingObject.h"

using std::string ;
using std::map ;

ChangeVelocity::ChangeVelocity(int moving_object, glm::vec3 v){
    anchor_id = moving_object;
    new_velocity = v ;
}

// Serialize this event's data, so it can be efficiently moved between timelines
std::map<std::string,Variant> ChangeVelocity::serialize(){
    map<string,Variant> serial;
    serial["v"] = Variant(new_velocity);
    serial["t"] = Variant(time);
    serial["a"] = Variant(anchor_id);
    return serial;

}

// Set this event to data generated by its serialize method
void ChangeVelocity::set(std::map<std::string,Variant>& serial){
    new_velocity = serial["v"].getVec3();
    time = serial["t"].getDouble();
    anchor_id = serial["a"].getInt();
}

// Runs the event
// This is what you need to override to implement your application
// To maintain causality run should only interact with dynamic data by using the privided methods:
// get(id), getMutable(), addEvent, createObject, deleteObject, and getCollisions
void ChangeVelocity::run(){
    MovingObject* o = (MovingObject*)getMutable() ;
    o->velocity = new_velocity ;
}