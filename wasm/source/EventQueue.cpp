#include "EventQueue.h"

#include "ObjectHistory.h"
#include "Timeline.h"
#include "TObject.h"
#include "TEvent.h"

#include <memory>

using std::vector ;
using std::map ;

// Returns the next event to be run from the given perspective
// returns nullptr if the queue is up to date
TEvent* EventQueue::next(glm::vec3 vantage, double time, double info_speed){
    //printf("event queue next...\n");
    double best_time = time;
    TEvent* best_event = nullptr;
    for(std::unique_ptr<TEvent>& e : events){
        if(e.get() != nullptr && !e->deleted && e->run_pending){
            TObject* eo = timeline->objects[e->anchor_id].get(e->time) ;
            double time_to_run = e->time ;
            if(eo !=nullptr){
                double dist = glm::length(vantage- eo->position);
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

// Returns a map from run_time(from this vantage) to event for all events pthat could be run at this time
// Note if any of these events changes the cantage then the all events after that may be out of order and you'll have to recalculate
std::map<double,TEvent*> EventQueue::allNext(glm::vec3 vantage, double time, double info_speed){
    map<double, TEvent*> all_next ;
    for(std::unique_ptr<TEvent>& e : events){
        if(e.get() != nullptr && !e->deleted && e->run_pending){
            TObject* eo = timeline->objects[e->anchor_id].get(e->time) ;
            double time_to_run = e->time ;
            if(eo !=nullptr){
                double dist = glm::length(vantage- eo->position);
                time_to_run = e->time + dist/info_speed;
            }
            if(time_to_run <= time){
                //printf("queue time: %f\n", time_to_run);
                if(all_next.find(time_to_run) != all_next.end()){
                    printf("Events had identical run_time! execution order not guranteed!\n");
                    all_next = map<double, TEvent*>();
                    all_next[0] = next(vantage, time, info_speed); // fall back to single next to try to recover
                    return all_next ;
                }else{
                    all_next[time_to_run] = e.get();
                }
            }
        }
    }
    return all_next ;
}

TEvent* EventQueue::addEvent(std::unique_ptr<TEvent> event){
    event->timeline = timeline ;
    // Put it in the slot of a delted item if posible
    for(int k=0;k<events.size();k++){
        int p = (k + add_pointer)%events.size();
        if(events[p].get() == nullptr || events[p]->deleted){
            events[p] = std::move(event) ;
            add_pointer = p+1; // next time start checking from the slot after the one we just filled
            return events[p].get();
        }
    }
    // No free slots, push on the end
    events.push_back(std::move(event));
    return events[events.size()-1].get();
}

void EventQueue::removeDependencies(TEvent* event){
    
    // Delete any data following from this write
    if(event->wrote_anchor){
        timeline->objects[event->anchor_id].deleteAfter(event->time);
        event->wrote_anchor = false;
    }

    // Delete any events this event spawned
    for(TEvent* s : event->spawned_events){
        if(!s->deleted){
            deleteEvent(s);
        }
    }
    event -> spawned_events.clear();
}

void EventQueue::deleteEvent(TEvent* event){
    //printf("Deleting event at time %f \n", event->time);
    removeDependencies(event);
    timeline->collisions.onDelete(event);
    event->deleted = true;
    event->spawner = nullptr ;
}

void EventQueue::rerunEvent(TEvent* event){
    //printf("Queueing rerun of event at time %f \n", event->time);
    removeDependencies(event);
    event->run_pending = true;
}

// Clears out all events before the given time
void EventQueue::clearHistoryBefore(double clear_time){
    // mark deleted and clear all pointers to events about to be newly removed
    for(int k=0;k<events.size();k++){
        if(events[k].get() != nullptr && !events[k]->deleted && events[k]->time < clear_time && !events[k]->run_pending ){
            timeline->collisions.onDelete(events[k].get()); // remove pointers in collision system
            events[k]->deleted = true;
            events[k]->spawner = nullptr ;
            // Remove the link to this as a spawner of future events that may not be deleted
            for(TEvent* s : events[k]->spawned_events){
                s->spawner = nullptr ;
            }
        }
    }
    //wipe the data for all deleted events
    for(int k=0;k<events.size();k++){
        if(events[k].get() != nullptr && events[k]->deleted){
            events[k].reset(); // actually free the data in the event
        }
    }
}

// Returns all events after the given time that were not spawned by another event also after the given time
std::vector<TEvent*> EventQueue::getBase(double time){
    vector<TEvent*> base ;
    for(int k=0;k<events.size();k++){
        if(events[k].get() != nullptr && !events[k]->deleted && events[k]->time > time){
            TEvent* spawner = events[k]->spawner ;
            if(spawner == nullptr || spawner->time < time){
                base.push_back(events[k].get());
            }
        }
    }
    return base ;
}