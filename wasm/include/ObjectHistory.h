#ifndef _OBJECTHISTORY_H_
#define _OBJECTHISTORY_H_ 1

#include "Variant.h"
#include "TObject.h"
#include "Timeline.h"
#include "glm/vec3.hpp"

#include <vector>

class TObject;
class TEvent;
class Timeline;
class EventQueue;

class ObjectHistory{

    public:

        ObjectHistory();

        ObjectHistory(TObject value, double make_time);

        //Returns the latest version of this oject that is observable from the given vantage point and time
        TObject* get(glm::vec3 vantage, double time, double info_speed);


        //Returns the version of this oject at the given time, ignoring time warp
        // For internal use to get the position of events from their anchor
        TObject* get(double time);

        // Deletes the object at the given time
        void deleteAt(double time);

        //Creates a new instant at the given time by deep copying the previous instant
        // and returns an editable version of it
        // deletes all data beyond that point and marks events appropriately
        TObject* getMutable(double time);

    private:
        std::vector<TObject> history;
        double deleted_time = 99999999.0;//TODO max value
        Timeline* timeline;

};
#endif // #ifndef _OBJECTHISTORY_H_