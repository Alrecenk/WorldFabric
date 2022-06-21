#include "CollisionSystem.h"

#include "Timeline.h"
#include "TObject.h"
#include "TEvent.h"

#include <memory>
#include <algorithm>

using glm::vec3 ;
using std::vector;
using std::weak_ptr ;


// Returns the collision indices for a given event
// Logs the result to rollback if needed
vector<int> CollisionSystem::getCollisions(TEvent* event){
    //printf("requesting collisions!\n");
    vector<int> collisions ;
    //TObject* vo = timeline->objects[event->anchor_id].get(event->time) ;
    weak_ptr<TObject> vow = timeline->getObjectInstant(event->anchor_id, event->time);

    if(auto vo = vow.lock()){
        vec3 vantage = vo->position;
        for(auto& [id, history] : timeline->objects){
            if(id != event->anchor_id){
                //TObject* o = history.get(vantage, event->time, timeline->info_speed);
                weak_ptr<TObject> ow = timeline->getObjectInstant(vantage, id, event->time);
                if(auto o = ow.lock()){
                    vec3 diff = (o->position-vantage) ;
                    if(glm::dot(diff,diff) < (o->radius + vo->radius) * (o->radius + vo->radius)){
                        collisions.push_back(id);
                        //printf("%d found collision : %d!\n",event->anchor_id, id);
                    }
                }
            }
        }
        requests[event] = collisions ;
        most_recent_request_time = fmax(most_recent_request_time,event->time);
    }
    // client andf server objects may not be in same order in map, so we sort to guarantee events get identical list
    std::sort(collisions.begin(), collisions.end());
    return collisions ;
}

// Must be called when an event is deleted to potentially clear out its collision request history
void CollisionSystem::removeRequests(TEvent* event){
        requests.erase(event);
}

// Must be called when an event writes its anchpor object
// to potentially rollback events with changed collision results
void CollisionSystem::onDataChanged(TEvent* event){
    if(event->time >= most_recent_request_time){ // if this event is not before any checks
        return ; // it can't cause collision roll back, we don't need to check
    }
    //printf("Event time: %f request_time: %f\n", event->time, most_recent_request_time);
    vector<TEvent*> to_rerun ;
    for(auto& [caller, collisions] : requests){
        // only rollback stuff that ran after this change
        if(caller->time > event->time && caller->anchor_id != event->anchor_id && !caller->disabled && caller->has_run){ 
            //printf("caller time: %f event time: %f\n", caller->time, event->time);
            //TObject* vo = timeline->objects[caller->anchor_id].get(caller->time) ; // we're still fro mthe persepctive of the caller not the new data
            weak_ptr<TObject> vow = timeline->getObjectInstant(event->anchor_id, event->time);
            if(auto vo = vow.lock()){
                vec3 vantage = vo->position ;
                weak_ptr<TObject> ow = timeline->getObjectInstant(vantage, event->time, timeline->info_speed);
                if(auto o = ow.lock()){
                    vec3 diff = (o->position-vantage) ;
                    bool now_collides = glm::dot(diff,diff) < (o->radius + vo->radius) * (o->radius + vo->radius) ;
                    bool then_collided = false;
                    for(auto& id : collisions){
                        then_collided |= (id == event->anchor_id) ;
                    }
                    if(now_collides != then_collided){
                        //printf("rolling back event due to collision change!\n");
                        //caller->print();
                        to_rerun.push_back(caller);
                        //caller->unrun();
                    }
                }
            }
        }
    }
    
    for( TEvent* r : to_rerun){
        if(!r->disabled && r->has_run){
            timeline->collisions.removeRequests(r); // aggressive removal prevents recursive unruns from slowing down on this function           
            r->unrun();
        }
    }
}