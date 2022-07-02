#ifndef _KD_BRANCH_H_
#define _KD_BRANCH_H_ 1

#include "glm/vec3.hpp"
#include <memory>
#include <set>
#include <vector>
#include "KDNode.h"

class KDBranch : public KDNode {
  public:
    int axis; // 0,1, or 2 for axis this node splits on
    float value; // the value of the axis this node splits on
    std::unique_ptr<KDNode> less_child; // elements with p[axis] < value_
    std::unique_ptr<KDNode> more_child; // elements with p[axis] >= value_

    // Constructor makes children leaves
    KDBranch(int axis, float value, std::vector<KDNode::BoundingSphere> objects);

    // Returns -1 if mesh_ is on less side
    // Return +1 if on more side
    // Returns 0 is spans split
    int getSplitSide(const KDNode::BoundingSphere& m) const;


    // Override virtual methods for the branch
    KDNode* add(KDNode::BoundingSphere& m) override;

    // Accumulates the ids for collisions of objects in this node into the collision set passed
    void getCollisionCandidates(const KDNode::BoundingSphere& m, std::set<int>& candidates) override;

    // Clears all bounding sphre references from the tree older than clear_time if it's not their newest reference
    KDNode* clearHistory(double clear_time, std::unordered_map<int,double>& last_edit_time) override;  

    ~KDBranch() override = default;
};

#endif // #ifndef _KD_BRANCH_H_