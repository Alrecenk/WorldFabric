#include "ChangeBallVelocity.h"
#include "BouncingBall.h"

using std::string ;
using std::map ;
using std::weak_ptr;
using std::shared_ptr;
using std::unique_ptr;


ChangeBallVelocity::ChangeBallVelocity(){

}
ChangeBallVelocity::ChangeBallVelocity(int moving_object, glm::vec3 v){
    anchor_id = moving_object;
    new_velocity = v ;
}

ChangeBallVelocity::~ChangeBallVelocity() {}

// Serialize this event's data, so it can be efficiently moved between timelines
std::map<std::string,Variant> ChangeBallVelocity::serialize() const{
    map<string,Variant> serial;
    serial["v"] = Variant(new_velocity);
    serial["t"] = Variant(time);
    serial["a"] = Variant(anchor_id);
    return serial;

}

// Set this event to data generated by its serialize method
void ChangeBallVelocity::set(std::map<std::string,Variant>& serial){
    new_velocity = serial["v"].getVec3();
    time = serial["t"].getDouble();
    anchor_id = serial["a"].getInt();
}

// Runs the event
// This is what you need to override to implement your application
// To maintain causality run should only interact with dynamic data by using the privided methods:
// get(id), getMutable(), addEvent, createObject, deleteObject, and getCollisions
void ChangeBallVelocity::run(){
    //printf("Running change velocity...\n");
    weak_ptr<TObject> ow = getMutable() ;
    if(auto og = ow.lock()){
        shared_ptr<BouncingBall> o = std::static_pointer_cast<BouncingBall>(og);
        o->velocity = new_velocity ;
    }
}