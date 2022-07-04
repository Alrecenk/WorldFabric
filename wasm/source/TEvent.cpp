#include "TEvent.h"
#include "CreateObject.h"
#include "glm/glm.hpp"

using glm::vec3 ;
using std::vector ;
using std::weak_ptr;
using std::shared_ptr;
using std::unique_ptr;

std::unique_ptr<TEvent>(*TEvent::generateTypedTEvent)(const Variant& serialized) ; 


// Returns the latest data for the given object available to this event
const std::weak_ptr<TObject> TEvent::get(int id){
    weak_ptr<TObject> vo = timeline->getObjectInstant(anchor_id, time) ;
    vec3 vantage(0,0,0) ;
    if(auto vo2 = vo.lock()){ // if position data available
        vantage = vo2->position ;
    }

    weak_ptr<TObject> g = timeline->getObjectInstant(vantage, id, time) ;
    if(auto g2 = g.lock()){
        double read_time = time - fmin(timeline->max_time_warp,glm::length(g2->position-vantage)/timeline->info_speed) ;
        g2->readers[weak_this] = read_time ;
        read.push_back(g);
    }
    return g ;
}

// Returns a mutable version of the object this event is anchored to
// This is how you edit objects from inside events.
std::weak_ptr<TObject> TEvent::getMutable(){
    timeline->deleteAfter(anchor_id, time);
    weak_ptr<TObject> g = get(anchor_id);

    if(auto prev_instant = g.lock()){
        unique_ptr<TObject> new_instant = prev_instant->deepCopy();
        new_instant->prev = prev_instant ; // make the new shared pointer to previous so it doesnt deleted when we put new in objects
        timeline->objects[anchor_id] = std::move(new_instant);
        prev_instant->next = timeline->objects[anchor_id];
        timeline->objects[anchor_id]->timeline = timeline;
        timeline->objects[anchor_id]->write_time = time ;
        wrote_anchor = true;
        return timeline->objects[anchor_id] ;
    }
    return weak_ptr<TObject>() ; // tried to get mutable of a nonexisting item
}

// Adds an event to the Timeline this event is in
// If no time is set on the event it will be run at the earliest possible time
void TEvent::addEvent(std::unique_ptr<TEvent> event){
    event->time = fmax(event->time, time+timeline->min_spawned_event_delay) ; //enforce minimum spawn delay on internal events
    event->spawner = weak_this ;
    event->spawner_time = time ;
    std::weak_ptr<TEvent> e = timeline->insertEvent(std::move(event));
    spawned_events.push_back(e);
}

// Creates an event that creates an object at the earliest possible time
void TEvent::createObject(std::unique_ptr<TObject> obj, std::unique_ptr<TEvent> on_created){
    std::unique_ptr<CreateObject> create_object_event = std::make_unique<CreateObject>(std::move(obj), std::move(on_created)) ;
    create_object_event->anchor_id = anchor_id ;
    addEvent(std::move(create_object_event));
}

    // Creates an event that deletes an object at the earliest possible time
void TEvent::deleteObject(int id){
    printf("Delete object is not implemented yet!\n");
}

void TEvent::unrun(){
    timeline->total_unruns++;
    has_run = false;
    timeline->collisions.removeRequests(this);

    // Remove links for the data this event accessed on run (so we don't get rerun if that data changes)
    clearReaderPointers();

    // Delete any data following from this write
    if(wrote_anchor){
        timeline->deleteAfter(anchor_id, time);
        wrote_anchor = false;
    }
    // Delete any events this event spawned
    for(weak_ptr<TEvent> s : spawned_events){
        if(auto s2 = s.lock()){
            if(s2->has_run){// TODO s-> deleted can segfault?
                s2->unrun() ;
            }
            s2->disabled = true;
        }
    }
    spawned_events.clear();
    timeline->recent_unruns.push_back(weak_this);
}

// Returns the IDs of all TObjects colliding with the bounding sphere of the anchor object
// at the time of this event
std::vector<int> TEvent::getCollisions(){
    //return std::vector<int>();
    return timeline->collisions.getCollisions(this);
}

std::unique_ptr<TEvent> TEvent::deepCopy(){
    return TEvent::generateTypedTEvent(Variant(serialize()));
}

void TEvent::print() const{
    Variant(serialize()).printFormatted();
}

// clears all pointers to this event on objects it read from
void TEvent::clearReaderPointers(){
    for(int k=0;k<read.size();k++){
        if(auto rk = read[k].lock()){
            rk->readers.erase(weak_this);
        }
    }
    read.clear();
}