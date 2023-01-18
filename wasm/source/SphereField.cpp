#include "SphereField.h"

#include <stdio.h>
#include <math.h>

using glm::vec3 ;

SphereField::SphereField(glm::vec3 sphere_center, float sphere_radius, int q){
    center = sphere_center;
    radius = sphere_radius;
    quality = q ;
    texture_width = 4 * (1<<q)*(1<<q) ;
    texture.makeFillableByteArray(texture_width*texture_width*3);
}

// Add a ray to the field (overwriting any in the same location)
void SphereField::setRay(const glm::vec3 &p, const glm::vec3 &v, const glm::vec3 &color){
    auto [tx,ty] = getTextureCoordinates(p, v);
    tx = std::min(std::max(0,tx),texture_width-1);
    ty = std::min(std::max(0,ty),texture_width-1);
    byte *pixels = texture.getByteArray();
    int i = (ty * texture_width + tx)*3 ;
    pixels[i] = (byte)(color.r *255);
    pixels[i+1] = (byte)(color.g *255);
    pixels[i+2] = (byte)(color.b *255);
}

// get the color of a ray
glm::vec3 SphereField::getColor(const glm::vec3 &p, const glm::vec3 &v){
    auto [tx,ty] = getTextureCoordinates(p, v);
    tx = std::min(std::max(0,tx),texture_width-1);
    ty = std::min(std::max(0,ty),texture_width-1);
    byte *pixels = texture.getByteArray();
    int i = (ty * texture_width + tx)*3 ;
    return glm::vec3(pixels[i]/255.0f, pixels[i+1]/255.0f, pixels[i+2]/255.0f);
}

// get the texture coordinates of a given ray byu mapping its start and end onto the sphere
std::pair<int, int> SphereField::getTextureCoordinates(const glm::vec3 &p, const glm::vec3 &v){
    auto [r0,r1] = getRaySphereHits(p, v);
    return std::pair<int,int>(getTextureCoordinate(r0), getTextureCoordinate(r1)) ;
}

// returns a texture coordinate mapping the given point o nthe sphere to a single number
int SphereField::getTextureCoordinate(const glm::vec3 &rw){
    vec3 r = (rw-center)/radius; 
    // Get triangle of octahedron this sphere point falls under
    vec3 ax = vec3(1.0f,0,0);
    int b0 = 1 ;
    if(r.x < 0){
        ax.x = -1.0f;
        b0 = 0 ;
    }
    vec3 bx = vec3(0, 1.0f,0);
    int b1 = 1 ;
    if(r.y < 0){
        bx.y = -1.0f;
        b1 = 0 ;
    }
    vec3 cx = vec3(0,0,1.0f);
    int b2 = 1 ;
    if(r.z < 0){
        cx.z = -1.0f;
        b2 = 0 ;
    }
    // get barycentric coordinates
    float ab = glm::length(glm::cross(cx-bx, r-bx)) ;
    float bb = glm::length(glm::cross(cx-ax, r-ax)) ;
    float cb = glm::length(glm::cross(bx-ax, r-ax)) ;
    float total_area = ab+bb+cb;
    ab/=total_area;
    bb/=total_area;
    cb/=total_area;

    float u = ab;
    float v = bb;
    // use first bit to flip triangle over diagonal, so space isn't wasted since u+v < 1
    if(b0 > 0){
        u = 1.0f-u;
        v= 1.0f-v ;
    }
    int ui = (int)(u*(1<<quality));
    int vi = (int)(v*(1<<quality));   
    return b1 | (b2 << 1) | (ui << 2) | (vi << (2 + quality)) ;
}

// Get a ray that would generate the given texture coordinates
std::pair<glm::vec3,glm::vec3> SphereField::getRayFromTextureCoordinates(const int tx, const int ty){
    //TODO
    printf("getRayFromTextureCoordinates not implemented!\n");
    return std::pair<vec3,vec3>(vec3(0,0,0), vec3(0,0,0));
}

// Get the points on the sphere where the ray intesects
std::pair<glm::vec3,glm::vec3> SphereField::getRaySphereHits(const glm::vec3 &p, const glm::vec3 &v){
    float a = glm::dot(v,v);
    float b = glm::dot(v, p-center) * 2.0f;
    float c = glm::dot(center,center) + glm::dot(p,p) - radius*radius - glm::dot(p,center) * 2.0f;

    float inner = b*b - 4*a*c ;
    if(inner < 0){
        //TODO signal miss better
        std::pair<vec3,vec3>(vec3(0,0,0), vec3(0,0,0));
    }
    float t1 = (-b - sqrt(inner))/ (2.0f*a);
    float t2 = (-b + sqrt(inner))/ (2.0f*a);
    return std::pair<vec3,vec3>(p + v*t1, p + v*t2);
}

