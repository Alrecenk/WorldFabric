#ifndef _EVENTQUEUE_H_
#define _EVENTQUEUE_H_ 1

#include "glm/vec3.hpp"

#include <vector>
#include <memory>
#include <map>

class TObject;
class TEvent;
class Timeline;

class EventQueue{

    public:
        Timeline* timeline;
        std::vector<std::shared_ptr<TEvent>> events;
        int add_pointer = 0 ; // place to start looking for slot to add events

        // Returns the next event to be run from the given perspective
        // returns null if the queue is up to date
        TEvent* next(glm::vec3 vantage, double time, double info_speed);

        // Returns a map from run_time(from this vantage) to event for all events pthat could be run at this time
        // Note if any of these events changes the cantage then the all events after that may be out of order and you'll have to recalculate
        std::map<double,TEvent*> allNext(glm::vec3 vantage, double time, double info_speed);

        // Adds an event to the queue
        // If event requires rollback, this automatically clears affected events
        TEvent* addEvent(std::unique_ptr<TEvent> event);

        // deletes an Event
        // This for internal use when a reran event has spawned events that need to be cleared
        // This is NOT SAFE to call unless any pointers to the event have already been cleared
        void deleteEvent(TEvent* event);

        // Marks an event to be rerun
        // This is for internal use when data has been changed after an event has already read it
        void rerunEvent(TEvent* event);

        void removeDependencies(TEvent* event);

        // Clears out all events before the given time
        void clearHistoryBefore(double clear_time);

        // Returns all events after the given time that were not spawned by another event also after the given time
        std::vector<TEvent*> getBase(double time);
        
        

        
};
#endif // #ifndef _EVENTQUEUE_H_