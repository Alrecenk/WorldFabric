#include "MoveObject.h"
#include "MovingObject.h"

using std::string ;
using std::map ;


MoveObject::MoveObject(){

}
MoveObject::MoveObject(int moving_object, double move_step){
    anchor_id = moving_object;
    interval = move_step;
}

MoveObject::MoveObject(double move_step){
    anchor_id = -99999;
    interval = move_step;
}

MoveObject::~MoveObject() {}

// Serialize this event's data, so it can be efficiently moved between timelines
std::map<std::string,Variant> MoveObject::serialize() const{
    map<string,Variant> serial;
    serial["dt"] = Variant(interval);
    serial["t"] = Variant(time);
    serial["a"] = Variant(anchor_id);
    return serial;

}

// Set this event to data generated by its serialize method
void MoveObject::set(std::map<std::string,Variant>& serial){
    interval = serial["dt"].getDouble();
    time = serial["t"].getDouble();
    anchor_id = serial["a"].getInt();
}

// Runs the event
// This is what you need to override to implement your application
// To maintain causality run should only interact with dynamic data by using the privided methods:
// get(id), getMutable(), addEvent, createObject, deleteObject, and getCollisions
void MoveObject::run(){
    MovingObject* o = (MovingObject*)getMutable() ;
    o->position += o->velocity * (float)interval;
    //printf("Object moving to : %f,%f, %f\n", o->position.x, o->position.y, o->position.z);

    std::unique_ptr<MoveObject> next_tick = std::make_unique<MoveObject>(anchor_id, interval);
    next_tick->time = time + interval ;
    addEvent(std::move(next_tick));
}