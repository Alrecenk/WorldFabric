#include "EventQueue.h"

// Returns the next event to be run from the given perspective
// returns nullptr if the queue is up to date
TEvent* EventQueue::next(glm::vec3 vantage, double time, double info_speed){

    double best_time = time// TODO double max;
    TEvent* best_event = nullptr;
    for(TEvent& e : events){
        double dist = glm::length(vantage-timeline->objects[e.anchor_id].get(e.time)->position);
        double time_to_run = e.time + dist/info_speed;
        if(time_to_run <= best_time){
            best_time = time_to_run;
            best_event = &e ;
        }
    }
    return best_event ;
}

TEvent* EventQueue::addEvent(const TEvent& event){
    // Put it in the slot of a delted item if posible
    for(int k=0;k<events.size();k++){
        if(events[k].deleted){
            events[k] = event ; // TODO could cause a memory leak if event being overwritten has an unowned pointer
            return &events[k];
        }
    }
    // No free slots, push on the end

    events.push_back(event);
    return &events[events.size()-1];
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
    }

    event->wrote_anchor = false;
}

void EventQueue::deleteEvent(TEvent* event){
    removeDependencies(event);
    event->deleted = true;
}

void EventQueue::rerunEvent(TEvent* event){
    removeDependencies(event);
    event->run_pending = true;
}