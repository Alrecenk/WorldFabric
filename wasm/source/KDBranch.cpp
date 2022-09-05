#include "KDBranch.h"
#include "KDLeaf.h"

#include <algorithm>

using std::set;
using glm::vec3;
using std::vector;
using std::unique_ptr;
using std::unordered_set;

KDBranch::KDBranch(int axs, float val, std::map<int, KDNode::BoundingSphere>& objects) {
    axis = axs;
    value = val;
    less_child = unique_ptr<KDNode>(new KDLeaf(this));
    more_child = unique_ptr<KDNode>(new KDLeaf(this));
    for (auto& [id, obj] : objects) {
        add(obj);
    }
}

// Returns -1 if mesh_ is on less side
// Return +1 if on more side
// Returns 0 if spans split
int KDBranch::getSplitSide(const KDNode::BoundingSphere& m) const {
    auto box = getBoundingBox(m);
    vec3 world_min = box.first;
    vec3 world_max = box.second;
    if (world_max[axis] < value) {
        return -1;
    } else if (world_min[axis] >= value) {
        return 1;
    } else {
        return 0;
    }
}

KDNode* KDBranch::add(KDNode::BoundingSphere& m) {
    int side = getSplitSide(m);
    // Child leaf nodes sometimes become branch nodes on adds
    if (side <= 0) {
        KDNode* new_less = less_child->add(m);
        if (new_less != less_child.get()) {
            less_child = unique_ptr<KDNode>(new_less);
        }
    }
    if (side >= 0) {
        KDNode* new_more = more_child->add(m);
        if (new_more != more_child.get()) {
            more_child = unique_ptr<KDNode>(new_more);
        }
    }
    return this; // Branch nodes stay branch nodes;
}

//TODO we only use the front item, fetching ste is inefficient
void KDBranch::getCollisionCandidates(const KDNode::BoundingSphere& m, std::unordered_set<int>& candidates) {
    int side = getSplitSide(m);
    if (side <= 0) {
        less_child->getCollisionCandidates(m, candidates);
    }
    if (side >= 0) {
        more_child->getCollisionCandidates(m, candidates);
    }
}

// Clears all bounding sphre references from the tree older than clear_time if it's not their newest reference
KDNode* KDBranch::clearHistory(double clear_time, std::unordered_map<int,double>& last_edit_time){
    less_child->clearHistory(clear_time, last_edit_time);
    more_child->clearHistory(clear_time, last_edit_time);
    //TODO if(less_child->objects.size() + more_child->objects.size() < amount_to_merge){merge}
    return this ;
}
