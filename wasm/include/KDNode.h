#ifndef _KD_NODE_H_
#define _KD_NODE_H_ 1


#include "glm/vec3.hpp"
#include <stdio.h>
#include <memory>
#include <set>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>


class KDBranch; // Forward declaration for parent

class KDNode {
  public:
    struct BoundingSphere{
        int id=-1; // id of object thisis bounding
        glm::vec3 center;
        float radius = 0;
        double time = 0;
    };

    KDBranch* parent;

    // Adds a mesh to this node, may return a branch when called on a leaf if the node was split
    virtual KDNode* add(KDNode::BoundingSphere& m) = 0;

    // Accumulates the ids for collisions of objects in this node into the collision set passed
    virtual void getCollisionCandidates(const KDNode::BoundingSphere& m, std::unordered_set<int>& candidates) = 0;

    // Clears all bounding sphre references from the tree older than clear_time if it's not their newest reference
    virtual KDNode* clearHistory(double clear_time, std::unordered_map<int,double>& last_edit_time) = 0;

    // Convenience method for calling get collisions externally on the root node
    std::unordered_set<int> getCollisionCandidates(const KDNode::BoundingSphere& m);

    // first is min, second is max of the bounding box of the given bounding sphere
    static std::pair<glm::vec3,glm::vec3> getBoundingBox(const BoundingSphere& m);


    virtual ~KDNode() = default;
};
#endif // #ifndef _KD_NODE_H_