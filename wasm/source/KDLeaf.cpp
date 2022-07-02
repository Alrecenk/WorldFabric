#include "KDLeaf.h"
#include "KDBranch.h"

#include <algorithm>
#include <cmath>

using std::set;
using glm::vec3;
using std::vector;

KDLeaf::KDLeaf() {
    parent = nullptr;
}

KDLeaf::KDLeaf(KDBranch* parent) {
    parent = parent;
}

KDNode* KDLeaf::split() {
    // Make box of 2 random objects, split at middle on axis of biggest difference
    auto m1 = objects[rand() % objects.size()], m2 = objects[
            rand() % objects.size()];
    auto box1 = getBoundingBox(m1);
    auto box2 = getBoundingBox(m2);
    vec3 box_min(
            fmin(box1.first.x, box2.first.x),
            fmin(box1.first.y, box2.first.y),
            fmin(box1.first.z, box2.first.z)
    );
    vec3 box_max(
            fmax(box1.second.x, box2.second.x),
            fmax(box1.second.y, box2.second.y),
            fmax(box1.second.z, box2.second.z)
    );
    int axis = -1;
    float best_diff = 0;
    for (int k = 0; k < 3; k++) {
        float diff = box_max[k] - box_min[k];
        if (diff > best_diff) {
            best_diff = diff;
            axis = k;
        }
    }
    if (axis >= 0) {
        float value = (box_max[axis] + box_min[axis]) * .5f;
        return new KDBranch(axis, value, objects);
    } else {
        return this;
    }
}

KDNode* KDLeaf::add(KDNode::BoundingSphere& m) {
    objects.push_back(m);

    if (objects.size() >= KDLeaf::amount_to_split) {
        return split();
    } else {
        return this;
    }
}

void KDLeaf::getCollisionCandidates(const KDNode::BoundingSphere& m, set<int>& candidates) {
    for (auto& obj : objects) {
        candidates.insert(obj.id);
    }
}

// Clears all bounding sphre references from the tree older than clear_time if it's not their newest reference
KDNode* KDLeaf::clearHistory(double clear_time, std::unordered_map<int,double>& last_edit_time){
    vector<KDNode::BoundingSphere> new_objects ;
    for (auto& obj : objects) {
        if(obj.time >= fmin(clear_time, last_edit_time[obj.id])){
            new_objects.push_back(obj);
        }
    }
    objects = new_objects ;
    return this ;
}