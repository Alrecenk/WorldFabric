#include "CollisionSystem.h"

#include "Timeline.h"
#include "TObject.h"
#include "TEvent.h"
#include "KDLeaf.h"

#include <memory>
#include <algorithm>

using glm::vec3 ;
using std::vector;
using std::weak_ptr ;
using std::unique_ptr ;
using std::set ;


CollisionSystem::CollisionSystem(){
    root = unique_ptr<KDNode>(new KDLeaf());
}

// Returns the collision indices for a given event
// Logs the result to rollback if needed
vector<int> CollisionSystem::getCollisions(TEvent* event){
    //printf("requesting collisions!\n");
    vector<int> collisions ;
    weak_ptr<TObject> vow = timeline->getObjectInstant(event->anchor_id, event->time);

    if(auto vo = vow.lock()){
        vec3 vantage = vo->position;
        set<int> candidates = getCandidates(vantage, vo->radius);
        for(int id : candidates){
            if(id != event->anchor_id){
                weak_ptr<TObject> ow = timeline->getObjectInstant(vantage, id, event->time);
                if(auto o = ow.lock()){
                    vec3 diff = (o->position-vantage) ;
                    if(glm::dot(diff,diff) < (o->radius + vo->radius) * (o->radius + vo->radius)){
                        collisions.push_back(id);
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

// Must be called when an event writes its anchor object
// to potentially rollback events with changed collision results
void CollisionSystem::onDataChanged(TEvent* event){

    weak_ptr<TObject> eo = timeline->getObjectInstant(event->anchor_id, event->time);
    if(auto e = eo.lock()){
       addObject(event->anchor_id, e->position, e->radius, event->time);
    }

    if(event->time >= most_recent_request_time){ // if this event is not before any checks
        return ; // it can't cause collision roll back, we don't need to check
    }
    //printf("Event time: %f request_time: %f\n", event->time, most_recent_request_time);
    vector<TEvent*> to_rerun ;
    for(auto& [caller, collisions] : requests){
        // only rollback stuff that ran after this change
        if(caller->time > event->time && caller->anchor_id != event->anchor_id && !caller->disabled && caller->has_run){ 
            weak_ptr<TObject> vow = timeline->getObjectInstant(caller->anchor_id, caller->time); // the original caller is the vantage point
            if(auto vo = vow.lock()){
                vec3 vantage = vo->position ;
                weak_ptr<TObject> ow = timeline->getObjectInstant(vantage, event->anchor_id, caller->time); // ge the event data from the caller's perspective
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


//Adds an object to the collision structure for generating candidates
void CollisionSystem::addObject(int id, glm::vec3 center, float radius, double time){
    if(last_edit_time.find(id) == last_edit_time.end()){
        last_edit_time[id] = time ;
    }else{
        last_edit_time[id] = fmax(last_edit_time[id], time) ;
    }
    KDNode::BoundingSphere bound = {id, center, radius, time} ;
    KDNode* new_root = root->add(bound);
    if(new_root != root.get()){
        root =  unique_ptr<KDNode>(new_root);
    }
}

// Clean up entries in the collision structure that are older than clear_time and no longer present
void CollisionSystem::clearHistory(double clear_time){
    KDNode* new_root = root->clearHistory(clear_time, last_edit_time);
    if(new_root != root.get()){
        root =  unique_ptr<KDNode>(new_root);
    }
}

std::set<int> CollisionSystem::getCandidates(glm::vec3 x, float radius){
    KDNode::BoundingSphere bound = {-1, x, radius, 0.0} ;
    return root->getCollisionCandidates(bound);
}