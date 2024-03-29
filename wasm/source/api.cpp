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
#include <math.h>
#include "glm/vec3.hpp"
#include "TableInterface.h"
#include "UnitTests.h"
#include "Timeline.h"
#include "TObject.h"
#include "TEvent.h"
#include "CreateObject.h"
#include "BouncingBall.h"
#include "MoveBouncingBall.h"
#include "ChangeBallVelocity.h"
#include "BallWall.h"
#include "MeshInstance.h"
#include "SetMeshInstance.h"
#include "MeshLibrary.h"
#include "ConvexShape.h"
#include "ConvexSolid.h"
#include "MoveSimpleSolid.h"
#include "SetConvexSolid.h"
#include "BSPNode.h"
#include "Subcurve.h"


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
//map<string,GLTF> meshes;
MeshLibrary meshes(2000) ;

string my_avatar = "default_avatar" ;

byte* packet_ptr ; // location ofr data passed as function parameters and returns
int last_pack_size = 0 ;

int selected_animation = -1;
std::chrono::high_resolution_clock::time_point animation_start_time;

unique_ptr<Timeline> timeline ;

float action_delay = 0.05;

std::unordered_map<int, TObject*> descriptor_cache ;


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
    timeline->observable_interpolation = true;
    MoveBouncingBall::friction = 2 ;
    return timeline.get() ;
}

void addBall(int width, int height, float min_radius, float max_radius, float max_speed){
    float radius = min_radius + randomFloat()*(max_radius-min_radius);
        vec3 position(radius *2 + randomFloat()*(width-radius*4), radius * 2 + randomFloat()*(height-radius*4), 0);
        float angle = randomFloat()*6.29;
        float speed = randomFloat()*max_speed ;

        vec3 velocity(sin(angle)*speed, cos(angle)*speed,0);
        
        std::unique_ptr<BouncingBall> o = std::make_unique<BouncingBall>(position, velocity, radius) ;
        std::unique_ptr<MoveBouncingBall> o_move = std::make_unique<MoveBouncingBall>(1.0/30.0) ;
        float timeoffset = randomFloat() ;
        timeline->createObject(std::move(o), std::move(o_move) , timeline->current_time + timeoffset);

}

Timeline* initialize2DBallTimeline(int width, int height, int amount, float min_radius, float max_radius, float max_speed){
    printf("initializing timeline!\n");
    initializeBallTimeline();
    //printf("amount: %d\n", amount);

    for(int k=0;k<amount;k++){
        addBall(width, height, min_radius, max_radius, max_speed);
        
    }

    std::unique_ptr<BallWall> center_wall = std::make_unique<BallWall>(vec3(width/2-100,height/2-50,-1), vec3(width/2+100,height/2+50,1)) ;
    timeline->createObject(std::move(center_wall), std::unique_ptr<TEvent>(nullptr) , 0.1 * randomFloat());

    std::unique_ptr<BallWall> outer_wall = std::make_unique<BallWall>(vec3(10,10,-1), vec3(width-10,height-10, 1)) ;
    timeline->createObject(std::move(outer_wall), std::unique_ptr<TEvent>(nullptr) , 0.1 * randomFloat());

    timeline->run(1.0);
    return timeline.get();
}


Timeline* initializeChatTimeline(){
    timeline = make_unique<Timeline>(&MeshInstance::createEvent, &MeshInstance::createObject);
    timeline->auto_clear_history=true;
    timeline->observable_interpolation = true;
    return timeline.get() ;
}

Variant getVariantMatrix(glm::quat q){
    mat4 m = glm::mat4_cast(q);
    Variant matrix ;
    matrix.makeFillableFloatArray(16);
    float* vm = matrix.getFloatArray();
    for(int k=0;k<16;k++){
        vm[k] = *(((float*)&m)+k) ;
    }
    return matrix ;
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
    selected_animation = -1; // stop animating while editing the model
    string select = my_avatar;
    std::shared_ptr<GLTF> model = std::make_shared<GLTF>() ;
    meshes.meshes.insert(select, model); // TODO add function for this so meshes.meshes can be private

    auto start_time = now();
    auto obj = Variant::deserializeObject(ptr);
    byte* byte_array = obj["data"].getByteArray();
    int num_bytes = obj["data"].getArrayLength();
    
    model->setModel(byte_array, num_bytes);

    float size = 0 ;
    vec3 center(0,0,0);
    for(int k=0;k<3;k++){
        center[k] = (model->max[k]+ model->min[k])*0.5f ;
        size = fmax(size , fabs(model->max[k]-center[k]));
        size = fmax(size , fabs(model->min[k]-center[k]));
        
    }

    model->transform = mat4(1);
    if(size < 0.5 || size > 3){
        model->transform  = glm::scale(mat4(1), {(1.0f/size),(1.0f/size),(1.0f/size)});
        model->transform  = glm::translate(model->transform, center*-1.0f);
    }
    
    model->computeNodeMatrices();
    model->applyTransforms();
    
    //printf("Zoom:%f\n", zoom);
    map<string, Variant> ret_map;
    ret_map["center"] = Variant(center);
    ret_map["size"] = Variant(size);

    int millis = millisBetween(start_time, now());
    printf("Total model load time: %d ms\n", millis);

    return pack(ret_map);
}

byte* requestModel(byte* ptr){
    auto obj = Variant::deserializeObject(ptr);
    string key = obj["key"].getString() ;
    meshes.requestRemoteMesh(key);

    /*
    string select = my_avatar;

    
    std::shared_ptr<GLTF> model = meshes[select];

    auto start_time = now();
    

    model->requestTableData(key);

    */

    return emptyReturn();
}



byte* getUpdatedBuffers(byte* ptr){

    map<string,Variant> buffers;
    vector<string> mesh_keys = meshes.getAllKeys();
    bool has_hand = false;
    auto st = now();
    int total_size = 0 ;
    for(auto &name : mesh_keys){
        std::shared_ptr<GLTF> mesh = meshes[name];
        
        if(mesh->position_changed || mesh->model_changed || mesh->bones_changed){
            bool completed_mesh = true ;
            for(auto const & [material_id, mat]: mesh->materials){
                if(material_id >= mesh->buffer_stopped_material_index){

                    std::stringstream ss;
                    ss << name << "-m=" << material_id;
                    string s_id = ss.str();
                    buffers[s_id] = mesh->getChangedBuffer(material_id) ;
                    int size = buffers[s_id].getSize() ;
                    total_size +=size ;
                    printf("got buffer: %s  size: %d\n", s_id.c_str(),size );
                    if(total_size > 50000000 && size != total_size){
                        buffers.erase(s_id) ;
                        mesh->buffer_stopped_material_index = material_id;
                        completed_mesh = false ;
                        break ; 
                    }
                }
            }
            if(completed_mesh){
                mesh->position_changed = false;
                mesh->model_changed = false;
                mesh->bones_changed = false;
                mesh->buffer_stopped_material_index = 0 ;
            }                
        }
        if(name == "HAND"){
            has_hand = true;
        }
    }

    if(!has_hand){
        shared_ptr<ConvexShape> shape = std::make_unique<ConvexShape>(ConvexShape::makeSphere(glm::vec3(0,0,0), 0.01, 0));
        meshes.addLocalShapeMesh("HAND", shape, vec3(0.85, 0.85, 1.0));
    }


    int ms = millisBetween(st, now());
    if(total_size > 0){
        printf("api get all buffers: %d ms total size: %d\n", ms, total_size);
    }
    return pack(buffers) ;
}

//expects an object with p and v, retruns a serialized single float for t
byte* rayTrace(byte* ptr){
    std::shared_ptr<GLTF> model = meshes[my_avatar];
    auto obj = Variant::deserializeObject(ptr);
    vec3 p = obj["p"].getVec3() ;
    vec3 v = obj["v"].getVec3();
    float t = model->rayTrace(p, v);
    map<string, Variant> ret_map;
    ret_map["t"] = Variant(t);
    ret_map["x"] = Variant(p+v*t);
    ret_map["index"] = Variant(model->last_traced_tri);
    return pack(ret_map);
}

byte* scan(byte* ptr){
    std::shared_ptr<GLTF> model = meshes[my_avatar];
    auto obj = Variant::deserializeObject(ptr);
    vec3 p = obj["p"].getVec3() ;
    vec3 v = obj["v"].getVec3();

    model->applyTransforms(); // Get current animated coordinates on CPU
    float t = model->rayTrace(p, v);
    map<string, Variant> ret_map;
    ret_map["t"] = Variant(t);
    ret_map["x"] = Variant(p+v*t);
    ret_map["index"] = Variant(model->last_traced_tri);

    if(model->last_traced_tri != -1){
        GLTF::Triangle tri = model->triangles[model->last_traced_tri];
        int material_id = tri.material;
        GLTF::Material mat = model->materials[tri.material];
        printf("triangle : %d, material: %d \n" ,model->last_traced_tri, material_id);
        if(model->json["materials"][material_id].defined()){
            model->json["materials"][material_id].printFormatted();
        }
        
        GLTF::Vertex& v = model->vertices[tri.A];
        glm::ivec4 joints = v.joints;
        for(int k=0;k<4;k++){
            int n = v.joints[k];
            string name = model->nodes[n].name;
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
    std::shared_ptr<GLTF> model = meshes[my_avatar];
    selected_animation = selected_animation+1;
    if(selected_animation >= model->animations.size()){
        selected_animation = -1;
        model->setBasePose();
        model->computeNodeMatrices();
    }
    animation_start_time = now();
    return emptyReturn();
}

// Returns the pending table requests that require a network request to fetch
// Items which are already in the cache are given to objects when this is called
//Note: this returns a pointer to VARIANT_ARRAY data but not the type, so it cannot be deserialized by default methods that assume an OBJECT type
byte* getTableNetworkRequest(byte* nothing) {
    TableInterface::serveCachedRequests();
    vector<string> requests = TableInterface::getRequestedKeys();
    vector<Variant> network_request;
    network_request.reserve(requests.size());
    for (const auto &key : requests) {
        // emplace back constructs Variants from strings
        network_request.emplace_back(key);
        //printf("Client requesting: %s\n", key.c_str());
    }

    for(auto& w : TableInterface::pending_writes){
        network_request.emplace_back(w); // TODO might need move semantics to avoid copy
    }
    TableInterface::pending_writes.clear();

    Variant request_packet = Variant(network_request) ;
    return pack(request_packet);
}

//Caches and Distributes data from a network table data response
byte* distributeTableNetworkData(byte* data_ptr) {
    map<string, Variant> data = Variant::deserializeObject(data_ptr);
    TableInterface::receiveTableData(data);
    return emptyReturn();
}

byte* createPin(byte* ptr) {
    auto obj = Variant::deserializeObject(ptr);
    vec3 p = obj["p"].getVec3() ;
    string name = obj["name"].getString();
    std::shared_ptr<GLTF> model = meshes[my_avatar];
    model->applyTransforms(); // Get current animated coordinates on CPU
    int vertex_index = -1 ;
    vec3 global ;
    if(obj["v"].defined()){ // we're making the pin from a ray
        vec3 v = obj["v"].getVec3();
        float t = model->rayTrace(p, v);
        if(model->last_traced_tri != -1){
            GLTF::Triangle tri = model->triangles[model->last_traced_tri]; 
            vertex_index = tri.A;
        }
        global = p + v * t;
    }else{
        vertex_index = model->getClosestVertex(p);
        global = p ;
    }
    if(vertex_index < 0){
        model->computeNodeMatrices();
        return emptyReturn();
    }
    GLTF::Vertex& vert = model->vertices[vertex_index];
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

    vec3 mesh_space =   glm::inverse(model->nodes[bone].transform) * vec4(global,1) ;
    vec3 local = model->nodes[bone].mesh_to_bone * vec4(mesh_space,1) ;

    glm::quat initial = model->createPin(name, bone, local, 1.0f, 1.0f);
    model->applyPins();
    
    map<string, Variant> ret_map ;
    ret_map["initial"].makeFillableFloatArray(4);
    float* vm = ret_map["initial"].getFloatArray();
    vm[0] = initial.w;
    vm[1] = initial.x;
    vm[2] = initial.y;
    vm[3] = initial.z;

    mat4 m = glm::mat4_cast(initial);
    ret_map["matrix"].makeFillableFloatArray(16);
    vm = ret_map["matrix"].getFloatArray();
    for(int k=0;k<16;k++){
        vm[k] = *(((float*)&m)+k) ;
    }
    //printf("Pin '%s' created for %s.\n" , name.c_str(), model->nodes[bone].name.c_str());
    return pack(ret_map);

}

byte* setPinTarget(byte* ptr) {
    auto obj = Variant::deserializeObject(ptr);
    string name = obj["name"].getString();
    std::shared_ptr<GLTF> model = meshes[my_avatar];
    if(obj["p"].defined()){
        vec3 p = obj["p"].getVec3() ;
        vec3 target ;
        if(obj["v"].defined()){ // if fiven a ray
            vec3 v = obj["v"].getVec3();
            GLTF::Pin& pin = model->pins[name] ;
            vec3 current = model->nodes[pin.bone].transform * ( model->nodes[pin.bone].bone_to_mesh * vec4(pin.local_point,1));
            // Pull toward the closest point on the mouse ray
            target = p + v * (glm::dot(current-p, v) / glm::dot(v,v)) ;
        }else{
            target = p ; // no ray, target is point given
        }
        if(!isnan(target.x) && !isnan(target.y) && !isnan(target.z)){
            model->setPinTarget(name, target);
        }
    }
   
    if(obj["o"].defined()){
        float* vm = obj["o"].getFloatArray();
        glm::mat4 m ;
        bool has_nan = false;
        for(int k=0;k<16;k++){
            *(((float*)&m)+k) = vm[k] ;
            has_nan = has_nan || isnan(vm[k]);
        }
        if(!has_nan){
            glm::quat rot_target = glm::quat_cast(m) ;
            rot_target = glm::normalize(rot_target);
            model->setPinTarget(name, rot_target);
        }
    }

    //model->applyPins();

    return emptyReturn();
}

byte* applyPins(byte* ptr){
    meshes[my_avatar]->applyPins();
    return emptyReturn();
}

byte* deletePin(byte* ptr) {
    auto obj = Variant::deserializeObject(ptr);
    string name = obj["name"].getString();
    std::shared_ptr<GLTF> model = meshes[my_avatar];
    model->deletePin(name);
    //printf("Pin '%s' deleted.\n" , name.c_str());
    return emptyReturn();
}

byte* getNodeTransform(byte* ptr) {
    auto obj = Variant::deserializeObject(ptr);
    string name = obj["name"].getString();
    glm::mat4 m = meshes[my_avatar]->getNodeTransform(name);
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

byte* runTimeline(byte* ptr){
    timeline->auto_clear_history=true;
    MoveBouncingBall::friction = 2 ;
    auto obj = Variant::deserializeObject(ptr);
    if(obj.find("time") == obj.end()){
        timeline->run();
    }else{
        float time = obj["time"].getNumberAsFloat();
        timeline->run(time);
    }
    return emptyReturn();
}

byte* getBallObjects(byte* ptr){
    vector<int> ob = timeline->updateObservables();

    map<string, Variant> ret_map ;
    int num_per_obj = 6 ;
    int* render_data = (int*)malloc(ob.size()*4*num_per_obj);
    //vector<Variant> output ;
    for(int k=0;k<ob.size();k++){
        weak_ptr<TObject> ow = timeline->getLastObserved(ob[k]) ;
        if(auto o = ow.lock()){
            render_data[num_per_obj*k] = ob[k];
            render_data[num_per_obj*k+1] = o->type;
            if(o->type == 1) { // ball
                render_data[num_per_obj*k+2] = (int)o->position.x;
                render_data[num_per_obj*k+3] = (int)o->position.y;
                render_data[num_per_obj*k+4] = (int)o->radius;

            }else if(o->type == 2){ // wall
                shared_ptr<BallWall> w = std::static_pointer_cast<BallWall>(o);
                render_data[num_per_obj*k+2] = (int)w->min.x;
                render_data[num_per_obj*k+3] = (int)w->min.y;
                render_data[num_per_obj*k+4] = (int)w->max.x;
                render_data[num_per_obj*k+5] = (int)w->max.y;
            }
            
            
        //output.push_back(Variant(timeline->getLastObserved(ob[k])->serialize()));
        }
    }
    //ret_map["observables"] = Variant(output);
    ret_map["stride"] = Variant(num_per_obj) ;
    ret_map["observables"] = Variant(render_data, ob.size()*num_per_obj);
    free(render_data);
    return pack(ret_map);
}


byte* getMeshInstances(byte* ptr){
    vector<int> ob = timeline->updateObservables();

    map<string, Variant> ret_map ;
    //vector<Variant> output ;
    for(int k=0;k<ob.size();k++){
        weak_ptr<TObject> ow = timeline->getLastObserved(ob[k]) ;
        if(auto o = ow.lock()){
            if(o->type == 1){
                shared_ptr<MeshInstance> instance = std::static_pointer_cast<MeshInstance>(o);
                std::shared_ptr<GLTF> mesh_asset = meshes[instance->mesh_name];
                if(mesh_asset != nullptr){
                    map<string, Variant> inst_map ;
                    inst_map["mesh"] = Variant(instance->mesh_name) ;
                    inst_map["owner"] = Variant(instance->owner) ;
                    inst_map["pose"] = Variant(instance->pose);
                    if(instance->bone_data.defined()){
                        if(instance->bones_compressed){
                            inst_map["bones"] = mesh_asset->getBoneData(instance->bone_data) ;
                        }else{
                            inst_map["bones"] = instance->bone_data.clone() ;
                        }
                    }

                    if(instance->write_time > timeline->current_time - 5 || instance->owner == "server"){ // TODO less hardcoded timeouts
                        ret_map[std::to_string(ob[k])] = Variant(inst_map);
                        //printf("Returned instance:\n");
                        //ret_map[std::to_string(ob[k])].printFormatted();
                    }
                }
            }else if(o->type == 2){ // convex solid
                shared_ptr<ConvexSolid> solid= std::static_pointer_cast<ConvexSolid>(o);
                string mesh_name = "s-" + std::to_string(solid->shape_id);
                std::shared_ptr<GLTF> mesh_asset = meshes[mesh_name];
                if(mesh_asset != nullptr){
                    map<string, Variant> inst_map ;
                    inst_map["mesh"] = Variant(mesh_name) ;
                    //inst_map["owner"] = Variant("physics") ;
                    inst_map["pose"] = Variant(solid->getTransform());
                    ret_map[std::to_string(ob[k])] = Variant(inst_map);
                }
            
            }else if(o->type == 3){ // convex shape
                shared_ptr<ConvexShape> shape = std::static_pointer_cast<ConvexShape>(o);
                    string mesh_name = "s-" + std::to_string(ob[k]) ;
                    std::shared_ptr<GLTF> mesh_asset = meshes[mesh_name] ;
                    if(mesh_asset == nullptr){
                        
                        meshes.addLocalShapeMesh(mesh_name, shape, vec3(0.85, 0.85, 1.0)); // blue tint for contrast with grey background
                        //meshes.addLocalShapeMesh(mesh_name + "1", shape, vec3(1.0, 1.0, 0.65)); // yellow for sphere collision
                        //meshes.addLocalShapeMesh(mesh_name + "2", shape, vec3(1.0, 0.8, 0.8)); // red for true collision
                    }
            }else{
                printf("Got an unrecognized object in the timeline:\n");
                Variant(o->serialize()).printFormatted();
            }
        }
    }
    return pack(ret_map);
}

byte* createMeshInstance(byte* ptr){
    //printf("Creating mesh instance!");
    auto obj = Variant::deserializeObject(ptr);
    string owner = obj["owner"].getString();
    glm::mat4 tm(1);
    Variant bones = meshes[my_avatar]->getCompressedBoneData();
    std::unique_ptr<MeshInstance> o = std::make_unique<MeshInstance>(glm::vec3(0,0,0), 2, owner, my_avatar, tm, bones, true) ;
    timeline->createObject(std::move(o), std::unique_ptr<TEvent>(nullptr) , timeline->current_time + action_delay);
    return emptyReturn();
}

byte* setMeshInstance(byte* ptr){
    //printf("Updating mesh instance!");
    auto obj = Variant::deserializeObject(ptr);
    //Variant(obj).printFormatted();
    int id = obj["id"].getInt();
    glm::mat4 pose = obj["pose"].getMat4();
    Variant bones = meshes[my_avatar]->getCompressedBoneData();


    timeline->addEvent(std::make_unique<SetMeshInstance>(id, glm::vec3(0,0,0), 2, my_avatar, pose, bones),  timeline->current_time+action_delay) ;
    return emptyReturn();
}

byte* setAvatar(byte* ptr){
    auto obj = Variant::deserializeObject(ptr);
    my_avatar = obj["avatar"].getString();
    printf("Set avatar (API): %s\n", my_avatar.c_str());
    return emptyReturn();
}

byte* getInitialTimelineRequest(byte* ptr){
    
    //initializeBallTimeline();
    initializeChatTimeline();
    map<string, Variant> sync_data ;
    sync_data["descriptor"] = timeline->getDescriptor(0.0, false);
    //printf("initial packet:\n");
    //Variant(sync_data).printFormatted();
    return pack(sync_data);
}

byte* synchronizeTimeline(byte* ptr){
    map<string, Variant> sync_data = Variant::deserializeObject(ptr);
    map<string, Variant> out_packet = timeline->synchronize(sync_data, false, descriptor_cache) ;
    return pack(out_packet);
}

byte* setBallVelocity(byte* ptr){
    auto obj = Variant::deserializeObject(ptr);
    int id = obj["id"].getInt();
    vec3 velocity = obj["v"].getVec3();
    timeline->addEvent(std::make_unique<ChangeBallVelocity>(id, velocity), timeline->current_time+0.021245682) ;
    return emptyReturn();
}

byte* getTimelineRunStats(byte* ptr){
    map<string, Variant> ret_map ;
    ret_map["runs"] = Variant(timeline->total_runs) ;
    ret_map["unruns"] = Variant(timeline->total_unruns) ;
    timeline->total_runs = 0;
    timeline->total_unruns = 0 ;
    return pack(ret_map);
}

byte* popPendingQuickSends(byte* ptr){
    Variant qs = timeline->popQuickSends() ;
    if(qs.defined()){
        return pack(qs);
    }else{
        return emptyReturn();
    }
}

// Returns the position of the viewpoint of a VRM model in model space
byte* getFirstPersonPosition(byte* ptr){
    map<string, Variant> ret_map ;
    ret_map["position"] = Variant (meshes[my_avatar]->getFirstPersonPosition());
    return pack(ret_map);
}

byte* createVRMPins(byte* ptr){
    std::shared_ptr<GLTF> avatar = meshes[my_avatar] ;
    meshes[my_avatar]->setBasePose();
    map<string, Variant> ret_map ;
    if(avatar == nullptr){
        ret_map["error"] = Variant("avatar not found on VRM IK bind!");
        return pack(ret_map);
    }

    ret_map["head"] = getVariantMatrix(avatar->createPin("head", avatar->first_person_bone, avatar->first_person_offset, 1.0f, 0.1f));

    ret_map["left_hand"] = getVariantMatrix(avatar->createPin("left_hand", avatar->human_bone["leftHand"], vec3(0,0,0), 1.0f, 0.1f));

    ret_map["right_hand"] = getVariantMatrix(avatar->createPin("right_hand", avatar->human_bone["rightHand"], vec3(0,0,0), 1.0f, 0.1f));
    
    //(Variant( ret_map)).printFormatted();
    return pack(ret_map);
}

// returns the current bone data for the given mesh
byte* getBones(byte* ptr){

    auto obj = Variant::deserializeObject(ptr);
    string mesh_name = obj["mesh"].getString();

    //if(obj["animation"].defined){
        //int selected_animation = obj["animation"].getInt();

        if(selected_animation >= 0){
            auto& animation = meshes[mesh_name]->animations[selected_animation];
            float time = millisBetween(animation_start_time, now()) / 1000.0f; // TODO accept time as parameter
                if(time > animation.duration){
                    time = 0 ;
                    animation_start_time = now();
                }
            meshes[mesh_name]->animate(animation,time);
        }

    //}

    map<string, Variant> ret_map ;
    ret_map["bones"] = meshes[mesh_name]->getBoneData() ;
    return pack(ret_map);
}

// Returns the timeline "id" and "distance" of the nearest observable solid to the given point p
byte* getNearestSolid(byte* ptr){
    auto obj = Variant::deserializeObject(ptr);
    vec3 grab_position = obj["p"].getVec3();
    int closest = -1;
    float best_score = 1E30;
    mat4 initial_pose;

    vector<int> ob = timeline->updateObservables();
    for(int k=0;k<ob.size();k++){
        weak_ptr<TObject> ow = timeline->getLastObserved(ob[k]) ;
        if(auto o = ow.lock()){
            if(o->type == 2){
                shared_ptr<ConvexSolid> solid = std::static_pointer_cast<ConvexSolid>(o);
                float score = glm::length(solid->position-grab_position);
                if(score < best_score && solid->moveable){
                    best_score = score;
                    closest = ob[k];
                    initial_pose = solid->getTransform();
                }
            }
        }
    }

    map<string, Variant> ret_map ;
    ret_map["id"] = Variant(closest);
    ret_map["distance"] = Variant(best_score);
    ret_map["initial_pose"] = Variant(initial_pose);
    return pack(ret_map);
}

// sets the solid given by "id" to the given mat4 "pose" 
byte* setSolidPose(byte* ptr){
    auto obj = Variant::deserializeObject(ptr);
    int id = obj["id"].getInt();
    mat4 pose = obj["pose"].getMat4();
    vec3 p = vec3(pose[3]);
    vec3 v = vec3(0,0,0);
    glm::quat o = glm::quat_cast(pose);
    vec3 av = vec3(0,0,0);

    //Variant(obj).printFormatted();
    if(obj["last_pose"].defined()){
        mat4 last_pose = obj["last_pose"].getMat4();
        float dt = obj["dt"].getNumberAsFloat();
        vec3 lp = vec3(last_pose[3]);
        v = (p-lp)/dt ;
        glm::quat lo = glm::quat_cast(last_pose);

        glm::quat dq = o * glm::inverse(lo); 
        float angle = glm::angle(dq);
        vec3 axis = glm::axis(dq);
        av = axis*angle/dt;
    }
    bool mv = !(obj["freeze"].defined());
    timeline->addEvent(std::make_unique<SetConvexSolid>(id, p, v, o, av , mv),  timeline->current_time+action_delay) ;
    return emptyReturn();
}

byte* setIKParams(byte* ptr){
    auto obj = Variant::deserializeObject(ptr);
    std::shared_ptr<GLTF> model = meshes[my_avatar];

    if(obj["barrier_strength"].defined()){
        model->barrier_strength = obj["barrier_strength"].getNumberAsFloat();
    }
    if(obj["stiffness_strength"].defined()){
        model->stiffness_strength = obj["stiffness_strength"].getNumberAsFloat();
    }
    if(obj["tolerance"].defined()){
        model->tolerance = obj["tolerance"].getNumberAsFloat();
    }
    if(obj["stiffness_decay"].defined()){
        model->stiffness_decay = obj["stiffness_decay"].getNumberAsFloat();
    }
    if(obj["lbfgs_m"].defined()){
        model->lbfgs_m = obj["lbfgs_m"].getInt();
    }
    if(obj["iter"].defined()){
        model->iter = obj["iter"].getInt();
    }
    if(obj["step_iter"].defined()){
        model->step_iter = obj["step_iter"].getInt();
    }

    return emptyReturn();
}

byte* smoothLine(byte* ptr){
    auto obj = Variant::deserializeObject(ptr);
    Variant in_array = obj["input"];
    float window = obj["window"].getNumberAsFloat();
    int degree = (int)obj["degree"].getNumberAsFloat();
    vector<vec3> input ;
    for(int k=0;k<in_array.getArrayLength();k++){
        input.push_back(in_array[k].getVec3());
    }
    vector<vec3> output ;
    Subcurve curve(degree) ;
    for(int k=0;k<input.size();k++){
        curve.addTarget(input[k][1],input[k][2],input[k][0]); 
  
    }
    curve.solve();

    for(float t = 0; t <=1;t+=0.01){
        glm::vec2 p = curve.valueNormalized(t) ;
        output.push_back(vec3(t,p.x,p.y));
    }

    
    vector<Variant> out_array ;
    for(int k=0;k<output.size();k++){
        out_array.push_back(Variant(output[k]));
    }
    map<string, Variant> ret_map ;
    ret_map["result"] = Variant(out_array);
    return pack(ret_map);
}

}// end extern C