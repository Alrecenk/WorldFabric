#include "Timeline.h"

#include "CreateObject.h"
#include "TObject.h"
#include "TEvent.h"

#include <unordered_map>
#include <unordered_set>

using glm::vec3;
using std::vector;
using std::map;
using std::string ;
using std::unordered_map;
using std::unordered_set;
using std::unique_ptr;


Timeline::Timeline(){
    events.timeline = this;
    collisions.timeline = this ;
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
    pending_external_events.push_back(events.addEvent(std::move(e)));
}

// Creates an event that creates an object at the earliest possible time
void Timeline::createObject(std::unique_ptr<TObject> obj, std::unique_ptr<TEvent> on_created, double send_time){
    std::unique_ptr<CreateObject> create_object_event = std::make_unique<CreateObject>(std::move(obj), std::move(on_created)) ;
    create_object_event->anchor_id = vantage_id ;
    addEvent(std::move(create_object_event), send_time);
}

    // Creates an event that deletes an object at the earliest possible time
void Timeline::deleteObject(int id, double send_time){
    //TODO
}

// Runs events in the timeline until the location at the vantage object reaches the given time
void Timeline::run(double new_time){
    //printf("timeline running......\n");

    TObject* vo = objects[vantage_id].get(new_time) ;
    vec3 vantage(0,0,0) ;
    if(vo!= nullptr){ // if position data available
        vantage = vo->position ;
    }
    /*
    TEvent* current_event = events.next(vantage, new_time, info_speed) ;
    while(current_event != nullptr){
        current_event->run();
        current_event->run_pending = false;
        if(current_event->wrote_anchor){
            collisions.onDataChanged(current_event); // trigger rollback from potential retroactive changes to collision requests
            if(current_event->anchor_id == vantage_id ){ // if vantage object changed
                TObject* eo = objects[current_event->anchor_id].get(new_time) ;
                if(eo!=nullptr){
                    vantage = eo->position; // vantage point may have changed
                }
            }
        }
        current_event = events.next(vantage, new_time, info_speed) ;
    }
    */

    map<double, TEvent*> events_to_run = events.allNext(vantage, new_time, info_speed) ;
    //printf("events to run: %d\n", (int)events_to_run.size());
    //double last_time = 0 ;
    while(events_to_run.size()>0){
        double max_time = new_time ;
        for(auto& [event_run_time,current_event] : events_to_run){
            if(current_event->time > max_time){ // this event occurs after an event spawned during this batch
                break;
            }
            /*
            if(last_time > current_event->time){
                printf(" %f Event ran out of order! %f > %f\n", event_run_time, last_time, current_event->time);
            }
            last_time = current_event->time ; 
            */
            //printf("even time:%f\n", event_run_time);
            current_event->run();
            current_event->run_pending = false;
            if(current_event->wrote_anchor){
                collisions.onDataChanged(current_event);
                if(current_event->anchor_id == vantage_id){ // if vantage object changed
                    TObject* eo = objects[current_event->anchor_id].get(new_time) ;
                    if(eo!=nullptr){ // vantaghe object actually exists
                        if(eo->position != vantage){ // vantage point changed
                            vantage = eo->position; // vantage point may have changed
                            //events_to_run = events.allNext(vantage, new_time, info_speed) ; // recompute order with new vantage point
                            break;
                        }
                    }else{
                        printf("WTF: vantage object edited duringrun but doesn't exist! Maybe it's moving too fast?n");
                    }
                }
            }
            for(TEvent* s : current_event->spawned_events){
                max_time = fmin(max_time,  s->time);
            }
        }
        events_to_run = events.allNext(vantage, new_time, info_speed) ;
    }

    current_time = new_time ;
}

// Clears out all events and data changes before the given time
// Objects may have a single instant before the clear time, so their value at that time can be fetched
void Timeline::clearHistoryBefore(double clear_time){
    clear_time = fmin(clear_time, current_time); // don't allow clearing beyond the current time
    events.clearHistoryBefore(clear_time);
    for(auto& [id, history] : objects){
        history.clearHistoryBefore(clear_time);
    }
}

// Return the minimum state required to generate a matching timeline from the given time
std::pair<std::vector<TEvent*>, std::map<int, TObject*>> Timeline::getBaseState(double time){
    std::pair<std::vector<TEvent*>, std::map<int, TObject*>> state ;
    state.first = events.getBase(time) ;
    for(auto& [id, history] : objects){
        TObject* bo = history.get(time);
        if(bo!=nullptr){
            state.second[id] = bo ;
        }
    }
    return state ;
}

// Returns a serialized descriptor of the state of the this Timeline that can be used by another Timeline ot generate a synchronization update
Variant Timeline::getDescriptor(double time){
    auto [base_events,base_objects] = getBaseState(time);
    int* base_event_hashes  = (int*)malloc(4 * base_events.size());
    for(int k=0;k<base_events.size();k++){
        base_event_hashes[k] = Variant(base_events[k]->serialize()).hash() ;
    }
    int* base_object_hashes  = (int*)malloc(4 * 2 * base_objects.size());
    int k=0;
    for(auto& [id,object] : base_objects){
        base_object_hashes[k*2] = id;
        base_object_hashes[k*2+1] = Variant(object->serialize()).hash();
    }
    
    map<string,Variant> descriptor_map ;
    descriptor_map["time"] = Variant(time);
    descriptor_map["events"] = Variant(base_event_hashes, base_events.size() );
    descriptor_map["objects"] = Variant(base_object_hashes, 2 * base_objects.size());
    free(base_event_hashes);
    free(base_object_hashes); // TODO build these Variants without using exposed pointers so manaul free isn't required
    return Variant(descriptor_map);
}

// Given another tree's descriptor, produces an update that woulds bring that tree into syncwith this one
Variant Timeline::getUpdateFor(const Variant& descriptor){
    map<string,Variant> descriptor_map  = descriptor.getObject();
    double time = descriptor_map["time"].getDouble();
    int* other_events = descriptor_map["events"].getIntArray();
    int num_other_events = descriptor_map["events"].getArrayLength();
    unordered_set<int> other_event_set ;
    for(int k=0;k<num_other_events;k++){
        other_event_set.insert(other_events[k]);
    }    
    int* other_objects = descriptor_map["objects"].getIntArray();
    int num_other_objects = descriptor_map["objects"].getArrayLength()/2;
    unordered_map<int,int> other_object_map;
    for(int k=0;k<num_other_objects ;k++){
        other_object_map[other_objects[2*k]] = other_objects[2*k+1];
    }
    descriptor_map.clear();

    auto [base_events,base_objects] = getBaseState(time);
    vector<Variant> event_updates;
    for(int k=0;k<base_events.size();k++){
        TEvent* event = base_events[k] ;
        Variant serial = Variant(event->serialize());
        int hash = serial.hash();
        if(other_event_set.find(hash) == other_event_set.end()){ // we have event other didn't have
            event_updates.emplace_back(std::move(serial)); 
        }
    }

    map<int,Variant> object_updates ;
    int k=0;
    for(auto& [id,object] : base_objects){
        Variant serial = Variant(object->serialize()) ;
        int hash = serial.hash();
        if(other_object_map[id] != hash){
            object_updates[id] = std::move(serial) ;
        }

    }

    map<string,Variant> update_map ;
    update_map["time"] = Variant(time);
    update_map["events"] = Variant(event_updates);
    update_map["objects"] = Variant(object_updates);

    return Variant(update_map);
}

// applies a syncrhoniation update produced by another timeline's use of getUpdateFor'
void Timeline::applyUpdate(const Variant& update){
    map<string,Variant> update_map = update.getObject();
    double time = update_map["time"].getDouble();
    
    map<int,Variant> object_updates = update_map["objects"].getIntObject();
    for(auto& [id,serial] : object_updates){
        if(objects.find(id) == objects.end()){ // if not present
            //printf("update creating new baseobject!\n");
            //serial.printFormatted();
            unique_ptr<TObject> new_obj = TObject::generateTypedTObject(serial); // infer type and build using generator
            objects[id] = ObjectHistory(std::move(new_obj), time); // place into timeline
        }else{ // if already present but nonmatching value
            //printf("update updating base object!\n");
            TObject* existing_obj = objects[id].getMutable(time); // write using functionality that triggers rollback
            map<string,Variant> serial_map = serial.getObject() ;
            existing_obj->set(serial_map);
        }
    }

    vector<Variant> event_updates = update_map["events"].getVariantArray();
    for(int k=0;k<event_updates.size();k++){
        events.addEvent(std::move(TEvent::generateTypedTEvent(event_updates[k])));
    }
}

// Updates all observables to the current time, performing interpolation as required
// and returnsa list of ID for all observables
std::vector<int> Timeline::updateObservables(){
    TObject* vo = objects[vantage_id].get(current_time) ;
    vector<int> observed_ids;
    for(auto& [id, object_history] : objects){
        TObject* read = nullptr ;
        if(vo != nullptr){
            read = object_history.get(vo->position,current_time, info_speed);
        }else{
            read = object_history.get(current_time);
        }
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