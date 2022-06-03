#include "TEvent.h"
#include "CreateObject.h"
#include "glm/glm.hpp"

using glm::vec3 ;
using std::vector ;

std::unique_ptr<TEvent>(*TEvent::generateTypedTEvent)(const Variant& serialized) ; 


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
void TEvent::addEvent(std::unique_ptr<TEvent> e){
    e->time = fmax(e->time, time + timeline->min_spawned_event_delay); // enforce minimum delay
    TObject* vo = timeline->objects[anchor_id].get(e->time) ;
    TObject* eo = timeline->objects[e->anchor_id].get(e->time) ;
    if(vo!= nullptr && eo !=nullptr){ // if position data available
        // delay event creation by time warp effect
        e->time = fmax(e->time, time + glm::length(vo->position - eo->position)/timeline->info_speed) ;
    }
    spawned_events.push_back(timeline->events.addEvent(std::move(e)));
}

// Creates an event that creates an object at the earliest possible time
void TEvent::createObject(std::unique_ptr<TObject> obj, std::unique_ptr<TEvent> on_created){
    addEvent(std::make_unique<CreateObject>(std::move(obj), std::move(on_created)));
}

    // Creates an event that deletes an object at the earliest possible time
void TEvent::deleteObject(int id){
    printf("Delete object is not implemented yet!\n");
}

// Returns the IDs of all TObjects colliding with the bounding sphere of the anchor object
// at the time of this event
std::vector<int> TEvent::getCollisions(){
    printf("Get collisions is not implemented yet!\n");
    return vector<int>();
}