#include "MoveSimpleSolid.h"
#include "ConvexSolid.h"

using std::string ;
using std::map ;
using std::vector;
using glm::vec3;
using std::weak_ptr;
using std::shared_ptr;
using std::unique_ptr;


float MoveSimpleSolid::friction = 0.45 ;
float MoveSimpleSolid::angular_friction = 0.25 ;

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
        
        float speed = glm::length(self->velocity);
        if(speed > 0){
            moving = true;
            float new_speed = fmax(0,speed-MoveSimpleSolid::friction*interval);
            self->velocity *= new_speed/speed;
        }

        speed = glm::length(self->angular_velocity);
        if(speed > 0){
            moving = true;
            float new_speed = fmax(0,speed-MoveSimpleSolid::angular_friction*interval);
            self->angular_velocity *= new_speed/speed;
        }

        // Calculate radius if it isn't set yet
        if(self->radius == 0){ // TODO this should probably be done on initialization somehow
            weak_ptr<TObject> cw = get(self->shape_id) ; 
            if(auto cg = cw.lock()){
                shared_ptr<ConvexShape> shape = std::static_pointer_cast<ConvexShape>(cg);
                for(int k=0;k<shape->vertex.size();k++){
                    float r = glm::length(shape->vertex[k]);
                    if(r > self->radius){
                        self->radius = r ;
                    }
                }
            }
        }

        if(moving){
            self->move(interval);

            /*
            vector<int> collisions = getCollisions();
            for(int j = 0 ;j < collisions.size();j++){
                weak_ptr<TObject> cw = get(collisions[j]) ; 
                if(auto cg = cw.lock()){
                
                }
            }
            */

            //clamp to local area for demo
            if(glm::length(self->position)>7){
                self->position = vec3(0,0,0);
                self->velocity = vec3(0,0,0);
            }

        }
        

    }

    std::unique_ptr<MoveSimpleSolid> next_tick = std::make_unique<MoveSimpleSolid>(anchor_id, interval);
    next_tick->time = time + interval ;
    addEvent(std::move(next_tick));
}