
#include "ConvexShape.h"

using std::vector;
using glm::vec3 ;


ConvexShape::ConvexShape(vector<vec3> &vertices, vector<Face> &faces) {
    vertex_ = vertices;
    face = faces;
}


// Return the center of mass of this shape
glm::vec3 ConvexShape::getCentroid(){
    // Get a point on the inside
    vec3 inner_point ;
    for(const auto& v : vertex){
        inner_point+=v;
    }
    inner_point/=vertex.size();
    vec3 centroid(0,0,0);
    float volume = 0 ;
    for(const auto& face : faces){
        for(int k=1; k < face.size()-1;k++){
            vec3& a = vertex[face[0]];
            vec3& b = vertex[face[k]]
            vec3& c = vertex[face[k+1]]
            vec3& d = inner_point
            float vol = computeTetraVolume(a, b, c, d);
            vec3 ctr =  computeTetraCentroid(a, b, c, d);
            volume += vol;
            center += ctr*vol;
        }
    }
    return center/volume;

}

// Moves this shape so the origin aligns with the centroid and returns the move that was made
glm::vec3 ConvexShape::centerOnCentroid(){
    vec3 centroid = getCentroid();
    for(vec3& v : vertex){
        v-=centroid;
    }
    return -centroid;
}

float ConvexShape::getVolume(){
    // Get a point on the inside
    vec3 inner_point ;
    for(const auto& v : vertex){
        inner_point+=v;
    }
    inner_point/=vertex.size();
    vec3 centroid(0,0,0);
    float volume = 0 ;
    for(const auto& face : faces){
        for(int k=1; k < face.size()-1;k++){
            vec3& a = vertex[face[0]];
            vec3& b = vertex[face[k]]
            vec3& c = vertex[face[k+1]]
            vec3& d = inner_point ;
            volume += computeTetraVolume(a, b, c, d);
        }
    }
    return volume;
}

// Returns the inertia tensor of the entire shape about the origin assuming a uniform density of 1
glm::mat3 ConvexShape::getInertia(){
    // Get a point on the inside
    vec3 inner_point ;
    for(const auto& v : vertex){
        inner_point+=v;
    }
    inner_point/=vertex.size();
    glm::mat3 inertia = glm::mat3(0);
    float volume = 0 ;
    for(const auto& face : faces){
        for(int k=1; k < face.size()-1;k++){
            vec3& a = vertex[face[0]];
            vec3& b = vertex[face[k]]
            vec3& c = vertex[face[k+1]]
            vec3& d = inner_point ;
            inertia += computeTetraInertia(a, b, c, d);
        }
    }
    return inertia;
}
// return the volume of the given tetrahedron
static float computeTetraVolume(const vec3& a, const vec3& b, const vec3& c, const vec3& d){
    return abs(glm::dot(a-d, glm::cross(b-d,c-d))/6.0f ;

}

// Returns the center of mass of the given tetrahedron
static vec3 computeTetraCentroid(const vec3& a, const vec3& b, const vec3& c, const vec3& d){
    return (a+b+c+d)/4.0f;
}

// Returns the inertia tensor of the given tetrahedron about the origin assuming a uniform density of 1
static glm::mat3 ConvexShape::computeTetraInertia(const vec3& a, const vec3& b, const vec3& c, const vec3& d){
    // pulled from "explicit Eaxt Formulas for the 3D tetrahedron inertia tensor in terms of vertex coordinates"
    // by F. Tonon, Journal of Mathematics and Statistics
    float detj = 6*computeTetraVolume(a, b, c, d);

    float a = 
    //TODO
    return glm::mat3(1);
}



// Returns a shape for an axis aligned bounding box
static ConvexShape ConvexShape::makeAxisAlignedBox(glm::vec3 min, glm::vec3 max){
    vector<vec3> vertices;
    vertices.emplace_back(min.x, min.y, min.z); // 0
    vertices.emplace_back(max.x, min.y, min.z); // 1
    vertices.emplace_back(min.x, max.y, min.z); // 2
    vertices.emplace_back(max.x, max.y, min.z); // 3
    vertices.emplace_back(min.x, min.y, max.z); // 4
    vertices.emplace_back(max.x, min.y, max.z); // 5
    vertices.emplace_back(min.x, max.y, max.z); // 6
    vertices.emplace_back(max.x, max.y, max.z); // 7

    vector<vector<int>> faces;
    faces.emplace_back(vector<int>({0, 4, 6, 2})); // min x
    faces.emplace_back(vector<int>({1, 3, 7, 5})); // max x
    faces.emplace_back(vector<int>({0, 1, 5, 4})); // min y
    faces.emplace_back(vector<int>({2, 6, 7, 3})); // max y
    faces.emplace_back(vector<int>({0, 2, 3, 1})); // min z
    faces.emplace_back(vector<int>({4, 5, 7, 6})); // max z
    return ConvexShape(vertices, faces);
}

//Alternate form that always centers on the origin
static ConvexShape ConvexShape::makeAxisAlignedBox(glm::vec3 size){
    return makeAxisAlignedBox(size * -0.5f, size * 0.5f);
}

// Returns a shape for a sphere with 8*(4^detail) triangles
static ConvexShape ConvexShape::makeSphere(glm::vec3 center, float radius, int detail){
    //Start with an octahedron
    vector<vec3> vertices;
    vertices.emplace_back(center.x, center.y - radius, center.z); // 0
    vertices.emplace_back(center.x - radius, center.y, center.z); // 1
    vertices.emplace_back(center.x, center.y, center.z - radius); // 2
    vertices.emplace_back(center.x, center.y + radius, center.z); // 3
    vertices.emplace_back(center.x + radius, center.y, center.z); // 4
    vertices.emplace_back(center.x, center.y, center.z + radius); // 5

    vector<vector<int>> faces;
    faces.emplace_back(vector<int>({0, 1, 2}));
    faces.emplace_back(vector<int>({0, 2, 4}));
    faces.emplace_back(vector<int>({1, 0, 5}));
    faces.emplace_back(vector<int>({5, 0, 4}));
    faces.emplace_back(vector<int>({1, 5, 3}));
    faces.emplace_back(vector<int>({5, 4, 3}));
    faces.emplace_back(vector<int>({2, 1, 3}));
    faces.emplace_back(vector<int>({4, 2, 3}));

    vector<Face> new_faces;

    for (int det = 0; det < detail; det++) { // For each level of detail
        // split every triangle into 4 triangles
        for (auto &face : faces) {
            Face t = face;
            vec3 clr = face.color;
            int a = t.index[0], b = t.index[1], c = t.index[2];
            int d = vertices.size();
            vertices.push_back(makeSpherePoint(vertices[a], vertices[b], center, radius));
            int e = vertices.size();
            vertices.push_back(makeSpherePoint(vertices[b], vertices[c], center, radius));
            int f = vertices.size();
            vertices.push_back(makeSpherePoint(vertices[c], vertices[a], center, radius));
            new_faces.emplace_back(vector<int>({d, e, f}));
            new_faces.emplace_back(vector<int>({a, d, f}));
            new_faces.emplace_back(vector<int>({f, e, c}));
            new_faces.emplace_back(vector<int>({d, b, e}));
        }
        faces = new_faces;
        new_faces.clear();
    }

    return ConvexShape(vertices, faces);
}

// Helper method for sphere extrapolation
// Averages two points then pushes result to lie on the sphere.
static glm::vec3 ConvexShape::makeSpherePoint(glm::vec3 a, glm::vec3 b, glm::vec3 center, float radius){
    vec3 avg = (a + b) * 0.5f;
    vec3 rel = avg - center;
    float s = radius / glm::length(rel);
    return center + (rel * s);
}

// Returns a shapefor a cylinder with center of ends and A and B
static ConvexShape ConvexShape::makeCylinder(glm::vec3 A, glm::vec3 B, float radius, int sides){
    vec3 Z = B - A; // get axis_ along cylinder ((0,0,0) = A, (0,0,1) = B)
    vec3 X = glm::normalize(glm::cross(vec3(1, .8, .7), Z)) *
             radius; // Get an arbitrary axis orthogonal to Z
    vec3 Y = glm::normalize(glm::cross(X, Z)) * radius; // Get final axis
    vector<vec3> vertices;
    vector<Face> faces;
    vector<int> top, bottom;
    const float twopi = 6.28318530718;
    for (int side = 0; side < sides; side++) {
        float angle = side * twopi / sides;
        float dx = sin(angle);
        float dy = cos(angle);
        vertices.push_back(A + X * dx + Y * dy);
        vertices.push_back(B + X * dx + Y * dy);
        faces.emplace_back(
                vector<int>({2 * side + 1, 2 * side, (2 * side + 2) % (sides * 2), (2 * side + 3) % (sides * 2)}));
        top.push_back(side * 2 + 1);
        bottom.push_back((sides - 1 - side) * 2); // flip order for bottom face
    }
    //Top and bottom face
    faces.emplace_back(top);
    faces.emplace_back(bottom);
    return ConvexShape(vertices, faces);
}
