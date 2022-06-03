#include "EventQueue.h"

#include "ObjectHistory.h"
#include "Timeline.h"
#include "TObject.h"
#include "TEvent.h"

#include <memory>

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
        if(events[k]->deleted){
            events[k] = std::move(event) ; // TODO could cause a memory leak if event being overwritten has an unowned pointer
            return events[k].get();
        }
    }
    // No free slots, push on the end

    events.push_back(std::move(event));
    return events[events.size()-1].get();
}

void EventQueue::removeDependencies(TEvent* event){
    // Delete any events this event spawned
    for(TEvent* s : event->spawned_events){
        if(!s->deleted){
            deleteEvent(s);
        }
    }
    event -> spawned_events.clear();

    if(event->wrote_anchor){
        timeline->objects[event->anchor_id].deleteAt(event->time);
        event->wrote_anchor = false;
    }

}

void EventQueue::deleteEvent(TEvent* event){
    printf("Deleting event at time %f \n", event->time);
    removeDependencies(event);
    event->deleted = true;
}

void EventQueue::rerunEvent(TEvent* event){
    printf("Queueing rerun of event at time %f \n", event->time);
    removeDependencies(event);
    event->run_pending = true;
}