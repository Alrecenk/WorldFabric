#include "Timeline.h"

using glm::vec3;
using std::vector;
using std::map;
using std::string ;

// Set the functions to be used for generating typed timeline events and objects from serialized data
void Timeline::setGenerators(TEvent(*event_generator)(Variant& serialized), TObject(*object_generator)(Variant& serialized)){
    TEvent::generateTypedTEvent = event_generator;
    TObject::generateTypedTObject = object_generator ;
}

// Adds an event to this timeline
// Peforms rollback and correction as required
void Timeline::addEvent(TEvent e, double send_time){
    e.time = send_time ;
    vec3 vantage = objects[vantage_id].get(send_time).position; // TODO cache
    vec3 new_loc = objects[vantage_id].get(send_time) ; //TODO is this sufficient or do we need to iterate to consider movement away?
    e.time == send_time + glm::length(vantage-new_loc)/info_speed ;
    pending_external_events.push_back(events.addEvent(e));
}

// Creates an event that creates an object at the earliest possible time
void Timeline::createObject(TObject obj, TEvent on_created, double send_time){
    addEvent(CreateObject(obj, on_created), send_time));
}

    // Creates an event that deletes an object at the earliest possible time
void Timeline::deleteObject(TObject obj, double send_time)){
    //TODO
}

// Runs events in the timeline until the location at the vantage object reaches the given time
void Timeline::run(double new_time){
    vec3 vantage = objects[vantage_id].get(new_time);
    TEvent* current_event = events.next(vantage, new_time, info_speed) ;
    while(current_event != nullptr){
        current_event->run();
        if(current_event->anchor_id == vantage_id && current_event->wrote_anchor){ // if vantage object changed
            vantage = objects[vantage_id].get(new_time)->position; // vantage point may have changed
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
    vec3 vantage = objects[vantage_id].get(new_time);
    vector<int> observed_ids;
    for(auto& [id, object_history] : history){
        TObject* read = object_history.get(vantage,current_time, info_speed);
        if(read != nullptr){
            last_observed[id] = read->getObserved(last_observed[id]);
            observed_ids.push_back(id);
        }
    }
    return observed_ids ;
}

// Returns a reference to the last observed value of a given ID
const &TObject Timeline::getLastObserved(int id){
    return last_observed[id];
}

// returns the next valid ID that should be used for a created object
int Timeline::getNextID(){
    int max_id=0 ;
    for(auto& [id, object_history] : history){ // TODO find a way to do this less likely to create remote conflicts
        max_id = std::max(max_id, id);
    }
    return max_id + 1 ;
}