#ifndef _CREATEOBJECT_H_
#define _CREATEOBJECT_H_ 1

#include "TEvent.h"
#include "Timeline.h"
#include "TObject.h"

class CreateObject : TEvent{

    public:
        // Serialize this event's data, so it can be efficiently moved between timelines
        std::map<string,Variant> serialize() override;

        // Set this event to data generated by its serialize method
        void set(std::map<string,Variant>& serialized) override;

        // Runs the event
        // This is what you need to override to implement your application
        // To maintain causality run should only interact with dynamic data by using the privided methods:
        // get(id), getMutable(), addEvent, createObject, deleteObject, and getCollisions
        run() override;

    private:
        TObject new_object;
        TEvent on_created;

        
};
#endif // #ifndef _EVENTQUEUE_H_