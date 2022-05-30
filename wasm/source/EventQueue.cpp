#include "EventQueue.h"

// Returns the next event to be run from the given perspective
// returns nullptr if the queue is up to date
TEvent* EventQueue::next(vec3 vantage, double time, double info_speed){

    double best_time = time// TODO double max;
    TEvent* best_event = nullptr;
    for(TEvent* e : events){
        double dist = glm::length(vantage-timeline.objects[e->anchor_id].get(e->time));
        double time_to_run = e->time + length/info_speed;
        if(time_to_run <= best_time){
            best_time = time_to_run;
            best_event = e ;
        }
    }
    return e ;
}

EventQueue::addEvent(const TEvent& event){
    // Put it in the slot of a delted item if posible
    for(int k=0;k<events.size();k++){
        if(events[k].deleted){
            events[k] = event ;
            return ;
        }
    }
    // No free slots, push on the end
    events.push_back(event);
}

EventQueue::removeDependencies(TEvent* event){
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

EventQueue::deleteEvent(TEvent* event){
    removeDependencies(event);
    event->deleted = true;
}

EventQueue::rerunEvent(TEvent* event){
    removeDependencies(event);
    event->run_pending = true;
}