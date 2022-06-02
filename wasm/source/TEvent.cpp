#include "TEvent.h"
#include "CreateObject.h"
#include "glm/glm.hpp"

using glm::vec3 ;
using std::vector ;

// Returns the latest data for the given object available to this event
const TObject* TEvent::get(int id){
    if(timeline->objects.find(id) == timeline->objects.end()){
        return nullptr ;
    }
    vec3 vantage = timeline->objects[anchor_id].get(time)->position; // TODO cache
    TObject* obj = timeline->objects[id].get(vantage, time, timeline->info_speed) ;
    double read_time = time - glm::length(obj->position-vantage)/timeline->info_speed ;
    obj->readers.push_back(std::pair<TEvent*, double>(this, read_time));
    return obj;
}

// Returns a mutable version of the object this event is anchored to
// This is how you edit objects from inside events.
TObject* TEvent::getMutable(){
    if(timeline->objects.find(anchor_id) == timeline->objects.end()){
        return nullptr ;
    }
    TObject* obj = timeline->objects[anchor_id].getMutable(time) ;
    obj->readers.push_back(std::pair<TEvent*, double>(this, time));
    wrote_anchor = true;
    return obj;
}

// Adds an event to the Timeline this event is in
// If no time is set on the event it will be run at the earliest possible time
void TEvent::addEvent(TEvent e){
    e.time = fmax(e.time, time + timeline->min_spawned_event_delay);
    vec3 vantage = timeline->objects[anchor_id].get(time)->position; // TODO cache
    vec3 new_loc = timeline->objects[e.anchor_id].get(e.time)->position ; //TODO is this sufficient or do we need to iterate to consider movement away?
    double earliest_time = time + glm::length(vantage-new_loc)/timeline->info_speed ;
    e.time =  fmax(e.time, earliest_time);
    spawned_events.push_back(timeline->events.addEvent(e));
}

// Creates an event that creates an object at the earliest possible time
void TEvent::createObject(TObject obj, TEvent on_create){
    addEvent(CreateObject(obj, on_create));
}

    // Creates an event that deletes an object at the earliest possible time
void TEvent::deleteObject(TObject obj){
    printf("Delete object is not implemented yet!\n");
}

// Returns the IDs of all TObjects colliding with the bounding sphere of the anchor object
// at the time of this event
std::vector<int> TEvent::getCollisions(){
    printf("Get collisions is not implemented yet!\n");
    return vector<int>();
}