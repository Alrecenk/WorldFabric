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
    collisions.timeline = this ;
}


// Adds an event to this timeline
// Peforms rollback and correction as required
void Timeline::addEvent(std::unique_ptr<TEvent> event, double send_time){
    lock.lock();
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
    lock.unlock();
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
    lock.lock();
    std::unique_ptr<CreateObject> create_object_event = std::make_unique<CreateObject>(std::move(obj), std::move(on_created)) ;
    create_object_event->anchor_id = vantage_id ;
    addEvent(std::move(create_object_event), send_time);
    lock.unlock();
}

    // Creates an event that deletes an object at the earliest possible time
void Timeline::deleteObject(int id, double send_time){
    printf("deleting IDs is not supported yet!\n");
}


TEvent* Timeline::nextEventToRun(glm::vec3 vantage, double time, double info_speed){
    double best_time = time;
    TEvent* best_event = nullptr;
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
                best_event = e.get() ;
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
    
    TEvent*  current_event = nextEventToRun(vantage, new_time, info_speed) ;
    int run_events = 0 ;
    while(current_event != nullptr){
        current_event->run();
        current_event->has_run = true;
        run_events++;
        if(current_event->wrote_anchor){
            collisions.onDataChanged(current_event);
            if(has_vantage && current_event->anchor_id == vantage_id ){ // if vantage object changed
                weak_ptr<TObject> eo = getObjectInstant(current_event->anchor_id, new_time) ;
                if(auto eo2 = eo.lock()){
                    vantage = eo2->position; // vantage point may have changed
                }
            }
        }

        if(auto_clear_history && current_event->time - last_clear_time > history_kept*2){
            current_time = fmax(current_time,current_event->time) ;
            clearHistoryBefore(current_time-history_kept);
        }

        current_event = nextEventToRun(vantage, new_time, info_speed) ;

    }
    current_time = new_time ;
    //printf("run  %d events to time: %f\n", run_events, new_time);
    lock.unlock();
}

// Runs the current timeline based on the real-time passed since last clal to either run function
void Timeline::run(){
    lock.lock();
    long run_time = timeMilliseconds();
    double new_time = current_time + (run_time - last_run_time)/1000.0 ;
    /*if((int)current_time != (int)new_time){
        printf("Run() time : %f\n", new_time);
    }*/
    run(new_time);
    last_run_time = run_time ;
    lock.unlock();
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

// creates a new mutable instance of the object at the given id at time
// rolls back reads after the given time if required
std::weak_ptr<TObject> Timeline::getMutable(int id, double time){
    deleteAfter(id, time);
    weak_ptr<TObject> g = getObjectInstant(id, time);

    if(auto prev_instant = g.lock()){
        unique_ptr<TObject> new_instant = prev_instant->deepCopy();
        new_instant->prev = prev_instant ; // make the new shared pointer to previous so it doesnt deleted when we put new in objects
        objects[id] = std::move(new_instant);
        prev_instant->next = objects[id];
        objects[id]->timeline = this;
        objects[id]->write_time = time ;
        return objects[id] ;
    }
    return weak_ptr<TObject>() ; // tried to get mutable of a nonexisting item
}

void Timeline::deleteAfter(int id, double time){
    if(objects.find(id) == objects.end()){
        return ;
    }
    // find the last instant we're going to keep
    shared_ptr<TObject> last_instant = objects[id];
    while(last_instant->write_time >= time && last_instant->prev){
       
        // unrun all events that read data we're gonna wipe
        for(int k=0;k<last_instant->readers.size();k++){
            if(auto rk = last_instant->readers[k].first.lock()){
                if(rk->has_run){
                    rk->unrun();
                    last_instant->readers[k].first.reset() ; // remove the link to the reader since it might not read this when it reruns
                }
            }
        }
        last_instant = last_instant->prev ;
    }

    if(last_instant->write_time >= time){
        printf("WTF: deleting data before the earliest available instant!\n");
    }

    for(int k=0;k<last_instant->readers.size();k++){
        if(auto rk = last_instant->readers[k].first.lock()){
            if(rk->has_run && rk->time > time){ // have to check time because the last instant may only have some reruns
                rk->unrun();
                last_instant->readers[k].first.reset() ; // remove the link to the reader since it might not read this when it reruns
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

    // clear out run events older than the time and any disabled events stil lingering
    for(int k=0;k<events.size();k++){
        if(events[k] && (events[k]->disabled || (events[k]->has_run && events[k]->time < clear_time))){
            collisions.removeRequests(events[k].get());
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
std::pair<std::vector<std::shared_ptr<TEvent>>, std::map<int, std::shared_ptr<TObject>>> Timeline::getBaseState(double time){
    lock.lock();
    if(time < last_clear_time){
        printf("Base state requested for time older than clear time!\n");
    }else if(time > current_time){
        printf("Base state requested for time newer than current time!\n");

    }
    // get eventsafter time that are not spawned by other events after time
    std::vector<std::shared_ptr<TEvent>> base_events ;
    for(int k=0;k<events.size();k++){
        if(events[k].get() != nullptr && !events[k]->disabled && events[k]->time > time){
            std::weak_ptr<TEvent> spawner_w = events[k]->spawner ;
            if(auto spawner = spawner_w.lock()){
                if(spawner->time <= time){
                    base_events.push_back(events[k]);
                }
            }else{
                base_events.push_back(events[k]);
            }
        }
    }

    // return the value of the object at the time
    std::map<int, std::shared_ptr<TObject>> base_objects ;
    for(auto& [id, history] : objects){
        weak_ptr<TObject> ow = getObjectInstant(id, time);
        if(auto o = ow.lock()){
            base_objects[id] = o ;
        }
    }
    lock.unlock();
    return std::make_pair(base_events, base_objects) ;
}

// Returns a serialized descriptor of the state of the this Timeline at the goiven time 
// that can be used by another Timeline to generate a synchronization update
Variant Timeline::getDescriptor(double time,bool server){
    if(time < last_clear_time || time > current_time){ // asked for an update outside our timeline
        printf("Descriptor requested outside of time slice!\n");
        //return Variant();
    }
    long temp = last_run_time; // TODO maybe make a catch up function?
    run(current_time) ; // make sure we're caught up on running since updates before generating descriptor could cause rollback
    last_run_time = temp ;// don't change last run time though because we don't want this to move the clock

    map<string,Variant> descriptor_map ;
    time = fmax(time, last_clear_time);
    descriptor_map["time"] = Variant(time);
    
    auto [base_events,base_objects] = getBaseState(time);
    int* base_event_hashes  = (int*)malloc(4 * base_events.size());
    for(int k=0;k<base_events.size();k++){
        base_event_hashes[k] = Variant(base_events[k]->serialize()).hash() ;
    }
    descriptor_map["events"] = Variant(base_event_hashes, base_events.size() );
    free(base_event_hashes);

    if(!server){
        int* base_object_hashes  = (int*)malloc(4 * 2 * base_objects.size());
        int k=0;
        for(auto& [id,object] : base_objects){
            base_object_hashes[k*2] = id;
            base_object_hashes[k*2+1] = Variant(object->serialize()).hash();
            k++;
        }
        descriptor_map["objects"] = Variant(base_object_hashes, 2 * base_objects.size());
        free(base_object_hashes);
    }
    
    return Variant(descriptor_map);
}

// Given another tree's descriptor, produces an update that would bring that tree into sync with this one
Variant Timeline::getUpdateFor(const Variant& descriptor, bool server){
    map<string,Variant> descriptor_map  = descriptor.getObject();
    double time = descriptor_map["time"].getDouble();
    time = fmax(time, last_clear_time); // if requested an update older than cleared time, then send cleared time

    // Correct client clock drift
    
    if(!server){
        double target_time = time+base_age ;
        
        /*
        if(ping > 1 && ping < 200){
            target_time += ping/2000.0 ; // descriptor time comes from server so only aged by one way ping
        }*/
        if(fabs(current_time - target_time) > base_age*0.1 && (current_time < target_time || target_time > current_time-last_clear_time)){ 
            printf("Correcting clock in getupdateFor: update time:%f target:%f current:%f ping:%d\n", time, target_time, current_time, ping);
            run(target_time); // catch up
        }
    }
    if(time > current_time){ // asked for an update outside our timeline
        printf("Update requested ahead of time slice!\n");
        return Variant() ; // return nothing, we don't know what's outside of our time slice
    }

    
    int* other_events = descriptor_map["events"].getIntArray();
    int num_other_events = descriptor_map["events"].getArrayLength();
    unordered_set<int> other_event_set ;
    for(int k=0;k<num_other_events;k++){
        other_event_set.insert(other_events[k]);
    }
    
    map<string,Variant> update_map ;
    update_map["time"] = Variant(time);

    auto [base_events,base_objects] = getBaseState(time);
    vector<Variant> event_updates;
    for(int k=0;k<base_events.size();k++){
        Variant serial = Variant(base_events[k]->serialize());
        //serial.printFormatted();
        int hash = serial.hash();
        if(other_event_set.find(hash) == other_event_set.end()){ // we have event other didn't have
            event_updates.emplace_back(std::move(serial)); 
        }
    }

    update_map["events"] = Variant(event_updates);
    if(server){ // only server can update objects
        int* other_objects = descriptor_map["objects"].getIntArray();
        int num_other_objects = descriptor_map["objects"].getArrayLength()/2;
        unordered_map<int,int> other_object_map;
        for(int k=0;k<num_other_objects ;k++){
            other_object_map[other_objects[2*k]] = other_objects[2*k+1];
            //printf("got hash %d for %d\n", other_objects[2*k+1], other_objects[2*k]);
        }
        map<int,Variant> object_updates ;
        for(auto& [id,object] : base_objects){
            Variant serial = Variant(object->serialize()) ;
            int hash = serial.hash();
            //printf("generated hash %d for %d\n", hash, id);
            if(other_object_map[id] != hash){
                object_updates[id] = std::move(serial) ;
            }

        }
        update_map["objects"] = Variant(object_updates);
    }

    return Variant(update_map);
}

// applies a syncrhoniation update produced by another timeline's use of getUpdateFor
// returns the time of the update
void Timeline::applyUpdate(const Variant& update, bool server){
    map<string,Variant> update_map = update.getObject();
    double time = update_map["time"].getDouble();
    
    if(!server){
        double target_time = time+base_age ;
       
        if(target_time - current_time > base_age*0.1){  // apply update can only push the clock forward
            printf("Correcting clock in applyUpdate: update time:%f target:%f current:%f ping:%d\n", time, target_time, current_time, ping);
            run(target_time); // catch up
        }
    }

    if(time <= last_clear_time || time > current_time){ // asked for an update outside our timeline
        printf("Update received outside of time slice!\n");
        return ; // return nothing, we don't know what's outside of our time slice
    }

    if(update_map.find("objects") != update_map.end()){
        map<int,Variant> object_updates = update_map["objects"].getIntObject();
        for(auto& [id,serial] : object_updates){
            if(objects.find(id) == objects.end()){ // if not present
                //printf("update creating new baseobject!\n");
                //serial.printFormatted();
                unique_ptr<TObject> new_obj = TObject::generateTypedTObject(serial); // infer type and build using generator
                //Variant(new_obj->serialize()).printFormatted();
                objects[id] = std::move(new_obj) ;// place into timeline
                objects[id]->write_time = time;
                objects[id]->timeline = this ;
            }else{ // if already present but nonmatching value
                
                //printf("Update overwriting existing object! time : %f, current_time: %f, last_clear_time : %f\n", time, current_time, last_clear_time);
                weak_ptr<TObject> existing_obj = getMutable(id, time); // write using functionality that triggers rollback
                if(auto existing = existing_obj.lock()){
                    map<string,Variant> serial_map = serial.getObject() ;
                    existing->set(serial_map);
                }else{
                    //printf("Object exists in timeline, but not at the time it's being overwritten!? time : %f, current_time: %f, last_clear_time : %f\n", time, current_time, last_clear_time);
                }
                
            }
        }
    }

    vector<Variant> event_updates = update_map["events"].getVariantArray();
    for(int k=0;k<event_updates.size();k++){
        //printf("updating event\n");
        insertEvent(std::move(TEvent::generateTypedTEvent(event_updates[k])));
    }
}

// Given a packet with an update and optional descriptor
// applies the update, and if there was a descriptor returns an ypdate for it
// and a new descriptor of itself at current_time-base_age
std::map<std::string, Variant> Timeline::synchronize(std::map<std::string, Variant>& packet, bool server){
    lock.lock();
    if(packet.find("update") != packet.end()){
        auto update = packet["update"].getObject() ;
        /*
        if(update.find("objects") != update.end()){
            auto uo = update["objects"].getIntObject();
            if(uo.size() >0 ){
                printf("Got object update:\n");
                packet["update"].printFormatted();
                auto uo = update["objects"].getIntObject();
                for(auto& [id, obj] : uo){
                    obj.printFormatted();
                    weak_ptr<TObject> co = getObjectInstant(id, update["time"].getDouble());
                    if(auto co2 = co.lock()){
                        co2->print();
                    }else{
                        printf("Object not found!\n");
                    }
                }
            }
        }*/
        applyUpdate(packet["update"], server);
    }
    if(packet.find("descriptor") != packet.end()){
        //printf("Got descriptor:\n");
        //packet["descriptor"].printFormatted();
        if(!server){
            long new_sync_time = timeMilliseconds() ;
            ping = new_sync_time - last_sync_time ;
            last_sync_time = new_sync_time;
        }


        map<string,Variant> ret_map;
        //printf("generating update - > %f...\n", packet["descriptor"]["time"].getDouble());
        ret_map["update"] = getUpdateFor(packet["descriptor"], server);
        if(!ret_map["update"].defined()){
            ret_map.erase("update");
        }

       // ret_map["update"].printFormatted();
        //printf("generating descriptor - > %f...\n", current_time-base_age);
        ret_map["descriptor"] = getDescriptor(current_time-base_age, server);
        //ret_map["descriptor"].printFormatted();
        /*if(!ret_map["descriptor"].defined()){
            printf("Could not generate descriptor, synchronization lost!\n");
            ret_map.erase("descriptor");
        }*/
        lock.unlock();
        return ret_map;
    }
    lock.unlock();
    return std::map<string, Variant>();
}

// Updates all observables to the current time, performing interpolation as required
// and returnsa list of ID for all observables
std::vector<int> Timeline::updateObservables(){
    lock.lock();
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
    lock.unlock();
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
    lock.lock();
    int max_id=0 ;
    for(auto& [id, object_history] : objects){ // TODO find a way to do this less likely to create remote conflicts
        max_id = std::max(max_id, id);
    }
    lock.unlock();
    return max_id + 1 ;
}

long Timeline::timeMilliseconds() const{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}
