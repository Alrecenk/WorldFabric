#ifndef _TIMELINE_H_
#define _TIMELINE_H_ 1

#include "Variant.h"
#include "glm/vec3.hpp"
#include "TObject.h"
#include "TEvent.h"
#include "EventQueue.h"
#include "ObjectHistory.h"
#include <unordered_map>
#include <vector>

#include <map>
#include <string>

#include "glm/gtx/transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/ext.hpp"

class TObject;
class TEvent;
class EventQueue;
class ObjectHistory;

class Timeline{

    public:

        int vantage_id = 0; // ID of object currently acting as vantage point
        double current_time = 0; // current time at vantage point
        double info_speed = 1000; // maximum speed of information transfer between events and data
        double time_kept = 1.0 ; //Amount of history kept in the timeline
        double min_spawned_event_delay = 1.0/120; // Minimum time between an event spawned by another event at the same anchor

        // Set the functions to be used for generating typed timeline events and objects from serialized data
        void setGenerators(TEvent*(*event_generator)(Variant& serialized), TObject*(*object_generator)(Variant& serialized));

        // Adds an event to this timeline
        // Peforms rollback and correction as required
        void addEvent(TEvent e, double send_time);

        // Creates an event that creates an object at the earliest possible time
        void createObject(TObject obj, TEvent on_created, double send_time);

         // Creates an event that deletes an object at the earliest possible time
        void deleteObject(TObject obj, double send_time);

        // Runs events in the timeline until the location at the vantage object reaches the given time
        void run(double new_time);

        // Returns a serialized descriptor of the state of the this Timeline that can be used by another Timeline ot generate a synchronization update
        Variant getDescriptor();

        // Given another tree's descriptor, produces an update that woulds bring that tree into syncwith this one
        Variant getUpdateFor(Variant descriptor);

        // applies a syncrhoniation update produced by another timeline's use of getUpdateFor'
        void applyUpdate(Variant update);

        // Updates all observables to the current time, performing interpolation as required
        // and returnsa list of ID for all observables
        std::vector<int> updateObservables();

        // Returns a reference to the last observed value of a given ID
        const TObject& getLastObserved(int id);

        // returns the next valid ID that should be used for a created object
        int getNextID();

        EventQueue events;
        std::unordered_map<int, ObjectHistory> objects;
        std::unordered_map<int, TObject> last_observed ;

        std::vector<TEvent*> pending_external_events ; // tracks externally created events for quicksend

};
#endif // #ifndef _TIMELINE_H_