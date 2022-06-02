#ifndef _EVENTQUEUE_H_
#define _EVENTQUEUE_H_ 1

#include "Variant.h"
#include "Timeline.h"
#include "TEvent.h"
#include "glm/vec3.hpp"
#include <vector>

class TObject;
class TEvent;
class Timeline;
class ObjectHistory;

class EventQueue{

    public:
        // Returns the next event to be run from the given perspective
        // returns null if the queue is up to date
        TEvent* next(glm::vec3 vantage, double time, double info_speed);

        // Adds an event to the queue
        // If event requies rollback this automatically clears affected events
        TEvent* addEvent(const TEvent& event);

        // deletes an Event
        // This for internal use when a reran event has spawned events that need to be cleared
        // This is NOT SAFE to call unless any pointers to the event have already been cleared
        void deleteEvent(TEvent* event);

        // Marks an event to be rerun
        // This is for internal use when data has been changed after an event has already read it
        void rerunEvent(TEvent* event);

        void removeDependencies(TEvent* event);

    private:
        std::vector<TEvent> events;
        Timeline* timeline;

        
};
#endif // #ifndef _EVENTQUEUE_H_