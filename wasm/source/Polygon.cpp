#include "Polygon.h"

#include <stdio.h>
#include <algorithm>
#include <functional>

using glm::dvec3 ;
using std::vector ;

Polygon::Polygon(){}

Polygon::Polygon(std::vector<glm::dvec3> np){
    p = np;
}

// Returns N and d such that N*x+d = 0 for plane and N points out of shape
void Polygon::setPlane(){
    dvec3 AB = p[1] - p[0];
    dvec3 AC = p[2] - p[0];
    dvec3 N = glm::normalize(glm::cross(AB,AC)) ;
    double d = -glm::dot(N, p[0]);
    my_plane = std::make_pair(N, d);
}

// Returns two polygons of this polygon split on the given plane
// Note: either polygon may be empty
std::pair<Polygon, Polygon> Polygon::splitOnPlane(const std::pair<glm::dvec3, double>& plane) const{
    const dvec3& N = plane.first;
    double d = plane.second;
    //printf("N: %f, %f, %f  d: %f\n", N.x, N.y, N.z, d);
	vector<dvec3> above ; // points above
	vector<dvec3> below ; // points below
    bool has_inside = false;
    bool has_outside = false;
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
            has_inside = true;
		}else{
			above.push_back(a);
            has_outside = true;
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
    if(has_inside && has_outside){ // actually getting a spli
        auto pair = std::make_pair(Polygon(below), Polygon(above));
        pair.first.my_plane = my_plane ; // copy plane to prevent it from changing due ot rounding
        pair.second.my_plane = my_plane ;
        pair.first.on_last_plane = false;
        pair.second.on_last_plane = false;
        return pair ;
    }else if(!has_inside && !has_outside){ // Lies on plane, return on both sides
        //printf("  on plane%f to %f \n", mind, maxd);
        auto pair = std::pair<Polygon, Polygon>( 
            *this,
            *this
        ) ;
        pair.first.on_last_plane = true;
        pair.second.on_last_plane = true;
        return pair;
    }else if(has_inside){
        auto pair = std::pair<Polygon, Polygon>( 
            *this,
            Polygon()
        ) ;
        pair.first.on_last_plane = false;
        pair.second.on_last_plane = true;
        return pair;
    }else{
        auto pair = std::pair<Polygon, Polygon>( 
            Polygon(),
            *this
        ) ;
        pair.first.on_last_plane = true;
        pair.second.on_last_plane = false;
        return pair;
    }
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

std::pair<std::vector<Polygon>, std::vector<Polygon>> Polygon::splitOnPlane(std::vector<Polygon>& surface, const std::pair<glm::dvec3, double>& plane){
    vector<Polygon> left_surface;
    vector<Polygon> right_surface;
    vector<dvec3> plane_points;
    dvec3 plane_center(0,0,0);
    int center_amount = 0 ;
    for(int k=0;k<surface.size();k++){
        // Split the existing surfaces on the split plane.
        std::pair<Polygon, Polygon> split_surface = surface[k].splitOnPlane(plane);
        //printf("split: %d -> %d + %d\n", (int)surface[k].p.size(),(int)split_surface.first.p.size(),(int)split_surface.second.p.size());

        if(split_surface.first.p.size() > 2){
            left_surface.push_back(split_surface.first);
        }
        if(split_surface.second.p.size() >2){
            right_surface.push_back(split_surface.second);
        }
        // face actually crosses plane
        if(split_surface.first.p.size() > 2 && split_surface.second.p.size() > 2){
            // Collect the points of the boundary that intersect the split plane
            vector<dvec3> plane_hits = surface[k].getPlaneIntersections(plane);
            for(dvec3& p : plane_hits){
                plane_points.push_back(p);
                plane_center += p;
                center_amount++;
            }
        }
    }

    if(right_surface.size() <= 1){
        return std::pair<std::vector<Polygon>, std::vector<Polygon>>(surface, std::vector<Polygon>());
    }
    if(left_surface.size() <= 1){
        return std::pair<std::vector<Polygon>, std::vector<Polygon>>(std::vector<Polygon>(), surface);
    }
    // If it has both surfaces then make polygon for surface intersection

    // Create a center and axis around which to sort plane points
    plane_center /= center_amount;
    dvec3 x_axis(0.2,0.3,0.4);
    dvec3 y_axis = glm::normalize(glm::cross(x_axis, plane.first));
    x_axis = glm::normalize(glm::cross(y_axis, plane.first)) ;

    vector<SortablePoint> sp ;
    for(dvec3& p : plane_points){
        dvec3 d = p-plane_center;
        double x = glm::dot(x_axis, d);
        double y = glm::dot(y_axis, d);
        sp.push_back(Polygon::SortablePoint{x, y, p});
    }

    std::sort(sp.begin(), sp.end(), Polygon::radialSort);
    plane_points = vector<dvec3>();
    plane_points.push_back(sp[0].p);
    for(int k=1;k<sp.size();k++){
        dvec3 d = sp[k].p-sp[k-1].p;
        if(glm::dot(d,d) > Polygon::EPSILON*Polygon::EPSILON){ // skip duplicates
            plane_points.push_back(sp[k].p);
        }
    }

    Polygon plane_poly (plane_points);
    plane_poly.my_plane = plane ;
    left_surface.push_back(plane_poly);
    right_surface.push_back(plane_poly);

    return std::pair<std::vector<Polygon>, std::vector<Polygon>>(left_surface, right_surface);
}


// A Comparator to sort points into a clean winding order 
bool Polygon::radialSort(const Polygon::SortablePoint& a, const Polygon::SortablePoint& b){
    if (a.x >= 0 && b.x< 0){
        return true;
    }
    if (a.x < 0 && b.x >= 0){
        return false;
    }
    if (a.x == 0 && b.x  == 0) {
        if (a.y >= 0 || b.y  >= 0){
            return a.y > b.y ? true: false;
        }
        return b.y > a.y ? true :false;
    }

    // compute the cross product of the relative vectors 
    double det = a.x  * b.y  - b.x * a.y;
    if (det < 0)
        return true;
    return false;
}

bool Polygon::checkConvex(std::vector<Polygon>& surface){
    vector<dvec3> all_points ;
    for(auto& s : surface){
        for(auto& p : s.p){
            all_points.push_back(p);
        }
    }

    //printf("points: %d \n", (int)all_points.size());
    
    for(auto& s : surface){
        dvec3 N = s.my_plane.first;
        double d = s.my_plane.second ;
        s.setPlane();
        if(glm::dot(N,s.my_plane.first) < 0){
            s.my_plane.first*=-1;
            s.my_plane.second*=-1;
        }

        /*
        if(glm::length(s.my_plane.first-N) > EPSILON || fabs(s.my_plane.second - d) > EPSILON){
            printf ("Surface plane incorrect. Saved: (%f,%f,%f)x + %f  Calculated: (%f,%f,%f)x + %f\n", N.x, N.y, N.z, d, s.my_plane.first.x, s.my_plane.first.y,s.my_plane.first.z, s.my_plane.second);
            return false;
        }
        */
        bool front = false;
        bool back = false ;
        for(auto& p : all_points){
            double sd = glm::dot(N,p) + d ;
            //printf("sd: %f\n", sd);
            front = front || (sd > EPSILON) ;
            back = back || sd < -EPSILON ;
        }

        if(front && back){
            printf("Convex surface is not convex!\n");
            return false;
        }

        if(!front && !back){
            printf("Convex surface is flat!\n");
            return false;
        }
    }
    return true ;

}


// Returns all closed surfaces as separate polygon lists so BSP trees can be built from them
// Mesh is assumed toi be made of traingles with correct winding order
//triangles not in closed surfaces will be discarded
std::vector<std::vector<Polygon>> Polygon::collectClosedSurfaces(std::shared_ptr<GLTF> mesh){
    //TODO
    return vector<vector<Polygon>>();
}