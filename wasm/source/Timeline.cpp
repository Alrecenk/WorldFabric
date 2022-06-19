#include "Timeline.h"

#include "CreateObject.h"
#include "TObject.h"
#include "TEvent.h"

#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <ctime>

using glm::vec3;
using std::vector;
using std::map;
using std::string ;
using std::unordered_map;
using std::unordered_set;
using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr ;


// Set the functions to be used for generating typed timeline events and objects from serialized data
Timeline::Timeline(std::unique_ptr<TEvent> (*event_generator)(const Variant& serialized), 
                            std::unique_ptr<TObject>(*object_generator)(const Variant& serialized)){
    last_run_time = timeMilliseconds();
    TEvent::generateTypedTEvent = event_generator;
    TObject::generateTypedTObject = object_generator ;
}


// Adds an event to this timeline
// Peforms rollback and correction as required
void Timeline::addEvent(std::unique_ptr<TEvent> event, double send_time){
    event->time = send_time ;
    weak_ptr<TObject> vo = getObjectInstant(vantage_id, send_time) ;
    weak_ptr<TObject> eo = getObjectInstant(event->anchor_id, send_time) ;

    if(auto vo2 = vo.lock()){
    if(auto eo2 = eo.lock()){
        // if position data available
        // delay event creation by time warp effect
        event->time = send_time + glm::length(vo2->position - eo2->position)/info_speed ;
    }}
    insertEvent(std::move(event));
    //pending_external_events.push_back(event->weak_this);
}

std::weak_ptr<TEvent> Timeline::insertEvent(std::unique_ptr<TEvent> event){
    event->timeline = this ;
    
    // Put it in the slot of a delted item if posible
    for(int k=0;k<events.size();k++){
        int p = (k + event_add_pointer)%events.size();
        if(events[p].get() == nullptr || (events[p]->disabled && !events[p]->has_run)){
            events[p] = std::move(event) ;
            event_add_pointer = p+1; // next time start checking from the slot after the one we just filled
            events[p]->weak_this = events[p];
            return events[p];
        }
    }
    // No free slots, push on the end
    events.push_back(std::move(event));
    events[events.size()-1]->weak_this = events[events.size()-1];
    return events[events.size()-1];
}

// Creates an event that creates an object at the earliest possible time
void Timeline::createObject(std::unique_ptr<TObject> obj, std::unique_ptr<TEvent> on_created, double send_time){
    std::unique_ptr<CreateObject> create_object_event = std::make_unique<CreateObject>(std::move(obj), std::move(on_created)) ;
    create_object_event->anchor_id = vantage_id ;
    addEvent(std::move(create_object_event), send_time);
}

    // Creates an event that deletes an object at the earliest possible time
void Timeline::deleteObject(int id, double send_time){
    printf("deleting IDs is not supported yet!\n");
}

std::weak_ptr<TEvent> Timeline::nextEventToRun(double time, double info_speed){
    double best_time = time;
    std::shared_ptr<TEvent> best_event;
    for(std::shared_ptr<TEvent>& e : events){
        if(e.get() != nullptr && !e->disabled && !e->has_run){
            double time_to_run = e->time ;
            if(time_to_run <= best_time){
                best_time = time_to_run;
                best_event = e ;
            }
        }
    }
    return best_event ;
}

std::weak_ptr<TEvent> Timeline::nextEventToRun(glm::vec3 vantage, double time, double info_speed){
    double best_time = time;
    std::shared_ptr<TEvent> best_event;
    for(std::shared_ptr<TEvent>& e : events){
        if(e.get() != nullptr && !e->disabled && !e->has_run){
            weak_ptr<TObject> eo = getObjectInstant(e->anchor_id, e->time) ;
            double time_to_run = e->time ;
            if(auto eo2 = eo.lock()){
                double dist = glm::length(vantage- eo2->position);
                time_to_run = e->time + dist/info_speed;
            }
            if(time_to_run <= best_time){
                best_time = time_to_run;
                best_event = e ;
            }
        }
    }
    return best_event ;
}

// Runs events in the timeline until the location at the vantage object reaches the given time
void Timeline::run(double new_time){
    lock.lock();
    last_run_time = timeMilliseconds();

    weak_ptr<TObject> vo = getObjectInstant(vantage_id, new_time) ;
    vec3 vantage(0,0,0) ;
    bool has_vantage = false;
    if(auto vo2 = vo.lock()){ // if position data available
        vantage = vo2->position ;
        has_vantage= true;
    }
    
    std::weak_ptr<TEvent>  current_event = nextEventToRun(vantage, new_time, info_speed) ;
    while(auto ce = current_event.lock()){
        ce->run();
        ce->has_run = true;
        if(ce->wrote_anchor){
            if(has_vantage && ce->anchor_id == vantage_id ){ // if vantage object changed
                weak_ptr<TObject> eo = getObjectInstant(ce->anchor_id, new_time) ;
                if(auto eo2 = eo.lock()){
                    vantage = eo2->position; // vantage point may have changed
                }
            }
        }
        if(has_vantage){
            current_event = nextEventToRun(vantage, new_time, info_speed) ;
        }else{
            current_event = nextEventToRun(new_time, info_speed) ;
        }
    }
    current_time = new_time ;
    lock.unlock();
}

// Runs the current timeline based on the real-time passed since last clal to either run function
void Timeline::run(){
    long run_time = timeMilliseconds();
    double new_time = current_time + (run_time - last_run_time)/1000.0 ;
    /*if((int)current_time != (int)new_time){
        printf("Run() time : %f\n", new_time);
    }*/
    run(new_time);
    last_run_time = run_time ;
}

std::weak_ptr<TObject> Timeline::getObjectInstant(int id, double time){
    // objecvt has never been written
    if(objects.find(id) == objects.end()){
        return std::weak_ptr<TObject>();
    }
    shared_ptr<TObject> instant = objects[id];
    while(instant->write_time > time && instant->prev){
        //instant->print();
        instant = instant->prev ;
    }
    //reached oldest object and it was too new to be read
    if(instant->write_time > time){
        return std::weak_ptr<TObject>();
    }
    //instant->print();
    return instant ;

}

// Returns the value of an object at the given time deleyed by time warp from the given vantage point
std::weak_ptr<TObject> Timeline::getObjectInstant(const glm::vec3& vantage, int id, double time){
// objecvt has never been written
    if(objects.find(id) == objects.end()){
        return std::weak_ptr<TObject>();
    }

    shared_ptr<TObject> instant = objects[id];
    double available_time = instant->write_time + glm::length(instant->position-vantage)/info_speed ;
    while(available_time > time && instant->prev){
        instant = instant->prev ;
        available_time = instant->write_time + glm::length(instant->position-vantage)/info_speed ;
    }
    //reached oldest object and it was too new to be read
    if(available_time > time){
        return std::weak_ptr<TObject>();
    }
    return instant ;
}

void Timeline::deleteAfter(int id, double time){
    // find the last instant we're going to keep
    shared_ptr<TObject> last_instant = objects[id];
    while(last_instant->write_time > time && last_instant->prev){
        last_instant = last_instant->prev ;
        // unrun all events that read data we're gonna wipe
        for(int k=0;k<last_instant->readers.size();k++){
            if(auto rk = last_instant->readers[k].first.lock()){
                if(rk->has_run && rk->time > time){ // have to check time because the last instant may only have some reruns
                    rk->unrun();
                    last_instant->readers[k].first.reset() ; // remove the link to the reader since it might not read this when it reruns
                }
            }
        }
    }
    objects[id] = last_instant ; //since backward links are the only shared_pt this should cause proper deletion
}

// Clears out all events and data changes before the given time
// Objects may have a single instant before the clear time, so their value at that time can be fetched
void Timeline::clearHistoryBefore(double clear_time){
    lock.lock();
    clear_time = fmin(clear_time, current_time);// don't allow clearing beyond the current time

    // clear out run events older than the time andany disabled events stil lingering
    for(int k=0;k<events.size();k++){
        if(events[k] && (events[k]->disabled || (events[k]->has_run && events[k]->time < clear_time))){
            events[k].reset();
        }
    }

    for(auto& [id, most_recent] : objects){
        // find the first instant written before the clear time
        shared_ptr<TObject> first_instant = most_recent;
        while(first_instant->write_time >= clear_time && first_instant->prev){
            first_instant = first_instant->prev ;
        }
        // delete everything before that instanrt
        first_instant->prev.reset();
    }
    
    last_clear_time = clear_time ;
    lock.unlock();
}

// Return the minimum state required to generate a matching timeline from the given time
std::pair<std::vector<std::weak_ptr<TEvent>>, std::map<int, std::weak_ptr<TObject>>> Timeline::getBaseState(double time){
//TODO
return std::pair<std::vector<std::weak_ptr<TEvent>>, std::map<int, std::weak_ptr<TObject>>>();
}

// Returns a serialized descriptor of the state of the this Timeline at the goiven time 
// that can be used by another Timeline to generate a synchronization update
Variant Timeline::getDescriptor(double time,bool server){
    //TODO
    return Variant();
}

// Given another tree's descriptor, produces an update that would bring that tree into sync with this one
Variant Timeline::getUpdateFor(const Variant& descriptor, bool server){
    //TODO
    return Variant();
}

// applies a syncrhoniation update produced by another timeline's use of getUpdateFor
// returns the time of the update
void Timeline::applyUpdate(const Variant& update, bool server){
    //TODO
}

// Given a packet with an update and optional descriptor
// applies the update, and if there was a descriptor returns an ypdate for it
// and a new descriptor of itself at current_time-base_age
std::map<std::string, Variant> Timeline::synchronize(std::map<std::string, Variant>& packet, bool server){
    //TODO
    return std::map<std::string, Variant>();
}

// Updates all observables to the current time, performing interpolation as required
// and returnsa list of ID for all observables
std::vector<int> Timeline::updateObservables(){

    weak_ptr<TObject> vo = getObjectInstant(vantage_id, current_time) ;
    vec3 vantage(0,0,0) ;
    bool has_vantage = false;
    if(auto vo2 = vo.lock()){ // if position data available
        vantage = vo2->position ;
        has_vantage= true;
    }

    vector<int> observed_ids;
    for(auto& [id, object_history] : objects){

        weak_ptr<TObject> eo = getObjectInstant(id, current_time) ;
        weak_ptr<TObject> read ;
        if(auto vo2 = vo.lock()){
            read = getObjectInstant(vo2->position, id, current_time) ;
        }else{
            read = getObjectInstant(id, current_time) ;
        }
        if(auto read2 = read.lock()){
            last_observed[id] = std::move(read2->getObserved(last_observed[id]));
            observed_ids.push_back(id);
        }
    }
    return observed_ids ;

}

// Returns a reference to the last observed value of a given ID
const std::shared_ptr<TObject> Timeline::getLastObserved(int id){
    if(last_observed.find(id) != last_observed.end()){
        return last_observed[id] ;
    }
    return std::shared_ptr<TObject>();
}

// returns the next valid ID that should be used for a created object
int Timeline::getNextID(){
    int max_id=0 ;
    for(auto& [id, object_history] : objects){ // TODO find a way to do this less likely to create remote conflicts
        max_id = std::max(max_id, id);
    }
    return max_id + 1 ;
}

long Timeline::timeMilliseconds() const{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}
