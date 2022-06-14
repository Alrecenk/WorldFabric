#ifndef _TEVENT_H_
#define _TEVENT_H_ 1

#include "Variant.h"
#include "Timeline.h"
#include "TObject.h"
#include "glm/vec3.hpp"

#include <map>
#include <vector>
#include <string>
#include <memory>

class TObject;
class Timeline;

class TEvent{

    public:

        double time ; // time this event occurs
        int anchor_id; // The id of the TObject this event is anchored to (and the only object it can edit)

        virtual ~TEvent() = default;

        // pointer to user defined function to generated typed TEvents for their app
        static std::unique_ptr<TEvent>(*generateTypedTEvent)(const Variant& serialized) ; 

         // Serialize this event's data, so it can be efficiently moved between timelines
        virtual std::map<std::string,Variant> serialize() const{
            printf("Serialize not defined for event!\n");
            return std::map<std::string,Variant> ();
        }

        // Set this event to data generated by its serialize method
        virtual void set(std::map<std::string,Variant>& serialized){
            printf("Set not defined for event!\n");
        }

        // Runs the event
        // This is what you need to override to implement your application
        // To maintain causality run should only interact with dynamic data by using the privided methods:
        // get(id), getMutable(), addEvent, createObject, deleteObject, and getCollisions
        virtual void run(){
            printf("Run not defined for event!\n");
        }

        // Returns the latest data for the given object available to this event
        const TObject* get(int id);

        // Returns a mutable version of the object this event is anchored to
        // This is how you edit objects from inside events.
        TObject* getMutable();

        // Adds an event to the Timeline this event is in
        // If no time is set on the event it will be run at the earliest possible time
        void addEvent(std::unique_ptr<TEvent> e);

        // Creates an event that creates an object at the earliest possible time
        void createObject(std::unique_ptr<TObject> obj, std::unique_ptr<TEvent> on_created);

         // Creates an event that deletes an object at the earliest possible time
        void deleteObject(int id);

        // Returns the IDs of all TObjects colliding with the bounding sphere of the anchor object
        // at the time of this event
        std::vector<int> getCollisions();


        // Override this to provide an efficient deep copy of this object
        // If not overridden serialize and set will be used to copy your object (which will be inefficent)
        virtual std::unique_ptr<TEvent> deepCopy();

        void print() const;

        // Data and functions below this point are used for maintaining the timeline continuity

        bool run_pending = true; //whether the event still needs to be run
        std::vector<TEvent*> spawned_events ; //events spawen by this event when it was last run
        TEvent* spawner = nullptr ; // event that spawned this event if it was spawned by another timeline event
        bool wrote_anchor=false; // whether this event wrote to its anchor object last time it ran
        bool deleted = false;
        std::vector<TObject*> read;
        
        Timeline* timeline ;
};
#endif // #ifndef _TEVENT_H_