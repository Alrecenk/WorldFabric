#include "Hologram.h"

#include <stdio.h>
#include <math.h>
#include <queue>

using glm::vec3 ;
using glm::mat4;
using glm::mat3;
using glm::vec4;
using glm::vec2;
using std::string;
using std::map;
using std::vector;

Hologram::Hologram(){
}


// Adds a new panel for determining depth of view rays
void Hologram::addPanel(const HologramPanel& p){
    panel.push_back(p);
    printf("Panel added!\n");
}

// Constructs and adds a new panel
void Hologram::addPanel(glm::vec3 zero, glm::vec3 xperpixel, glm::vec3 yperpixel , glm::vec3 z_normal, std::vector<std::vector<float>>& depth, int block_size){
    panel.emplace_back(zero, xperpixel, yperpixel, z_normal);
    panel[panel.size()-1].setDepth(depth);
    panel[panel.size()-1].buildBlockImage(block_size);
    printf("Panel added!\n");
}

// Adds a new image view which will be reprojected to produce output colors
void Hologram::addView(const HologramView& v){
    view.push_back(v);
    printf("View added!\n");
}

// Constructs and adds a new view
// Note: image_data will be moved and passed in Variant will be destroyed
void Hologram::addView(glm::vec3 p, glm::vec3 n , const std::vector<std::pair<glm::vec3, glm::vec2>> &v_to_pixel, Variant& image_data, int w, int h, int c){
    view.emplace_back(p,n,v_to_pixel);
    view[view.size()-1].moveImage(image_data, w, h, c);
    printf("View added!\n");
}

// get the point a ray would hit by intersecting it with the appropriate panel
glm::vec3 Hologram::getPoint(const glm::vec3 &p, const glm::vec3 &v){
    std::priority_queue<std::pair<float, int>> best_panels ;
    int max_panel_blend = 3 ;
    for(int k=0;k<panel.size();k++){
        float score = panel[k].scoreAlignment(p,v);
        best_panels.push(std::pair<float, int>(score,k)) ; // we wan the worst at the top so we pop off the worst
        if(best_panels.size()>max_panel_blend){
            best_panels.pop();
        }
    }

    vec3 total_point = vec3(0,0,0) ;
    float total_weight=0;
    // weight of best hi and best miss 
    float best_miss = 0 ;
    float best_hit = 0 ;
    while(!best_panels.empty()){
        std::pair<float, int> bp = best_panels.top() ;
        best_panels.pop();
        // 1 is optimal alignment, 0 is perpendicular
        float start_blend = 0.5f;
        float alignment = -bp.first ;
        if(alignment > start_blend){ // pretty close to start blending

            float weight = (alignment-start_blend)*(alignment-start_blend) ;
            vec3 point = panel[bp.second].rayTrace(p,v) ;
            // don't blend a miss with a hit
            if(point.x == 0.0f && point.y == 0.0f && point.z == 0.0f){
                best_miss = fmax(weight,best_miss) ;
            }else{
                best_hit = fmax(weight,best_hit);
                total_point += point * weight;
                total_weight += weight;
            }
        }
    }

    // if we got no valid panels or betsscoring panel was a miss
    if(total_weight <= 0.0f || best_miss > best_hit){
        return vec3(0.0f,0.0f,0.0f); // then it was a miss
    }else{
        return total_point/total_weight; // blend hits
    }

}

// get the color of a ray
glm::vec3 Hologram::getColor(const glm::vec3 &p, const glm::vec3 &v){
    vec3 intersect = getPoint(p,v);
    if(intersect.x == 0.0f && intersect.y == 0.0f && intersect.z == 0.0f){
        return intersect ;
    }

    std::priority_queue<std::pair<float, int>> best_views ;
    int max_view_blend = 3 ;
    for(int k=0;k<view.size();k++){
        float w = view[k].blendWeight(p, intersect);
        if(w > 0){
            //if(print)printf("panel check %d score: %f\n", k, score);
            best_views.push(std::pair<float, int>(-w,k)) ; // we want the worst at the top so we pop off the worst
            if(best_views.size()>max_view_blend){
                best_views.pop();
            }
        }
    }

    vec3 total_color= vec3(0,0,0) ;
    float total_weight=0;
    while(!best_views.empty()){
        
        std::pair<float, int> bv = best_views.top() ;
        best_views.pop();
        float weight = -bv.first ;
        vec3 color = view[bv.second].getColor(intersect);
        total_color += color * weight;
        total_weight += weight;
            
    }

    if(total_weight <= 0.0f){
        return vec3(0.0f,0.0f,0.0f); // then it was a miss
    }else{
        return total_color/total_weight; // blend hits
    }
    
}

// Serialize this object
Variant Hologram::serialize(){

    
    /*
    vector<Variant> panels;
    for(int k=0;k<panel.size();k++){
        panels.push_back(panel[k].serialize());
    }
    serial["panels"] = Variant(panels);
    
    vector<Variant> views;
    for(int k=0;k<view.size();k++){
        views.push_back(view[k].serialize());
    }

    serial["views"] = Variant(views);
    
    //Variant serialized = Variant(serial) ;
    //serialized.printFormatted();
    */
    //printf("A\n");
    vector<float> floats;
    floats.reserve(10000);
    vector<int> ints;
    ints.reserve(10000);
    vector<byte> bytes ;
    bytes.reserve(10000);
    append(floats,ints,bytes);

//printf("Appended Floats:%d, Ints:%d, Bytes:%d\n", (int)floats.size(), (int)ints.size(), (int)bytes.size());
    


    /*
    map<string,Variant> serial;
    serial["floats"] = Variant(floats);
    serial["ints"] = Variant(ints);
    serial["bytes"] = Variant(bytes);
    return Variant(serial) ;
    */
    
    //return serialized;
    Variant p = pack(floats,ints,bytes) ;
    //floats.clear();
    //ints.clear();
    //bytes.clear();
/*
    printf("Packed:%d\n", p.getArrayLength());

    

    vector<float> floats3;
    vector<int> ints3;
    vector<byte> bytes3 ;
    unpack(p,floats3, ints3, bytes3);
    printf("Unpacked Floats:%d, Ints:%d, Bytes:%d\n", (int)floats3.size(), (int)ints3.size(), (int)bytes3.size());


    Hologram test ;
    test.setFrom(floats3,ints3, bytes3,0,0,0);


    vector<float> floats2;
    vector<int> ints2;
    vector<byte> bytes2 ;
    test.append(floats2,ints2,bytes2);
    bool error = false;
printf("C\n");
    if(floats.size() == floats2.size()){
        for(int k=0;k<floats.size() && !error;k++){
            if(abs(floats[k]-floats2[k])>0.001f){
                printf("float %d changed! %f != %f\n", k, floats[k], floats2[k]);
                error = true;
            }
        }
    }else{
        printf("incorrect numbr of floats %d != %d\n",(int)floats.size(), (int)floats2.size());
    }

printf("D\n");
    if(ints.size() == ints2.size()){
        for(int k=0;k<ints.size() && !error;k++){
            if(ints[k] != ints2[k]){
                printf("int changed! %d != %d\n", ints[k], ints2[k]);
                error = true ;
            }
        }
    }else{
        error = true ;
        printf("incorrect numbr of ints %d != %d\n",(int)ints.size(), (int)ints2.size());
    }
printf("E\n");
    if(bytes.size() == bytes2.size()){
        for(int k=0;k<bytes.size() && !error;k++){
            if(bytes[k] != bytes2[k]){
                printf("byte changed! %d != %d\n", bytes[k], bytes2[k]);
                error = true;
            }
        }
    }else{
        error = true ;
        printf("incorrect number of bytes %d != %d\n",(int)bytes.size(), (int)bytes2.size());
    }

    if(!error){
        printf("Packed Arrays are consistent!\n");
    }
printf("F\n");

*/
    return p;
}

// Set this object to data generated by its serialize method
void Hologram::set(Variant& serialized){
    //serialized.printFormatted();

    //map<string,Variant> serial = serialized.getObject();
    
    /*
    vector<Variant> panels = serial["panels"].getVariantArray();

    panel = vector<HologramPanel>();
    for(int k=0;k<panels.size();k++){
        panel.emplace_back(panels[k]);
    }
    
    vector<Variant> views = serial["views"].getVariantArray();
    view = vector<HologramView>();
    for(int k=0;k<views.size();k++){
        view.emplace_back(views[k]);
    }
    
    vector<float> floats = serial["floats"].clone().getFloatVector();
    vector<int> ints = serial["ints"].clone().getIntVector();
    vector<byte> bytes =  serial["bytes"].clone().getByteVector();
    */

    vector<float> floats;
    floats.reserve(10000);
    vector<int> ints;
    ints.reserve(10000);
    vector<byte> bytes ;
    bytes.reserve(10000);
    unpack(serialized,floats,ints,bytes);
    //printf("Unpacked Floats:%d, Ints:%d, Bytes:%d\n", (int)floats.size(), (int)ints.size(), (int)bytes.size());
    setFrom(floats,ints,bytes,0,0,0);

}

// Appends this hologram to the given arrays
void Hologram::append(std::vector<float>& floats, std::vector<int>& ints, std::vector<byte>& bytes){
    ints.push_back(panel.size());
    ints.push_back(view.size());
    int panel_start_ints = ints.size();
    for(int k=0;k<panel.size();k++){
        // allocate places to put ptr to panel data
        ints.push_back(0); //float ptr
        ints.push_back(0); //int ptr
        ints.push_back(0); // byte_ptr
    }
    int view_start_ints = ints.size();
    for(int k=0;k<view.size();k++){
        // allocate places to put ptr to view data
        ints.push_back(0); //float ptr
        ints.push_back(0); //int ptr
        ints.push_back(0); // byte_ptr
    }
    for(int k=0;k<panel.size();k++){
        // backfill the points to where we're putting the data
        ints[panel_start_ints+k*3] = floats.size();
        ints[panel_start_ints+k*3 + 1] = ints.size();
        ints[panel_start_ints+k*3 + 2] = bytes.size();
        panel[k].append(floats, ints, bytes);
    }
    for(int k=0;k<view.size();k++){
        // backfill the points to where we're putting the data
        ints[view_start_ints + k*3] = floats.size();
        ints[view_start_ints + k*3 + 1] = ints.size();
        ints[view_start_ints + k*3 + 2] = bytes.size();
        view[k].append(floats, ints, bytes);
    }

}

// sets this hologram read from the given arrays starting at the given indices
void Hologram::setFrom(std::vector<float>& floats, std::vector<int>& ints, std::vector<byte>& bytes, int float_ptr, int int_ptr, int byte_ptr){
    int num_panels = ints[int_ptr++];
    int num_views = ints[int_ptr++];

    panel = vector<HologramPanel>();    
    for(int k=0;k<num_panels;k++){
        int p_float = ints[int_ptr++] ;
        int p_int = ints[int_ptr++] ;
        int p_byte = ints[int_ptr++];
        panel.emplace_back(floats, ints, bytes, p_float, p_int, p_byte);
        printf("panel %d: %d, %d, %d\n", k, p_float, p_int, p_byte);
    }

    view = vector<HologramView>();
    for(int k=0;k<num_panels;k++){
        int p_float = ints[int_ptr++] ;
        int p_int = ints[int_ptr++] ;
        int p_byte = ints[int_ptr++];
        view.emplace_back(floats, ints, bytes, p_float, p_int, p_byte);
        printf("view %d: %d, %d, %d\n", k, p_float, p_int, p_byte);
    }
}

Variant Hologram::pack(std::vector<float>& floats, std::vector<int>& ints, std::vector<byte>& bytes){
    Variant total;
    int size = 12 + floats.size()*4 + ints.size()*4 + bytes.size() ;
    if(size%4!= 0){
        size+= 4-(size%4) ;
    }
    total.makeFillableByteArray(size);
    byte* ptr = total.getByteArray();
    int* i_ptr = (int*)ptr ;
    i_ptr[0] = (int)floats.size() ;
    i_ptr[1] = (int)ints.size() ;
    i_ptr[2] = (int)bytes.size() ;
    float* f_ptr = (float*)(ptr+12);
    for(int k=0;k<floats.size();k++){
        f_ptr[k] = floats[k];
    }
    i_ptr = (int*)(ptr+12 + 4*floats.size());
    for(int k=0;k<ints.size();k++){
        i_ptr[k] = ints[k];
    }
    ptr = ptr + 12 +  4*(floats.size()+ints.size());
    for(int k=0;k<bytes.size();k++){
        ptr[k] = bytes[k];
    }
    return total ;
}

void Hologram::unpack(Variant& total,std::vector<float>& floats, std::vector<int>& ints, std::vector<byte>& bytes){
    byte* ptr = total.getByteArray();
    int* i_ptr = (int*)ptr ;
    int num_floats = i_ptr[0];
    int num_ints = i_ptr[1];
    int num_bytes = i_ptr[2];
    printf("unpack floats: %d, ints: %d, bytes, %d\n", num_floats, num_ints, num_bytes);
    float* f_ptr = (float*)(ptr+12);
    //printf("1\n");
    for(int k=0;k<num_floats;k++){
        floats.push_back(f_ptr[k]) ;
    }
    //printf("2\n");
    i_ptr = (int*)(ptr+(12 + 4*num_floats));
    for(int k=0;k<num_ints;k++){
        ints.push_back(i_ptr[k]) ;
    }
    //printf("3\n");
    ptr = ptr + 12 +  4*(num_floats+num_ints);
    for(int k=0;k<num_bytes;k++){
        bytes.push_back(ptr[k]) ;
    }
    //printf("4\n");
}