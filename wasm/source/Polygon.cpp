#include "Polygon.h"

#include <stdio.h>

using glm::dvec3 ;
using std::vector ;

Polygon::Polygon(){}

Polygon::Polygon(std::vector<glm::dvec3> np){
    p = np;
}

// Returns N and d such that N*x+d = 0 for plane and N points out of shape
std::pair<glm::dvec3, double> Polygon::getPlane() const{
    dvec3 AB = p[1] - p[0];
    dvec3 AC = p[2] - p[0];
    dvec3 N = glm::normalize(glm::cross(AB,AC)) ;
    double d = -glm::dot(N, p[0]);
    return std::make_pair(N, d);
}

// Returns two polygons of this polygon split on the given plane
// Note: either polygon may be empty
std::pair<Polygon, Polygon> Polygon::splitOnPlane(const std::pair<glm::dvec3, double>& plane) const{
    const dvec3& N = plane.first;
    double d = plane.second;
    //printf("N: %f, %f, %f  d: %f\n", N.x, N.y, N.z, d);
	vector<dvec3> above ; // points above
	vector<dvec3> below ; // points below
    bool has_below = false;
    bool has_above = false;
	for(int k = 0 ;k < p.size(); k++){
        //printf("p: %f, %f, %f\n", p[k].x, p[k].y, p[k].z);
		const dvec3 &a = p[k] ;
		const dvec3 &b = p[(k+1)%p.size()];
		double ad = glm::dot(N, a) + d ;
		
		// Add A
		if(fabs(ad) < EPSILON){ // if on plane
			below.push_back(a);
			above.push_back(a);
		}else if(ad < 0){
			below.push_back(a);
            has_below = true;
		}else{
			above.push_back(a);
            has_above = true;
		}
		
		double  bd = glm::dot(N, b) + d ;
		if(ad*bd < 0){ // if next line crosses plane
			ad = fabs(ad);
			bd = fabs(bd);
            if(ad > EPSILON && bd > EPSILON){ // skip adding new points if one is already on the plane
                double td = bd + ad;	
                dvec3 np = (a*bd + b*ad)/td ;		
                above.push_back(np); // Add to both polygons
                below.push_back(np);
            }
		}
	}
    // If just an edge then it is nothing
    if(!has_below){
        below = vector<dvec3>(); 
    }
    if(!has_above){
        above= vector<dvec3>(); 
    }
   
	return std::make_pair(Polygon(below), Polygon(above));
}

std::vector<glm::dvec3> Polygon::getPlaneIntersections(const std::pair<glm::dvec3, double>& plane) const{
    std::vector<glm::dvec3> plane_points; 
    const dvec3 &N = plane.first;
    const double &d = plane.second;
    int prev_index =  p.size()-1 ;
    double prev_signed_distance = glm::dot(p[prev_index],N)+d ;
    double signed_distance;
    for(int k=0; k<p.size(); k++){
      signed_distance = glm::dot(p[k],N)+d ;
      if(fabs(signed_distance)<EPSILON){//if point on plane
        plane_points.push_back(p[k]);
      }else if((signed_distance < -EPSILON && prev_signed_distance > EPSILON) || //if crosses from front to back
          (signed_distance > EPSILON && prev_signed_distance < -EPSILON)  ){ //if crosses from back to front
        const dvec3 &a = p[prev_index];
        const dvec3 &b = p[k];
        //use similar triangles to find parameter of intersection point
        double am = fabs(signed_distance)/(fabs(prev_signed_distance) + fabs(signed_distance)) ;
        double bm = 1 - am ;
        dvec3 np = a*am + b*bm ;//calculate the intersection point with the plane
		plane_points.push_back(np);
      }
      prev_signed_distance = signed_distance ;
      prev_index = k ;
    }
    return plane_points;
}

glm::dvec3 Polygon::getCenter() const{
    dvec3 total(0,0,0);
    for( auto a : p){
        total +=a;
    }
    return total/(double)p.size();
}