#include "ApplyBallImpulse.h"
#include "BouncingBall.h"

using std::string ;
using std::map ;
using std::weak_ptr;
using std::shared_ptr;
using std::unique_ptr;


ApplyBallImpulse::ApplyBallImpulse(){
    type = 4 ;
}
ApplyBallImpulse::ApplyBallImpulse(int moving_object, glm::vec3 v){
    anchor_id = moving_object;
    impulse = v ;
    type = 4 ;
}

ApplyBallImpulse::~ApplyBallImpulse() {}

// Serialize this event's data, so it can be efficiently moved between timelines
std::map<std::string,Variant> ApplyBallImpulse::serialize() const{
    map<string,Variant> serial;
    serial["i"] = Variant(impulse);
    serial["t"] = Variant(time);
    serial["a"] = Variant(anchor_id);
    return serial;

}

// Set this event to data generated by its serialize method
void ApplyBallImpulse::set(std::map<std::string,Variant>& serial){
    impulse = serial["i"].getVec3();
    time = serial["t"].getDouble();
    anchor_id = serial["a"].getInt();
}

// Runs the event
// This is what you need to override to implement your application
// To maintain causality run should only interact with dynamic data by using the privided methods:
// get(id), getMutable(), addEvent, createObject, deleteObject, and getCollisions
void ApplyBallImpulse::run(){
    weak_ptr<TObject> ow = getMutable() ;
    if(auto og = ow.lock()){
            shared_ptr<BouncingBall> o = std::static_pointer_cast<BouncingBall>(og);
            float mass = o->radius*o->radius ;
            o->velocity += impulse/mass ;
    }
}