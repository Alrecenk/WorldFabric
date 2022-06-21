#include "Variant.h"
#include "GLTF.h"
#include <stdlib.h> 
#include <stdio.h>
#include <map>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>
#include <memory>
#include "glm/vec3.hpp"
#include "TableReader.h"
#include "UnitTests.h"
#include "Timeline.h"
#include "TObject.h"
#include "TEvent.h"
#include "CreateObject.h"
#include "BouncingBall.h"
#include "MoveBouncingBall.h"
#include "ChangeBallVelocity.h"


using std::vector;
using std::string;
using std::map;
using std::pair;
using glm::vec3;
using glm::vec4;
using glm::mat4;
using std::unique_ptr ;
using std::make_unique ;
using std::weak_ptr;
using std::shared_ptr;

// Outermost API holds a global reference to the core data model
map<string,GLTF> meshes;
const string MAIN_MODEL = "MAIN" ;
const string HAND = "HAND";

byte* packet_ptr ; // location ofr data passed as function parameters and returns
int last_pack_size = 0 ;

int selected_animation = -1;
std::chrono::high_resolution_clock::time_point animation_start_time;

unique_ptr<Timeline> timeline ;


long timeMilliseconds() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}

float randomFloat() {
    return (float) ((rand() % 1000000) / 1000000.0);
}

byte* pack(std::map<std::string, Variant> packet){
    Variant ret = Variant(packet);
    last_pack_size = ret.getSize() ;
    memcpy(packet_ptr, ret.ptr, last_pack_size);
    return packet_ptr ;
}

// Note: Javascript deserializer expects string objects
// This is only intended for server interfaces that don't deserialize in Javascript
byte* pack(Variant packet){
    last_pack_size = packet.getSize() ;
    memcpy(packet_ptr, packet.ptr, last_pack_size);
    return packet_ptr ;
}

// Convenience function that sets the return Variant to an empty map and returns it.
// "return emptyReturn();" can be used in any front-end function to return a valid empty object
byte* emptyReturn() {
    map<string, Variant> ret_map;
    return pack(ret_map);
}

std::chrono::high_resolution_clock::time_point now(){
    return std::chrono::high_resolution_clock::now();
} 

int millisBetween(std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point end){
    return (int)(std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count()*1000);
}

Timeline* initializeBallTimeline(){
    timeline = make_unique<Timeline>(&BouncingBall::createEvent, &BouncingBall::createObject);
    timeline->auto_clear_history=true;
    return timeline.get() ;
}

void addBall(int width, int height, float min_radius, float max_radius, float max_speed){
    vec3 box_min(0,0,-10000);
    vec3 box_max(width,height,10000);
    float radius = min_radius + randomFloat()*(max_radius-min_radius);
        vec3 position(radius *2 + randomFloat()*(width-radius*4), radius * 2 + randomFloat()*(height-radius*4), 0);
        float angle = randomFloat()*6.29;
        float speed = randomFloat()*max_speed ;

        vec3 velocity(sin(angle)*speed, cos(angle)*speed,0);
        
        std::unique_ptr<BouncingBall> o = std::make_unique<BouncingBall>(position, velocity, radius, box_min, box_max) ;
        std::unique_ptr<MoveBouncingBall> o_move = std::make_unique<MoveBouncingBall>(1.0/60.0) ;
        float timeoffset = randomFloat() ;
        printf("new ball in %f\n", timeoffset);
        timeline->createObject(std::move(o), std::move(o_move) , timeline->current_time + timeoffset);

}

Timeline* initialize2DBallTimeline(int width, int height, int amount, float min_radius, float max_radius, float max_speed){
    printf("initializing timeline!\n");
    initializeBallTimeline();
    //printf("amount: %d\n", amount);
    
    for(int k=0;k<amount;k++){
        addBall(width, height, min_radius, max_radius, max_speed);
        
    }
    timeline->run(1.0);
    return timeline.get();
}


extern "C" { // Prevents C++ from mangling the exported name apparently

// Allows javascript to check the size of the return without deserializing it
int getReturnSize(byte* nothing) {
    return last_pack_size;
}

void setPacketPointer(byte* p){
    //printf("Packet Pointer allocated at: %ld\n", (long)p);
    packet_ptr = p;
    map<string, Variant> ret_map;
    pack(ret_map);
}


// Wrappers to model functions applied to model global are made available to callers
// Expects an object with vertices and faces
byte* setModel(byte* ptr){
    string select = MAIN_MODEL;
    GLTF& model = meshes[select];

    auto start_time = now();
    auto obj = Variant::deserializeObject(ptr);
    byte* byte_array = obj["data"].getByteArray();
    int num_bytes = obj["data"].getArrayLength();
    
    model.setModel(byte_array, num_bytes);

    float size = 0 ;
    vec3 center(0,0,0);
    for(int k=0;k<3;k++){
        center[k] = (model.max[k]+ model.min[k])*0.5f ;
        size = fmax(size , abs(model.max[k]-center[k]));
        size = fmax(size , abs(model.min[k]-center[k]));
        
    }

    model.transform  = glm::scale(mat4(1), {(1.0f/size),(1.0f/size),(1.0f/size)});
    model.transform  = glm::translate(model.transform, center*-1.0f);
    
    model.computeNodeMatrices();
    model.applyTransforms();
    
    //printf("Zoom:%f\n", zoom);
    map<string, Variant> ret_map;
    ret_map["center"] = Variant(center);
    ret_map["size"] = Variant(size);

    int millis = millisBetween(start_time, now());
    printf("Total model load time: %d ms\n", millis);

    GLTF& hand_model = meshes[HAND];
    hand_model.setTetraModel(vec3(0,0,0), 0.015);

    return pack(ret_map);
}



byte* getUpdatedBuffers(byte* ptr){

    if(selected_animation >= 0){
        auto& animation = meshes[MAIN_MODEL].animations[selected_animation];
        float time = millisBetween(animation_start_time, now()) / 1000.0f;
            if(time > animation.duration){
                time = 0 ;
                animation_start_time = now();
            }
        meshes[MAIN_MODEL].animate(animation,time);
    }

    map<string,Variant> buffers;
    for(auto & [name, mesh] : meshes){
        //st = now();
        if(mesh.position_changed || mesh.model_changed || mesh.bones_changed){
            for(auto const & [material_id, mat]: mesh.materials){
                std::stringstream ss;
                ss << name << material_id;
                string s_id = ss.str();
                buffers[s_id] = mesh.getChangedBuffer(material_id) ;
            }
            mesh.position_changed = false;
            mesh.model_changed = false;
            mesh.bones_changed = false;                
        }
    }
    //ms = millisBetween(st, now());
    //printf("api get all buffers: %d ms\n", ms);
    return pack(buffers) ;
}

//expects an object with p and v, retruns a serialized single float for t
byte* rayTrace(byte* ptr){
    GLTF& model = meshes[MAIN_MODEL];
    auto obj = Variant::deserializeObject(ptr);
    vec3 p = obj["p"].getVec3() ;
    vec3 v = obj["v"].getVec3();
    float t = model.rayTrace(p, v);
    map<string, Variant> ret_map;
    ret_map["t"] = Variant(t);
    ret_map["x"] = Variant(p+v*t);
    ret_map["index"] = Variant(model.last_traced_tri);
    return pack(ret_map);
}

byte* scan(byte* ptr){
    GLTF& model = meshes[MAIN_MODEL];
    auto obj = Variant::deserializeObject(ptr);
    vec3 p = obj["p"].getVec3() ;
    vec3 v = obj["v"].getVec3();

    model.applyTransforms(); // Get current animated coordinates on CPU
    float t = model.rayTrace(p, v);
    map<string, Variant> ret_map;
    ret_map["t"] = Variant(t);
    ret_map["x"] = Variant(p+v*t);
    ret_map["index"] = Variant(model.last_traced_tri);

    if(model.last_traced_tri != -1){
        GLTF::Triangle tri = model.triangles[model.last_traced_tri];
        int material_id = tri.material;
        GLTF::Material mat = model.materials[tri.material];
        printf("triangle : %d, material: %d \n" ,model.last_traced_tri, material_id);
        if(model.json["materials"][material_id].defined()){
            model.json["materials"][material_id].printFormatted();
        }
        
        GLTF::Vertex& v = model.vertices[tri.A];
        glm::ivec4 joints = v.joints;
        for(int k=0;k<4;k++){
            int n = v.joints[k];
            string name = model.nodes[n].name;
            float w = v.weights[k] ;
            if(w > 0){
                printf("Joint[%d] = %d (%s)  weight: %f\n", k, n, name.c_str(), w);
            }
        }

        printf("Texcoords: %f, %f\n" , v.tex_coord.x, v.tex_coord.y);
    }
    return pack(ret_map);
}

byte* nextAnimation(byte* ptr){
    if(millisBetween(animation_start_time, now()) < 50){
        return emptyReturn();
    }
    GLTF& model = meshes[MAIN_MODEL];
    selected_animation = selected_animation+1;
    if(selected_animation >= model.animations.size()){
        selected_animation = -1;
        model.setBasePose();
        model.computeNodeMatrices();
    }
    animation_start_time = now();
    return emptyReturn();
}


// Returns the pending table requests that require a network request to fetch
// Items which are already in the cache are given to objects when this is called
//Note: this returns a pointer to VARIANT_ARRAY data but not the type, so it cannot be deserialized by default methods that assume an OBJECT type
byte* getTableNetworkRequest(byte* nothing) {
    TableReader::serveCachedRequests();
    vector<string> requests = TableReader::getRequestedKeys();
    vector<Variant> network_request;
    network_request.reserve(requests.size());
    for (const auto &key : requests) {
        // emplace back constructs Variants from strings
        network_request.emplace_back(key);
    }
    return pack(Variant(network_request));
}

//Caches and Distributes data from a network table data response
byte* distributeTableNetworkData(byte* data_ptr) {
    map<string, Variant> data = Variant::deserializeObject(data_ptr);
    TableReader::receiveTableData(data);
    return emptyReturn();
}

byte* createPin(byte* ptr) {
    auto obj = Variant::deserializeObject(ptr);
    vec3 p = obj["p"].getVec3() ;
    string name = obj["name"].getString();
    GLTF& model = meshes[MAIN_MODEL];
    model.applyTransforms(); // Get current animated coordinates on CPU
    int vertex_index = -1 ;
    vec3 global ;
    if(obj["v"].defined()){ // we're making the pin from a ray
        vec3 v = obj["v"].getVec3();
        float t = model.rayTrace(p, v);
        if(model.last_traced_tri != -1){
            GLTF::Triangle tri = model.triangles[model.last_traced_tri]; 
            vertex_index = tri.A;
        }
        global = p + v * t;
    }else{
        vertex_index = model.getClosestVertex(p);
        global = p ;
    }
    if(vertex_index < 0){
        model.computeNodeMatrices();
        return emptyReturn();
    }
    GLTF::Vertex& vert = model.vertices[vertex_index];
    glm::ivec4 joints = vert.joints;
    float max_w = -1.0f ;
    int bone = -1;
    for(int k=0;k<4;k++){
        float w = vert.weights[k] ;
        if(w > max_w){
            max_w = w ;
            bone = vert.joints[k] ;
        }
    }

    vec3 mesh_space =   glm::inverse(model.nodes[bone].transform) * vec4(global,1) ;
    vec3 local = model.nodes[bone].mesh_to_bone * vec4(mesh_space,1) ;

    model.createPin(name, bone, local, 1.0f);
    model.applyPins();
    /*
    printf("global: %f, %f, %f\n", global.x, global.y, global.z);
    //printf("actual: %f, %f, %f\n", actual.x, actual.y, actual.z);
    printf("Mesh: %f, %f, %f\n", mesh_space.x, mesh_space.y, mesh_space.z);
    printf("Local: %f, %f, %f\n", local.x, local.y, local.z);
    printf("Vert: %f, %f, %f\n", vert.position.x, vert.position.y, vert.position.z);
    printf("Vert Transformed: %f, %f, %f\n", vert.transformed_position.x, vert.transformed_position.y, vert.transformed_position.z);
    

    double error = model.error(model.getX());
    printf("Error: %f\n", error);
    */
    printf("Pin '%s' created for %s.\n" , name.c_str(), model.nodes[bone].name.c_str());
    

    return emptyReturn();

}

byte* setPinTarget(byte* ptr) {

    auto obj = Variant::deserializeObject(ptr);
    vec3 p = obj["p"].getVec3() ;
    
    string name = obj["name"].getString();
    GLTF& model = meshes[MAIN_MODEL];
    vec3 target ;
    if(obj["v"].defined()){ // if fiven a ray
        vec3 v = obj["v"].getVec3();
        GLTF::Pin& pin = model.pins[name] ;
        vec3 current = model.nodes[pin.bone].transform * ( model.nodes[pin.bone].bone_to_mesh * vec4(pin.local_point,1));
        // Pull toward the closest point on the mouse ray
        target = p + v * (glm::dot(current-p, v) / glm::dot(v,v)) ;
    }else{
        target = p ; // no ray, target is point given
    }

    //printf("Pulling %s to (%f, %f, %f).\n", name.c_str(), target.x, target.y, target.z);
    model.setPinTarget(name, target);

    //model.applyPins();

    return emptyReturn();
}

byte* applyPins(byte* ptr){
    meshes[MAIN_MODEL].applyPins();
    return emptyReturn();
}

byte* deletePin(byte* ptr) {
    auto obj = Variant::deserializeObject(ptr);
    string name = obj["name"].getString();
    GLTF& model = meshes[MAIN_MODEL];
    model.deletePin(name);
    //printf("Pin '%s' deleted.\n" , name.c_str());
    return emptyReturn();
}


byte* createRotationPin(byte* ptr) {
    auto obj = Variant::deserializeObject(ptr);
    vec3 p = obj["p"].getVec3() ;
    string name = obj["name"].getString();
    GLTF& model = meshes[MAIN_MODEL];
    model.applyTransforms(); // Get current animated coordinates on CPU
    int vertex_index = model.getClosestVertex(p);
    vec3 global = p ;

    if(vertex_index < 0){
        model.computeNodeMatrices();
        return emptyReturn();
    }
    GLTF::Vertex& vert = model.vertices[vertex_index];
    glm::ivec4 joints = vert.joints;
    float max_w = -1.0f ;
    int bone = -1;
    for(int k=0;k<4;k++){
        float w = vert.weights[k] ;
        if(w > max_w){
            max_w = w ;
            bone = vert.joints[k] ;
        }
    }

    glm::quat initial= model.createRotationPin(name, bone, 1.0f);

    map<string, Variant> ret_map ;
    ret_map["initial"].makeFillableFloatArray(4);
    float* vm = ret_map["initial"].getFloatArray();
    vm[0] = initial.w;
    vm[1] = initial.x;
    vm[2] = initial.y;
    vm[3] = initial.z;

    mat4 m = glm::mat4_cast(initial);// TODO check tranpose
    ret_map["matrix"].makeFillableFloatArray(16);
    vm = ret_map["matrix"].getFloatArray();
    for(int k=0;k<16;k++){
        vm[k] = *(((float*)&m)+k) ;
    }

    return pack(ret_map);

}

byte* setRotationPinTarget(byte* ptr) {
    auto obj = Variant::deserializeObject(ptr);
    vec3 p = obj["p"].getVec3() ;
    string name = obj["name"].getString();
    glm::mat4 m ;
    float* vm = obj["target"].getFloatArray();
    for(int k=0;k<16;k++){
        *(((float*)&m)+k) = vm[k] ;
    }
    glm::quat target = glm::quat_cast(m) ; // TODO check tranpose
    GLTF& model = meshes[MAIN_MODEL];
    model.setRotationPinTarget(name, target);
    return emptyReturn();
}

byte* getNodeTransform(byte* ptr) {
    auto obj = Variant::deserializeObject(ptr);
    string name = obj["name"].getString();
    glm::mat4 m = meshes[MAIN_MODEL].getNodeTransform(name);
    map<string, Variant> ret_map ;
    ret_map["transform"].makeFillableFloatArray(16);
    float* vm = ret_map["transform"].getFloatArray();
    for(int k=0;k<16;k++){
        vm[k] = *(((float*)&m)+k) ;
    }
    return pack(ret_map);
}

byte* runTimelineUnitTests(byte* ptr) {
    UnitTests::runAll();
    return emptyReturn();
}

byte* initialize2DBallTimeline(byte* ptr){
    printf("initializing timeline!\n");
    initializeBallTimeline();
    
    auto obj = Variant::deserializeObject(ptr);
    int width = obj["width"].getInt();
    int height = obj["height"].getInt();
    int amount = obj["amount"].getInt();
    float min_radius = obj["min_radius"].getNumberAsFloat();
    float max_radius = obj["max_radius"].getNumberAsFloat();
    float max_speed = obj["max_speed"].getNumberAsFloat();
    initialize2DBallTimeline(width, height, amount, min_radius, max_radius, max_speed) ;
    
    return emptyReturn();
}

byte* runTimeline(byte* ptr){
    timeline->auto_clear_history =true;
    auto obj = Variant::deserializeObject(ptr);
    if(obj.find("time") == obj.end()){
        timeline->run();
    }else{
        float time = obj["time"].getNumberAsFloat();
        timeline->run(time);
    }
    //timeline->clearHistoryBefore(timeline->current_time-history_kept);
    return emptyReturn();
}

byte* getTimelineCircles(byte* ptr){
    vector<int> ob = timeline->updateObservables();

    map<string, Variant> ret_map ;
    int* circles = (int*)malloc(ob.size()*4*4);
    //vector<Variant> output ;
    for(int k=0;k<ob.size();k++){
        weak_ptr<TObject> ow = timeline->getLastObserved(ob[k]) ;
        if(auto o = ow.lock()){
            circles[4*k] = (int)o->position.x;
            circles[4*k+1] = (int)o->position.y;
            circles[4*k+2] = (int)o->radius;
            circles[4*k+3] = ob[k];
        //output.push_back(Variant(timeline->getLastObserved(ob[k])->serialize()));
        }
    }
    //ret_map["observables"] = Variant(output);
    ret_map["observables"] = Variant(circles, ob.size()*4);
    free(circles);
    return pack(ret_map);
}

byte* getInitialTimelineRequest(byte* ptr){
    
    initializeBallTimeline();
    map<string, Variant> sync_data ;
    sync_data["descriptor"] = timeline->getDescriptor(0.0, false);
    //printf("initial packet:\n");
    //Variant(sync_data).printFormatted();
    return pack(sync_data);
}

byte* synchronizeTimeline(byte* ptr){
    map<string, Variant> sync_data = Variant::deserializeObject(ptr);
    map<string, Variant> out_packet = timeline->synchronize(sync_data, false) ;
    return pack(out_packet);
}

byte* randomizeBallVelocity(byte* ptr){
    auto obj = Variant::deserializeObject(ptr);
    int id = obj["id"].getInt();
    float max_speed = obj["max_speed"].getNumberAsFloat();
    float angle = randomFloat()*6.29;
    float speed = randomFloat()*max_speed ;
    vec3 velocity(sin(angle)*speed, cos(angle)*speed,0);
    timeline->addEvent(std::make_unique<ChangeBallVelocity>(id, velocity), timeline->current_time+0.02) ;
    return emptyReturn();
}

}// end extern C