#ifndef _TOBJECT_H_
#define _TOBJECT_H_ 1

#include "Variant.h"
#include "TEvent.h"
#include "Timeline.h"
#include "glm/vec3.hpp"

#include <map>
#include <vector>
#include <string>
#include <memory>

class TEvent;
class Timeline;

class TObject{

    public:

        glm::vec3 position;
        float radius;
        short type = -1 ; // a numerical type you can check when you ned to cast (if the overriders use it!)
        bool has_collision = true; // if false object cannot use getCollisions andwill not show up in others' calls

        double write_time ;
        bool deleted = false;
        std::shared_ptr<TObject> prev ;
        std::weak_ptr<TObject> next ;
        Timeline* timeline ;
        //std::vector<std::pair<std::weak_ptr<TEvent>, double>> readers ; // events that have read this object instant and when 
        std::map<std::weak_ptr<TEvent>, double, std::owner_less<std::weak_ptr<TEvent>>> readers;

        TObject();

        virtual ~TObject() = default ;

        // pointer to user defined function to generated typed TObjects for their app
        static std::unique_ptr<TObject>(*generateTypedTObject)(const Variant& serialized) ; 

        // Serialize this object, so it can be efficiently moved between timelines
        virtual std::map<std::string,Variant> serialize() const{
            printf("Serialize not defined for object!\n");
            return std::map<std::string,Variant> ();
        }

        // Set this object to data generated by its serialize method
        virtual void set(std::map<std::string,Variant>& serialized){
            printf("Set not defined for object!\n");
        }

        // Override this to provide an efficient deep copy of this object
        // If not overridden serialize and set will be used to copy your object (which will be inefficent)
        virtual std::unique_ptr<TObject> deepCopy();

        // Override this function to provide logic for interpolation after rollback or extrapolation for slowly updating objects
        // If not overridden getObserved returns the raw value of the object
        virtual std::unique_ptr<TObject> getObserved(double time, const std::weak_ptr<TObject> last_observed, double last_time);

        void print() const;

        // Data and functions below this point are used for maintining the timeline continuity
           
};
#endif // #ifndef _TOBJECT_H_