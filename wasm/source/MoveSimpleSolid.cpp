#include "MoveSimpleSolid.h"
#include "ConvexSolid.h"
#include "ApplySolidImpulse.h"

#include <algorithm>

using std::string ;
using std::map ;
using std::vector;
using glm::vec3;
using std::weak_ptr;
using std::shared_ptr;
using std::unique_ptr;


float MoveSimpleSolid::friction = 0.035 ;
float MoveSimpleSolid::angular_friction = 0.035 ;
float MoveSimpleSolid::max_speed = 10 ;
float MoveSimpleSolid::max_angular_speed = 10 ;

MoveSimpleSolid::MoveSimpleSolid(){
    type = 4 ; // TODO don't hardcode this
}
MoveSimpleSolid::MoveSimpleSolid(int moving_object, double move_step){
    anchor_id = moving_object;
    interval = move_step;
    type = 4 ;// TODO don't hardcode this
}

MoveSimpleSolid::MoveSimpleSolid(double move_step){
    anchor_id = -99999;
    interval = move_step;
    type = 4 ;
}

MoveSimpleSolid::~MoveSimpleSolid() {}

// Serialize this event's data, so it can be efficiently moved between timelines
std::map<std::string,Variant> MoveSimpleSolid::serialize() const{
    //printf("serializing move bouncing ball\n");
    map<string,Variant> serial;
    serial["dt"] = Variant(interval);
    serial["t"] = Variant(time);
    serial["a"] = Variant(anchor_id);
    serial["type"] = Variant(type);
    return serial;

}

// Set this event to data generated by its serialize method
void MoveSimpleSolid::set(std::map<std::string,Variant>& serial){
    interval = serial["dt"].getDouble();
    time = serial["t"].getDouble();
    anchor_id = serial["a"].getInt();

}

// Runs the event
// This is what you need to override to implement your application
// To maintain causality run should only interact with dynamic data by using the privided methods:
// get(id), getMutable(), addEvent, createObject, deleteObject, and getCollisions
void MoveSimpleSolid::run(){
    weak_ptr<TObject> ow = getMutable() ;
    if(auto og = ow.lock()){
        shared_ptr<ConvexSolid> self = std::static_pointer_cast<ConvexSolid>(og);
        bool moving = false;
        if(self->moveable){
            float speed = glm::length(self->velocity);
            if(speed > 0){
                moving = true;
                float new_speed = fmin(MoveSimpleSolid::max_speed,fmax(0,speed-MoveSimpleSolid::friction*interval));
                self->velocity *= new_speed/speed;
            }

            speed = glm::length(self->angular_velocity);
            if(speed > 0){
                moving = true;
                float new_speed = fmin(MoveSimpleSolid::max_angular_speed, fmax(0,speed-MoveSimpleSolid::angular_friction*interval));
                self->angular_velocity *= new_speed/speed;
            }
        }

        if(moving){
            self->move(interval);
            //clamp to local area for demo
            if(glm::length(self->position)>7){
                self->velocity = self->velocity*0.1f ;
                self->position = self->velocity + vec3(0,0,1.75) ;
            }
        }
        
        self->status = 0 ;
        weak_ptr<TObject> sw = get(self->shape_id) ; 
        if(auto sg = sw.lock()){ // if has a shape
            shared_ptr<ConvexShape> shape = std::static_pointer_cast<ConvexShape>(sg);
            self->radius = shape->radius; // keep radius up to date with shape
            if(moving || self->world_plane.size() == 0){
                self->computeWorldPlanes(shape);
            }
            vector<int> collisions = getCollisions();
            bool computed_inertia = false;
            for(int j = 0 ;j < collisions.size();j++){
            if(collisions[j] < anchor_id){ // only handle each collision once
                weak_ptr<TObject> cw = get(collisions[j]) ;
                if(auto cg = cw.lock()){
                    if(cg->type == 2 ) { //other is also a solid
                        shared_ptr<ConvexSolid> other = std::static_pointer_cast<ConvexSolid>(cg);
                        // Compute other's world planes if required (and it might be since they aren't serialized)
                        //if(other->world_vertex.size() == 0){
                            weak_ptr<TObject> qsw = get(other->shape_id) ; 

                            if(auto qsg = qsw.lock()){ // if has a shape
                                shared_ptr<ConvexShape> other_shape = std::static_pointer_cast<ConvexShape>(qsg);
                                if(other->world_plane.size() != 0){
                                    vector<vec3> collision = self->checkCollision(other) ;
                                    if(collision.size()>0){
                                        if(!computed_inertia){
                                            self->computeInertia(shape);
                                            computed_inertia = true;
                                        }
                                        other->computeInertia(other_shape); // TODO don't modify other shapes (even cache) when not anchored
                                        vec3& move = collision[0] ;
                                        vec3& point = collision[1] ;
                                        vec3& normal = collision[2] ;
                                        vec3 impulse = self->getCollisionImpulse(other, point,normal, 1.0);
                                        self->position += move*0.5f;
                                        self->applyImpulse(impulse, point);
                                        addEvent(std::make_unique<ApplySolidImpulse>(collisions[j], impulse*-1.0f, point, move*-0.5f));

                                        self->computeWorldPlanes(shape);
                                    }
                                }

                            }else{
                                printf("Error: solid without shape!\n");
                            }
                        //}

                        
                    }
                }
            }}
        }else{
            printf("Error: solid without shape!\n");
        }

        

    }

    std::unique_ptr<MoveSimpleSolid> next_tick = std::make_unique<MoveSimpleSolid>(anchor_id, interval);
    next_tick->time = time + interval ;
    addEvent(std::move(next_tick));
}