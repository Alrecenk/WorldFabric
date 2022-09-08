#ifndef _KD_LEAF_H_
#define _KD_LEAF_H_ 1

#include "glm/vec3.hpp"
#include <memory>
#include <set>
#include <vector>
#include <map>
#include "KDNode.h"

class KDLeaf : public KDNode {
  public:

    static const int amount_to_split = 30;
    std::map<int, KDNode::BoundingSphere> objects;
    int split_delay = 0 ;

    // Constructor withand without parent
    explicit KDLeaf(KDBranch* parent);

    KDLeaf();

    // Returns a branch to replace this leaf
    KDNode* split();

    // Override virtual methods for the leaf
    KDNode* add(KDNode::BoundingSphere& m) override;

    // Accumulates the ids for collisions of objects in this node into the collision set passed
    void getCollisionCandidates(const KDNode::BoundingSphere& m, std::unordered_set<int>& candidates) override;

    // Clears all bounding sphre references from the tree older than clear_time if it's not their newest reference
    KDNode* clearHistory(double clear_time, std::unordered_map<int,double>& last_edit_time) override;   

    // Convenience method for calling get collisions externally on the root node
    std::set<int> getCollisions(const KDNode::BoundingSphere& m);

    ~KDLeaf() override = default;
};


#endif // #ifndef _KD_LEAF_H_