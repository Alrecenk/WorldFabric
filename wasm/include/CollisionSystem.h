#ifndef _COLLISION_SYSTEM_H_
#define _COLLISION_SYSTEM_H_ 1

#include "KDNode.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>

class TObject;
class TEvent;
class Timeline;

class CollisionSystem{

    public:
        Timeline* timeline;

        CollisionSystem();
        
        // Returns the collision indices for a given event
        // Logs the result to rollback if needed
        std::vector<int> getCollisions(TEvent* event);

        // Must be called when an event is deleted to potentially clear out its collision request history
        void removeRequests(TEvent* event);

        // Must be called when an event writes its anchpor object
        // to potentially rollback events with changed collision results
        void onDataChanged(TEvent* event);

        //Adds an object to the collision structure for generating candidates
        void addObject(int id, glm::vec3 x, float radius, double time);

        // Clean up entries in the collision structure that are older than clear_time and no longer present
        void clearHistory(double clear_time);

        std::unordered_set<int> getCandidates(glm::vec3 x, float radius);

    private:
        std::unordered_map<TEvent*, std::vector<int>> requests ;
        double most_recent_request_time = 0 ;
        std::unordered_map<int, double> last_edit_time ;// last edit time for each id
        std::unique_ptr<KDNode> root ; // root of broad phase collision structure
           
};
#endif // #ifndef _COLLISION_SYSTEM_H_