#ifndef _TRIANGLE_MESH_H_
#define _TRIANGLE_MESH_H_ 1

#include "Variant.h"
#include <set>
#include "glm/glm.hpp"

typedef Variant::Type Type;

class TriangleMesh{

    public:

        struct Triangle{
            int A;
            int B;
            int C;
        };

        int num_vertices;
        int num_triangles;
        std::vector<glm::vec3> vertices ; 
        std::vector<glm::vec3> colors; 
        std::vector<glm::vec3> normals;
        std::vector<Triangle> triangles ; 
        glm::vec3 min; // minimum values in each axis part of AABB
        glm::vec3 max; // maximum values in each axis part of AABB
        bool vertices_changed = false;
        bool color_changed = false;

        // Constructor
        TriangleMesh();

        //Destructor
        ~TriangleMesh();
        
        // Returns a Variant of openGL triangle buffers for displaying this mesh_ in world_ space
        // result["position"] = float array of triangle vertices in order (Ax,Ay,Az,Bx,By,Bz,Cx,Cy,Cz)
        // result["normal] = same format for vertex normals
        // result["color"] = same format for colors but RGB floats from 0 to 1
        Variant getChangedBuffers();

        //Sets the model from an object containing {vertices:float_array vertices, faces:(int_array or short_array) triangles}
        // All colors are set to white for a new model
        void setModel(const std::string &STL_text, float model_height);

        // Compacts the given vertices and sets the model to them
        void setModel(std::vector<glm::vec3> vertices, std::vector<Triangle> triangles);

        // hashes a vertex to allow duplicates ot be detected and merged
        int hashVertex(glm::vec3 v);

        // returns the model with {vertices:float_array vertices, faces:(int_array or short_array) triangles}
        std::map<std::string,Variant> getModel();

        // Given a ray in model space (p + v*t) return the t value of the nearest collision
        // return negative if no collision
        float rayTrace(const glm::vec3 &p, const glm::vec3 &v);

        // Changes all vertices within radius of origin to the given color
        void paint(const glm::vec3 &center, const float &radius, const glm::vec3 &color);

    private:
        // Performs the duplicate work for the various get vertex buffer functions
        Variant getFloatBuffer(std::vector<glm::vec3>& ptr);

        // returns the normal of a triangle
        glm::vec3 getNormal(Triangle t);

        // Given a ray in model space (p + v*t) return the t value of the nearest collision
        // with the given triangle
        // return negative if no collision
        float trace(Triangle tri, const glm::vec3 &p, const glm::vec3 &v);

};
#endif // #ifndef _TRIANGLE_MESH_H_