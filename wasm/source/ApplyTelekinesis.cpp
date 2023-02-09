#include "ApplyTelekinesis.h"
#include "ConvexSolid.h"
#include "GLTF.h"

using std::string ;
using std::map ;
using std::weak_ptr;
using std::shared_ptr;
using std::unique_ptr;
using glm::vec3;


ApplyTelekinesis::ApplyTelekinesis(){
    type = 6 ;
}
ApplyTelekinesis::ApplyTelekinesis(int solid_id, glm::vec3 dv, float ma, glm::quat o, float mas){
    anchor_id = solid_id;
    target_velocity = dv;
    max_acceleration = ma;
    //target_angular_velocity = dav;
    max_angular_speed = mas;
    new_orientation = o;
    type = 6 ;
}

ApplyTelekinesis::~ApplyTelekinesis() {}

// Serialize this event's data, so it can be efficiently moved between timelines
std::map<std::string,Variant> ApplyTelekinesis::serialize() const {
    map<string,Variant> serial;
    serial["a"] = Variant(anchor_id);
    serial["t"] = Variant(time);
    serial["v"] = Variant(target_velocity);
    serial["ma"] = Variant(max_acceleration);
    //serial["w"] = Variant(target_angular_velocity);
    serial["mas"] = Variant(max_angular_speed);
    serial["type"] = Variant(type);
    serial["o"] = Variant(new_orientation);
    return serial;
}

// Set this event to data generated by its serialize method
void ApplyTelekinesis::set(std::map<std::string,Variant>& serialized){
    anchor_id = serialized["a"].getInt();
    time = serialized["t"].getDouble();
    target_velocity = serialized["v"].getVec3();
    //target_angular_velocity = serialized["w"].getVec3();
    max_acceleration = serialized["ma"].getFloat();
    max_angular_speed = serialized["mas"].getFloat();
    new_orientation = serialized["o"].getQuat();
}

// Runs the event
// This is what you need to override to implement your application
// To maintain causality run should only interact with dynamic data by using the privided methods:
// get(id), getMutable(), addEvent, createObject, deleteObject, and getCollisions
void ApplyTelekinesis::run(){
    weak_ptr<TObject> ow = getMutable() ;
    if(auto og = ow.lock()){
        shared_ptr<ConvexSolid> self = std::static_pointer_cast<ConvexSolid>(og);
        float dt = 1.0f/90.0f; // TODO don't ha5rdcode
        vec3 acc = target_velocity-self->velocity;
        if(acc.length() > max_acceleration*dt){
            acc *= max_acceleration*dt/acc.length() ;
        }
        self->velocity += acc;

        /*
        vec3 aacc = target_angular_velocity-self->angular_velocity;
        if(aacc.length() > max_angular_acceleration*dt){
            aacc *= max_angular_acceleration*dt/aacc.length() ;
        }
        self->angular_velocity += aacc;
        */

        //self->orientation = new_orientation;


        // compute rotation change as an angular velocity vector
        glm::quat dq = new_orientation * glm::inverse(self->orientation); 
        float angle = fabs(glm::angle(dq));
        float as = angle /dt ; //how fast we would have to go to move into the position in one frame
        if(as < max_angular_speed){
            self->orientation = new_orientation;
            
        }else{
            self->orientation = GLTF::slerp(self->orientation, new_orientation, max_angular_speed/as);
        }
        self->angular_velocity = vec3(0,0,0);
        /*
        vec3 axis = glm::axis(dq);
        vec3 da = (axis*angle)/dt ;
        float l = glm::length(da) ;
        if(l > max_angular_speed){ // if can't get there this frame then set angular velocity to max
            da *= max_angular_speed/l;
            self->angular_velocity = da ;
        }else{ // if can get there this frame, then set orientation
            self->orientation = new_orientation;
            self->angular_velocity = vec3(0,0,0);
        }
        */


        //self->last_set_time = time ;
        weak_ptr<TObject> sw = get(self->shape_id) ; 
        if(auto sg = sw.lock()){ // if has a shape
            shared_ptr<ConvexShape> shape = std::static_pointer_cast<ConvexShape>(sg);
            self->radius = shape->radius; // keep radius up to date with shape
            self->computeWorldPlanes(shape);
        }

    }
}