#ifndef _OBJECTHISTORY_H_
#define _OBJECTHISTORY_H_ 1

#include "Variant.h"
#include "glm/vec3.hpp"

#include <vector>
#include <memory>

class TObject;
class TEvent;
class Timeline;
class EventQueue;

class ObjectHistory{

    public:

        double deleted_time = 99999999.0;//TODO max value
        Timeline* timeline;

        ObjectHistory();

        ObjectHistory(std::unique_ptr<TObject> value, double make_time);

        //Returns the latest version of this oject that is observable from the given vantage point and time
        TObject* get(glm::vec3 vantage, double time, double info_speed);


        //Returns the version of this oject at the given time, ignoring time warp
        // For internal use to get the position of events from their anchor
        TObject* get(double time);

        // Deletes the object at the given time
        void deleteAfter(double time);

        //Creates a new instant at the given time by deep copying the previous instant
        // and returns an editable version of it
        // deletes all data beyond that point and marks events appropriately
        TObject* getMutable(double time);

    private:
        std::vector<std::unique_ptr<TObject>> history;
        

};
#endif // #ifndef _OBJECTHISTORY_H_