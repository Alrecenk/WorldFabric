#include "KDNode.h"

using std::unordered_set;
using glm::vec3;
#include <algorithm>

// Convenience method for calling get collisions externally on the root node
unordered_set<int> KDNode::getCollisionCandidates(const KDNode::BoundingSphere& m) {
    unordered_set<int> candidates;
    candidates.reserve(200);
    getCollisionCandidates(m, candidates);
    //printf("candidates:%d\n",(int)candidates.size());
    return candidates;
}

// first is min, second is max of the bounding box of the given bounding sphere
std::pair<glm::vec3,glm::vec3> KDNode::getBoundingBox(const BoundingSphere& m){
    return std::pair<glm::vec3,glm::vec3>(
        vec3(m.center.x - m.radius, m.center.y - m.radius, m.center.z - m.radius),
        vec3(m.center.x + m.radius, m.center.y + m.radius, m.center.z + m.radius)
    );
}