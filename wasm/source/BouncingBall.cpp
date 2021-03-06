#include "BouncingBall.h"

#include "CreateObject.h"
#include "MoveBouncingBall.h"
#include "ChangeBallVelocity.h"
#include "ApplyBallImpulse.h"
#include "BallWall.h"

using std::map;
using std::string;
using std::vector;
using glm::vec3;

BouncingBall::BouncingBall(){
    type = 1 ;
}

BouncingBall::BouncingBall(glm::vec3 p, glm::vec3 v, float r){
    position = p ;
    velocity = v;
    radius = r ;
    type = 1 ;
}

BouncingBall::~BouncingBall() {}

// Serialize this object, so it can be efficiently moved between timelines
std::map<std::string,Variant> BouncingBall::serialize() const{
    //printf("serializing bouncing ball\n");
    map<string,Variant> serial;
    serial["p"] = Variant(position);
    serial["v"] = Variant(velocity);
    serial["r"] = Variant(radius);
    return serial;
}

// Set this object to data generated by its serialize method
void BouncingBall::set(std::map<std::string,Variant>& serial){
    //printf("setting a bouncing ball\n");
    position = serial["p"].getVec3();
    velocity = serial["v"].getVec3();
    radius = serial["r"].getFloat();
    //printf("finished setting a bouncing ball\n");
}

// Override this to provide an efficient deep copy of this object
// If not overridden serialize and set will be used to copy your object (which will be inefficent)
std::unique_ptr<TObject> BouncingBall::deepCopy(){
    return std::make_unique<BouncingBall>(position, velocity, radius);
}

// Override this function to provide logic for interpolation after rollback or extrapolation for slowly updating objects
// If not overridden getObserved returns the raw value of the object
std::unique_ptr<TObject> BouncingBall::getObserved(double time, const std::weak_ptr<TObject> last_observed, double last_time){
    vec3 observed_position = position + velocity * (float)(time-write_time);

    float max_speed = 1.3*glm::length(velocity) ;
    float max_move = 10.0+max_speed * (time-last_time);
    
    if(auto last = last_observed.lock()){
        vec3 ov = observed_position - last->position ;
        float os = glm::length(ov) ;
        if(os < 100){ // only interpolate if not spawning or teleporting
            if(os > max_move){
                //printf("Capping speed: max_move: %f, os: %f, max_speed:%f\n", max_move, os, max_speed);
                //printf("time_ms: %l, last: %l\n", time_ms,last_time_ms);
                observed_position = last->position + (ov * (max_move/os)) ;
            } 
        }
    }
    
    return std::make_unique<BouncingBall>(observed_position, velocity, radius);
}

std::unique_ptr<TObject> BouncingBall::createObject(const Variant& serialized){
    //printf("callec create ball object!\n");
    //serialized.printFormatted();
    if(serialized.type_ != Variant::OBJECT){
        printf("timeline attemped to create an object with a nonobject variabnt!\n");
    }
    auto map = serialized.getObject() ;
    std::unique_ptr<TObject> obj ;
    if(map.find("p") != map.end()){
        obj = std::make_unique<BouncingBall>();
    }else{
        obj = std::make_unique<BallWall>();
    }
    obj->set(map);
    return std::move(obj);
}

std::unique_ptr<TEvent> BouncingBall::createEvent(const Variant& serialized){
    
    //TODO add a type system to make this check more intuitive
    if(serialized.type_ == Variant::NULL_VARIANT){ // events can hold poiners to other events which may be null
        return std::unique_ptr<TEvent>(nullptr);
    }
    auto map = serialized.getObject() ;
    std::unique_ptr<TEvent> event ;
    //TODO better way to distinguish event types
    if(map["o"].type_ == Variant::OBJECT){
        event = std::make_unique<CreateObject>();
    }else if(map["dt"].type_ == Variant::DOUBLE){
        event = std::make_unique<MoveBouncingBall>();
    }else if(map["v"].type_ == Variant::FLOAT_ARRAY){
        event = std::make_unique<ChangeBallVelocity>();
    }else if(map["i"].type_ == Variant::FLOAT_ARRAY){
        event = std::make_unique<ApplyBallImpulse>();
    }else{
        printf("Event not parsed!\n");
        serialized.printFormatted();
        //event = std::make_unique<ChangeVelocity>();
    }
    event->set(map);
    return std::move(event);
}