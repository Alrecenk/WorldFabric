#ifndef _TIMELINE_H_
#define _TIMELINE_H_ 1

#include "Variant.h"
#include "glm/vec3.hpp"
#include "EventQueue.h"
#include "ObjectHistory.h"
#include "CollisionSystem.h"

#include "glm/gtx/transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/ext.hpp"

#include <unordered_map>
#include <vector>
#include <map>
#include <string>
#include <mutex>



class TObject;
class TEvent;

class Timeline{

    public:

        int vantage_id = -99999; // ID of object currently acting as vantage point
        double current_time = 0; // current time at vantage point
        double info_speed = 1E100; // maximum speed of information transfer between events and data
        double min_spawned_event_delay = 1.0/1000; // Minimum time between an event spawned by another event at the same anchor
        long last_run_time = 0 ;
        long last_sync_time = 0;
        int ping = 0;
        double last_clear_time = -99999 ;

        static constexpr double base_age = 0.3;
        std::recursive_mutex lock ;

        Timeline();

        // Set the functions to be used for generating typed timeline events and objects from serialized data
        void setGenerators(std::unique_ptr<TEvent> (*event_generator)(const Variant& serialized), 
                                                    std::unique_ptr<TObject>(*object_generator)(const Variant& serialized));

        // Adds an event to this timeline
        // Peforms rollback and correction as required
        void addEvent(std::unique_ptr<TEvent> e, double send_time);

        // Creates an event that creates an object at the earliest possible time
        void createObject(std::unique_ptr<TObject> obj, std::unique_ptr<TEvent> on_created, double send_time);

         // Creates an event that deletes an object at the earliest possible time
        void deleteObject(int id, double send_time);

        // Runs events in the timeline until the location at the vantage object reaches the given time
        void run(double new_time);

        // Runs the current timeline based on the real-time passed since last clal to either run function
        void run();

        // Clears out all events and data changes before the given time
        // Objects may have a single instant before the clear time, so their value at that time can be fetched
        void clearHistoryBefore(double clear_time);

        // Return the minimum state required to generate a matching timeline from the given time
        std::pair<std::vector<TEvent*>, std::map<int, TObject*>> getBaseState(double time);

        // Returns a serialized descriptor of the state of the this Timeline at the goiven time 
        // that can be used by another Timeline to generate a synchronization update
        Variant getDescriptor(double time,bool server);

        // Given another tree's descriptor, produces an update that would bring that tree into sync with this one
        Variant getUpdateFor(const Variant& descriptor, bool server);

        // applies a syncrhoniation update produced by another timeline's use of getUpdateFor
        // returns the time of the update
        void applyUpdate(const Variant& update);

        // Given a packet with an update and optional descriptor
        // applies the update, and if there was a descriptor returns an ypdate for it
        // and a new descriptor of itself at current_time-base_age
        std::map<std::string, Variant> synchronize(std::map<std::string, Variant>& packet, bool server);

        // Updates all observables to the current time, performing interpolation as required
        // and returnsa list of ID for all observables
        std::vector<int> updateObservables();

        // Returns a reference to the last observed value of a given ID
        const TObject* getLastObserved(int id);

        // returns the next valid ID that should be used for a created object
        int getNextID();

        long timeMilliseconds() const;

        EventQueue events;
        std::unordered_map<int, ObjectHistory> objects;
        CollisionSystem collisions;
        std::unordered_map<int, std::unique_ptr<TObject>> last_observed ;

        std::vector<TEvent*> pending_external_events ; // tracks externally created events for quicksend

};
#endif // #ifndef _TIMELINE_H_