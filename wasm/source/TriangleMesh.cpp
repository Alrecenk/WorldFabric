#include "TriangleMesh.h"
#include "Variant.h"

#include <math.h>
#include <map>
#include <vector>
#include <string>
#include <limits>
#include <stdlib.h>

using std::string;
using std::vector;
using std::map;
using glm::vec3;
typedef TriangleMesh::Triangle Triangle;


// Constructor
TriangleMesh::TriangleMesh(){
    this->num_vertices = 0;
    this->num_triangles = 0;
    this->color_changed = false;
    this->vertices_changed = false;
}

//Destructor
TriangleMesh::~TriangleMesh(){
}

// Returns a Variant of 3 vec3's'for each each triangle dereferenced
// Used to get arrays for displaying with a simple shader
Variant TriangleMesh::getFloatBuffer(std::vector<glm::vec3>& point_list){
    Variant buffer;
    //TODO this access pattern prevents these Variant members from being private 
    // but using the constructor forces a copy that isn't necesarry 
    // Maybe here should be an array constructor that just takes type and size
    // and then the get array provides a shallow pointer that can't be freed?
    buffer.type_ = Variant::FLOAT_ARRAY;
    buffer.ptr = (byte*)malloc(4 + this->num_triangles * 9 * sizeof(float));
    
    *((int*)buffer.ptr) = this->num_triangles * 9 ;// number of floats in array
    float* buffer_array =  (float*)(buffer.ptr+4) ; // pointer to start of float array
    for(int k=0;k<this->num_triangles;k++){
        Triangle& t = this->triangles[k];
        int k9 = k*9; // does this really matter?
        // A
        vec3& A = point_list[t.A];
        buffer_array[k9] = A.x;
        buffer_array[k9+1] = A.y;
        buffer_array[k9+2] = A.z;
        // B
        vec3& B = point_list[t.B];
        buffer_array[k9+3] = B.x;
        buffer_array[k9+4] = B.y;
        buffer_array[k9+5] = B.z;
        // C
        vec3& C = point_list[t.C];
        buffer_array[k9+6] = C.x;
        buffer_array[k9+7] = C.y;
        buffer_array[k9+8] = C.z;
    }
    return buffer ;
}

// Returns a Variant of openGL triangle buffers for displaying this mesh_ in world_ space
// result["position"] = float array of triangle vertices in order (Ax,Ay,Az,Bx,By,Bz,Cx,Cy,Cz)
// result["normal] = same format for vertex normals
// result["color"] = same format for colors but RGB floats from 0 to 1
// result["vertices"] = number of vertices
Variant TriangleMesh::getChangedBuffers(){
    std::map<string, Variant> buffers;
    if(vertices_changed){
        buffers["position"] = this->getFloatBuffer(this->vertices);
        buffers["normal"] = this->getFloatBuffer(this->normals);
        vertices_changed = false;
    }
    if(color_changed){
        buffers["color"] = this->getFloatBuffer(this->colors);
        color_changed = false;
    }
    buffers["vertices"] = Variant(num_triangles * 3);
    return Variant(buffers);
}

void TriangleMesh::setModel(const std::string &STL_text, float model_height){
    std::string::size_type pos=0;
    const string vertex_tag = "vertex";
    int v_index=0;
    vector<vec3> new_vertices;
    vector<Triangle> new_triangles;
    vec3 center = vec3{0,0,0};
    float min_z=99999.0, max_z=-99999.0;
    while(pos < STL_text.length()){
        std::string::size_type v_pos = STL_text.find(vertex_tag, pos);
        if(v_pos > STL_text.length()){// Not found
            break;
        }
        pos = v_pos + vertex_tag.length()+1;
        std::string::size_type n_pos = STL_text.find(" ", pos);
        float x = stof(STL_text.substr(pos, n_pos-pos));
        pos = n_pos+1 ;
        n_pos = STL_text.find(" ", pos);
        float y = stof(STL_text.substr(pos, n_pos-pos));
        pos = n_pos+1 ;
        n_pos = STL_text.find(" ", pos);
        float z = stof(STL_text.substr(pos, n_pos-pos));
        pos = n_pos+1 ;
        new_vertices.push_back(vec3{x,y,z});
        if(v_index%3 == 2){
            new_triangles.push_back(Triangle{v_index-2, v_index-1, v_index});
        }
        v_index++;
        center.x+=x;
        center.y+=y;
        center.z+=z;
        if(z<min_z)min_z=z;
        if(z>max_z)max_z=z;
    }
    center.x /= new_vertices.size() ;
    center.y /= new_vertices.size() ;
    center.z /= new_vertices.size() ;
    float scale = model_height / (max_z-min_z);
    for(int k=0;k<new_vertices.size();k++){
        new_vertices[k].x = (new_vertices[k].x-center.x)*scale;
        new_vertices[k].y = (new_vertices[k].y-center.y)*scale;
        new_vertices[k].z = (new_vertices[k].z-center.z)*scale;
    }
    
    setModel(new_vertices, new_triangles);

}


// Compacts the given vertices and sets the model to them
void TriangleMesh::setModel(std::vector<vec3> vertices, std::vector<Triangle> triangles){

    vector<vec3> new_vertices;
    vector<Triangle> new_triangles;

    map<int, int> unique_vertices ; // map hashes to new vertex indices

    for(int k=0;k<vertices.size();k++){
        int hash = hashVertex(vertices[k]);
        if(unique_vertices.find(hash) == unique_vertices.end()){
            unique_vertices[hash] = new_vertices.size() ;
            new_vertices.push_back(vertices[k]);
        }
    }
    for(int k=0;k<triangles.size();k++){
        new_triangles.push_back(Triangle{
            unique_vertices[hashVertex(vertices[triangles[k].A])],
            unique_vertices[hashVertex(vertices[triangles[k].B])],
            unique_vertices[hashVertex(vertices[triangles[k].C])]
        });
    }

    this->vertices_changed = true;
    this->color_changed = true;

    this->num_vertices = new_vertices.size();
    this->num_triangles = new_triangles.size();
    this->vertices = new_vertices;
    this->triangles = new_triangles;

     // Set all colors to white and Initialize Normals
    
    this->colors = vector<glm::vec3>();
    this->normals = vector<glm::vec3>();
    for(int k=0; k<this->num_vertices; k++){
        this->colors.push_back(vec3{1.0f, 1.0f, 1.0f}) ;
        this->normals.push_back(vec3{0.0f, 0.0f, 0.0f});
    }
    
    // get normals by summing up touching triangles
    for(int k=0; k < this->num_triangles; k++){
        Triangle& t = this->triangles[k] ;
        vec3 n = this->getNormal(t);
        this->normals[t.A] += n ;
        this->normals[t.B] += n ;
        this->normals[t.C] += n ;
    }
    //normalize normals
    for(int k=0;k<this->num_vertices;k++){
        this->normals[k] = glm::normalize(this->normals[k]) ;
    }
    
    //update AABB
    this->min = {9999999,9999999,9999999};
    this->max = {-9999999,-9999999,-9999999};
    for(int k=0;k<this->num_vertices; k++){
        auto &v = this->vertices[k];
        if(v.x < this->min.x)this->min.x = v.x;
        if(v.y < this->min.y)this->min.y = v.y;
        if(v.z < this->min.z)this->min.z = v.z;
        if(v.x > this->max.x)this->max.x = v.x;
        if(v.y > this->max.y)this->max.y = v.y;
        if(v.z > this->max.z)this->max.z = v.z;
    }
}

// hashes a vertex to allow duplicates ot be detected and merged
int TriangleMesh::hashVertex(vec3 v){
    return Variant(v).hash();
}

vec3 TriangleMesh::getNormal(Triangle t){
    vec3 AB = this->vertices[t.B] - this->vertices[t.A];
    vec3 AC = this->vertices[t.C] - this->vertices[t.A];
    return glm::normalize(glm::cross(AB,AC));
}

// Given a ray in model space (p + v*t) return the t value of the nearest collision
// with the given triangle
// return negative if no collision
float EPSILON = 0.00001;
float TriangleMesh::trace(Triangle tri, const vec3 &p, const vec3 &v){
    vector<vec3>& x = this->vertices ;
    //TODO use the vector library
    vec3 AB = {x[tri.B].x-x[tri.A].x,x[tri.B].y-x[tri.A].y,x[tri.B].z-x[tri.A].z};
    vec3 AC = {x[tri.C].x-x[tri.A].x,x[tri.C].y-x[tri.A].y,x[tri.C].z-x[tri.A].z};
    // h = v x AC
    vec3 h = {v.y*AC.z - v.z*AC.y,
              v.z*AC.x - v.x*AC.z,
              v.x*AC.y - v.y*AC.x};
    // a = AB . h
    float a = AB.x*h.x + AB.y*h.y + AB.z*h.z;
	if (a < EPSILON)
      return -1;
	float f = 1.0 / a;
    // s = p - A
    vec3 s = {p.x-x[tri.A].x,p.y-x[tri.A].y,p.z-x[tri.A].z};
    // u = s.h * f
    float u = (s.x*h.x+s.y*h.y+s.z*h.z) * f;
	if (u < 0 || u > 1)
      return -1;
	// q = s x AB
    vec3 q = {s.y*AB.z - s.z*AB.y,
              s.z*AB.x - s.x*AB.z,
              s.x*AB.y - s.y*AB.x};
    // w = v.q * f
    float w = (v.x*q.x+v.y*q.y+v.z*q.z) * f;
	if (w < 0 || u + w > 1)
      return -1;
    // t = AC.q * f
    float t = (AC.x*q.x+AC.y*q.y+AC.z*q.z) * f;
    if( t > EPSILON){
      return t;
    }else{
      return -1;
    }

}

// Given a ray in model space (p + v*t) return the t value of the nearest collision
// return negative if no collision
float TriangleMesh::rayTrace(const vec3 &p, const vec3 &v){
    float min_t = std::numeric_limits<float>::max() ;
    for(int k=0;k<this->num_triangles;k++){
        float t = this->trace(this->triangles[k],p,v);
        if(t > 0 && t < min_t){
            min_t = t ;
        }
    }
    if(min_t < std::numeric_limits<float>::max() ){
        return min_t ;
    }return -1;
}

// Changes all vertices within radius of origin to the given color
void TriangleMesh::paint(const vec3 &center, const float &radius, const vec3 &color){
    float r2 = radius*radius;
    for(int k=0; k<this->num_vertices; k++){
        vec3 d = {this->vertices[k].x-center.x,this->vertices[k].y-center.y,this->vertices[k].z-center.z};
        float d2 = d.x*d.x+d.y*d.y+d.z*d.z;
        if(d2<r2){
            // if actually changing
            if(this->colors[k].x != color.x || this->colors[k].y != color.y || this->colors[k].z != color.z ){
                this->colors[k] = color;
                this->color_changed = true;
            }
        }
    }
}