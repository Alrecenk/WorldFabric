#include "KDLeaf.h"
#include "KDBranch.h"

#include <algorithm>
#include <cmath>

using std::set;
using glm::vec3;
using std::vector;
using std::unordered_map ;

KDLeaf::KDLeaf() {
    parent = nullptr;
    objects.reserve(amount_to_split);
}

KDLeaf::KDLeaf(KDBranch* p) {
    parent = p;
    objects.reserve(amount_to_split);
}

KDNode* KDLeaf::split() {
    // Make box of 2 random objects, split at middle on axis of biggest difference

    int index_1 = rand() % objects.size() ;
    int index_2 = rand() % objects.size() ;
    int i = 0 ;
    std::pair<glm::vec3,glm::vec3> box1, box2;
    for(auto& [id, sphere] : objects){
        if(i == index_1){
            box1 = getBoundingBox(sphere) ;
        }
        if(i == index_2){
            box2 = getBoundingBox(sphere) ;
        }
        i++;
    }
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

    if(axis > 0){
        float value = (box_max[axis] + box_min[axis]) * .5f;
        //count number of objects not crossing boundary on each side
        int num_less = 0;
        int num_more = 0;
        for(auto& [id, sphere] : objects){
            if(sphere.center[axis] + sphere.radius < value){
                num_less++;
            }
            if(sphere.center[axis] - sphere.radius  > value){
                num_more++;
            }
        }
        if (num_less>=required_to_split && num_more>=required_to_split) {
            return new KDBranch(axis, value, objects);
        } else {
            split_delay = adds_before_retrying_split; // if split fails don't try again for 10 adds  (we readd a lot of the same things)
            //printf("Reached collision leaf limit but split unsuccessful. Performance will be bad. Are you stacking a bunch of timeline objects in one place?\n");
            return this;
        }
    }else{
        return this ;
    }
}

KDNode* KDLeaf::add(KDNode::BoundingSphere& m) {
    objects[m.id] = m;

    if (objects.size() >= KDLeaf::amount_to_split) {
        split_delay--;
        if(split_delay <=0){
            return split();
        }else{
            return this ;
        }
    } else {
        return this;
    }
}

void KDLeaf::getCollisionCandidates(const KDNode::BoundingSphere& m, std::unordered_set<int>& candidates) {
    for (auto& [id, sphere] : objects) {
        candidates.insert(id);
    }
}

// Clears all bounding sphre references from the tree older than clear_time if it's not their newest reference
KDNode* KDLeaf::clearHistory(double clear_time, std::unordered_map<int,double>& last_edit_time){
    unordered_map<int, KDNode::BoundingSphere> new_objects ;
    for (auto& [id, sphere] : objects) {
        if(sphere.time >= fmin(clear_time, last_edit_time[id])){
            new_objects[id] = sphere ;
        }
    }
    objects = new_objects ;
    return this ;
}