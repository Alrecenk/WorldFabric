#include "Timeline.h"

#include "CreateObject.h"
#include "TObject.h"
#include "TEvent.h"

using glm::vec3;
using std::vector;
using std::map;
using std::string ;


Timeline::Timeline(){
    events.timeline = this;
}

// Set the functions to be used for generating typed timeline events and objects from serialized data
void Timeline::setGenerators(std::unique_ptr<TEvent> (*event_generator)(const Variant& serialized), 
                            std::unique_ptr<TObject>(*object_generator)(const Variant& serialized)){
    TEvent::generateTypedTEvent = event_generator;
    TObject::generateTypedTObject = object_generator ;
}

// Adds an event to this timeline
// Peforms rollback and correction as required
void Timeline::addEvent(std::unique_ptr<TEvent> e, double send_time){
    e->time = send_time ;
    TObject* vo = objects[vantage_id].get(send_time) ;
    TObject* eo = objects[e->anchor_id].get(send_time) ;

    if(vo!= nullptr && eo !=nullptr){ // if position data available
        // delay event creation by time warp effect
        e->time = send_time + glm::length(vo->position - eo->position)/info_speed ;
    }
    e->timeline = this;
    pending_external_events.push_back(events.addEvent(std::move(e)));
}

// Creates an event that creates an object at the earliest possible time
void Timeline::createObject(std::unique_ptr<TObject> obj, std::unique_ptr<TEvent> on_created, double send_time){
    addEvent(std::make_unique<CreateObject>(std::move(obj), std::move(on_created)), send_time);
}

    // Creates an event that deletes an object at the earliest possible time
void Timeline::deleteObject(int id, double send_time){
    //TODO
}

// Runs events in the timeline until the location at the vantage object reaches the given time
void Timeline::run(double new_time){
    printf("timeline running......\n");

    TObject* vo = objects[vantage_id].get(new_time) ;
    vec3 vantage(0,0,0) ;
    if(vo!= nullptr){ // if position data available
        vantage = vo->position ;
    }

    TEvent* current_event = events.next(vantage, new_time, info_speed) ;
    while(current_event != nullptr){
        current_event->run();
        current_event->run_pending = false;
        if(current_event->anchor_id == vantage_id && current_event->wrote_anchor){ // if vantage object changed
            TObject* eo = objects[current_event->anchor_id].get(new_time) ;
            if(eo!=nullptr){
                vantage = eo->position; // vantage point may have changed
            }
        }
        current_event = events.next(vantage, new_time, info_speed) ;
    }
    current_time = new_time ;
}

// Returns a serialized descriptor of the state of the this Timeline that can be used by another Timeline ot generate a synchronization update
Variant Timeline::getDescriptor(){
    //TODO
    return Variant();
}

// Given another tree's descriptor, produces an update that woulds bring that tree into syncwith this one
Variant Timeline::getUpdateFor(Variant descriptor){
    //TODO
    return Variant();
}

// applies a syncrhoniation update produced by another timeline's use of getUpdateFor'
void Timeline::applyUpdate(Variant update){
    //TODO
}

// Updates all observables to the current time, performing interpolation as required
// and returnsa list of ID for all observables
std::vector<int> Timeline::updateObservables(){

    TObject* vo = objects[vantage_id].get(current_time) ;
    vec3 vantage(0,0,0) ;
    if(vo!= nullptr){ // if position data available
        vantage = vo->position ;
    }

    vector<int> observed_ids;
    for(auto& [id, object_history] : objects){
        TObject* read = object_history.get(vantage,current_time, info_speed);
        if(read != nullptr){
            last_observed[id] = std::move(read->getObserved(last_observed[id].get()));
            observed_ids.push_back(id);
        }
    }
    return observed_ids ;
}

// Returns a reference to the last observed value of a given ID
const TObject* Timeline::getLastObserved(int id){
    return last_observed[id].get();
}

// returns the next valid ID that should be used for a created object
int Timeline::getNextID(){
    int max_id=0 ;
    for(auto& [id, object_history] : objects){ // TODO find a way to do this less likely to create remote conflicts
        max_id = std::max(max_id, id);
    }
    return max_id + 1 ;
}