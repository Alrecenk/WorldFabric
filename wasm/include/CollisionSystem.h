#ifndef _COLLISION_SYSTEM_H_
#define _COLLISION_SYSTEM_H_ 1

#include <vector>
#include <unordered_map>

class TObject;
class TEvent;
class Timeline;

class CollisionSystem{

    public:
        Timeline* timeline;
        
        // Returns the collision indices for a given event
        // Logs the result to rollback if needed
        std::vector<int> getCollisions(TEvent* event);

        // Must be called when an event is deleted to potentially clear out its collision request history
        void removeRequests(TEvent* event);

        // Must be called when an event writes its anchpor object
        // to potentially rollback events with changed collision results
        void onDataChanged(TEvent* event);


    private:
        std::unordered_map<TEvent*, std::vector<int>> requests ;
        double most_recent_request_time = 0 ;
           
};
#endif // #ifndef _COLLISION_SYSTEM_H_