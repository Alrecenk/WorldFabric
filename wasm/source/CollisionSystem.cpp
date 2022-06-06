#include "CollisionSystem.h"

#include "ObjectHistory.h"
#include "Timeline.h"
#include "TObject.h"
#include "TEvent.h"

#include <memory>

using glm::vec3 ;
using std::vector;


// Returns the collision indices for a given event
// Logs the result to rollback if needed
vector<int> CollisionSystem::getCollisions(TEvent* event){
    //printf("requesting collisions!\n");
    vector<int> collisions ;
    TObject* vo = timeline->objects[event->anchor_id].get(event->time) ;
    if(vo == nullptr){
        printf("WTF: Requested collision data from an event with no object!\n");
        return collisions ;
    }
    vec3 vantage = vo->position;

    for(auto& [id, history] : timeline->objects){
        if(id != event->anchor_id){
            TObject* o = history.get(vantage, event->time, timeline->info_speed);
            if(o!= nullptr){
                vec3 diff = (o->position-vantage) ;
                if(glm::dot(diff,diff) < (o->radius + vo->radius) * (o->radius + vo->radius)){
                    collisions.push_back(id);
                    //printf("%d found collision : %d!\n",event->anchor_id, id);
                }
            }
        }
    }
    requests[event] = collisions ;
    return collisions ;
}

// Must be called when an event is deleted to potentially clear out its collision request history
void CollisionSystem::onDelete(TEvent* event){
    requests.erase(event);
}

// Must be called when an event writes its anchpor object
// to potentially rollback events with changed collision results
void CollisionSystem::onDataChanged(TEvent* event){
    for(auto& [caller, collisions] : requests){
        // only rollback stuff that ran after this change
        if(caller->time > event->time && caller->anchor_id != event->anchor_id && !caller->deleted && !caller->run_pending){ 
            TObject* vo = timeline->objects[caller->anchor_id].get(caller->time) ; // we're still fro mthe persepctive of the caller not the new data
            vec3 vantage = vo->position ;
            TObject* o = timeline->objects[event->anchor_id].get(vantage, caller->time, timeline->info_speed);
            vec3 diff = (o->position-vantage) ;
            bool now_collides = glm::dot(diff,diff) < (o->radius + vo->radius) * (o->radius + vo->radius) ;
            bool then_collided = false;
            for(auto& id : collisions){
                then_collided |= (id == event->anchor_id) ;
            }
            if(now_collides != then_collided){
                //printf("rolling back event due to collision change!\n");
                //caller->print();
                timeline->events.rerunEvent(caller);
            }
        }
    }
}