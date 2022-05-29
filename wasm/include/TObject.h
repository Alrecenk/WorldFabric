#ifndef _TOBJECT_H_
#define _TOBJECT_H_ 1

#include "Variant.h"
class Timeline{

    public:

        vec3 position;
        float radius;

        // Serialize this object, so it can be efficiently moved between timelines
        virtual Variant serialize();

        // Set this object to data generated by its serialize method
        virtual void set(Variant& serialized);

        // Override this to provide an efficient deep copy of this object
        // If not overridden serialize and set will be used to copy your object (which will be inefficent)
        TObject deepCopy();

        // Override this function to provide logic for interpolation after rollback or extrapolation for slowly updating objects
        // If not overridden getObserved returns the raw value of the object
        TObject getObserved(const TObject& last_observed);


        // Data and functions below this point are used for maintining the timeline continuity
        vector<*TEvent> readers ; // events that have read this object instant
};
#endif // #ifndef _TOBJECT_H_