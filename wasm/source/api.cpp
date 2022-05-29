#include "Variant.h"
#include "TriangleMesh.h"
#include <stdlib.h> 
#include <stdio.h>
#include <map>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include "glm/vec3.hpp"

using std::vector;
using std::string;
using std::map;
using std::pair;
using glm::vec3;

// Outermost API holds a global reference to the core data model
TriangleMesh model_global;
int model_global_id = 1;

Variant ret ; // A long lived variant used to hold data being returned to webassembly
byte* packet_ptr ; // location ofr data passed as function parameters and returns


byte* pack(std::map<std::string, Variant> packet){
    ret = Variant(packet);
    memcpy(packet_ptr, ret.ptr, ret.getSize()); // TODO figure out how to avoid this copy
    return packet_ptr ;
}

// Convenience function that sets the return Variant to an empty map and returns it.
// "return emptyReturn();" can be used in any front-end function to return a valid empty object
byte* emptyReturn() {
    map<string, Variant> ret_map;
    return pack(ret_map);
}


extern "C" { // Prevents C++ from mangling the exported name apparently


void setPacketPointer(byte* p){
    printf("Packet Pointer allocated at: %ld\n", (long)p);
    packet_ptr = p;
    map<string, Variant> ret_map;
    pack(ret_map);
}


// Wrappers to model functions applied to model global are made available to callers
// Expects an object with vertices and faces
byte* setModel(byte* ptr){
    auto obj = Variant::deserializeObject(ptr);
    string text = obj["text"].getString();
    float model_size = obj["model_size"].getNumberAsFloat();
    model_global.setModel(text, model_size);
    return emptyReturn() ;
}

byte* getUpdatedBuffers(byte* ptr){
    map<string,Variant> buffers;
    std::stringstream ss;
    ss << model_global_id;
    string s_id = ss.str();
    buffers[s_id] = model_global.getChangedBuffers() ;
    return pack(buffers) ;
}


// expects an object with center, radius, and color as float arrays of length 3
byte* paint(byte* ptr){
    auto obj = Variant::deserializeObject(ptr);
    vec3 center = obj["center"].getVec3() ; 
    vec3 color = obj["color"].getVec3() ;
    float radius = obj["radius"].getNumberAsFloat() ;
    model_global.paint(center, radius, color);
    return emptyReturn() ;
}
//expects an object with p and v, retruns a serialized single float for t
byte* rayTrace(byte* ptr){
    auto obj = Variant::deserializeObject(ptr);
    vec3 p = obj["p"].getVec3() ;
    vec3 v = obj["v"].getVec3();
    float t = model_global.rayTrace(p, v);
    map<string, Variant> ret_map;
    ret_map["t"] = Variant(t);
    ret_map["x"] = Variant(p+v*t);
    return pack(ret_map);
}

//Allocates an array of integers of a given size and returns a pointer to it.
// Note: this is intended for testing JS to C++ pointer moves and memory limits and LEAKS MEMORY if not cleared on the JS side.
byte* testAllocate(byte* ptr){
    auto obj = Variant::deserializeObject(ptr);
    int size = obj["size"].getInt();
    int num = obj["num"].getInt();

    int* array = new int[size];
    for(int k=0;k<size;k++){
        array[k] = num + k;
    }
    printf("C++ array ptr: %ld\n", (long)array);
    map<string, Variant> ret_map;
    ret_map["ptr"] = Variant((int)array);
    ret = Variant(ret_map);
    printf("C++ ret ptr: %ld\n", (long)ret.ptr);
    auto obj2 = Variant::deserializeObject(ret.ptr);
    printf("C++ deserialized array ptr: %ld\n", (long)obj2["ptr"].getInt());
    return pack(ret_map);

}

}// end extern C