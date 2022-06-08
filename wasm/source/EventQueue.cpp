#include "EventQueue.h"

#include "ObjectHistory.h"
#include "Timeline.h"
#include "TObject.h"
#include "TEvent.h"

#include <memory>

using std::vector ;

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

TEvent* EventQueue::addEvent(std::unique_ptr<TEvent> event){
    // Put it in the slot of a delted item if posible
    for(int k=0;k<events.size();k++){
        if(events[k].get() == nullptr || events[k]->deleted){
            events[k] = std::move(event) ; // TODO could cause a memory leak if event being overwritten has an unowned pointer
            return events[k].get();
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