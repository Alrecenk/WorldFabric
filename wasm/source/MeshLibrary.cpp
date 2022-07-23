
#include "MeshLibrary.h"

using std::string ;

// Gets a mesh from the cache
// If the mesh is not present, requests it from the remote server and returns nullptr
std::shared_ptr<GLTF> MeshLibrary::get(std::string key){
    if(meshes.contains(key)){
        return meshes.get(key);
    }else{
        requestRemoteMesh(key);
        return std::shared_ptr<GLTF>(nullptr);
    }
}

std::shared_ptr<GLTF>  MeshLibrary::operator [](std::string key){
    return get(key) ;
}

// Request a fetch of a pending item if one is not already queued
void MeshLibrary::tryRequestNext(){
    if(!request_is_pending && !missing_keys.empty()){
        request_is_pending = true ;
        string r = *(missing_keys.begin());
        //printf("fetching remote mesh: %s\n", r.c_str());
        requestTableData(r);
    }
}

// Queues the download of a mesh in advance
void MeshLibrary::requestRemoteMesh(std::string key){
    if(!meshes.contains(key)){ // this is public so don't allow excessive calls to make network calls
        //printf("Requesting remote mesh: %s\n", key.c_str());
        missing_keys.insert(key);
        tryRequestNext();
    }
}

// TableIntrface method to catch downloaded mesh data
void MeshLibrary::receiveTableData(std::string key, const Variant& data){
    //printf("Got remote mesh: %s\n", key.c_str());
    std::shared_ptr<GLTF> new_mesh = std::make_shared<GLTF>() ;
    new_mesh->receiveTableData(key, data);
    meshes.insert(key, new_mesh);
    missing_keys.erase(key);
    request_is_pending = false;
    tryRequestNext();
}

// Uploads a GLB file to the server, making its mesh data available to all connected clients
void MeshLibrary::uploadMesh(std::string key, const Variant& glb_file_data){
    if(!meshes.contains(key)){// this is public so don't allow excessive calls to make network calls
        std::shared_ptr<GLTF> new_mesh = std::make_shared<GLTF>() ;
        new_mesh->receiveTableData(key, glb_file_data);
        meshes.insert(key, new_mesh);
        writeTableData(key, glb_file_data);
    }
}

// Returns and deletes the set of keys of meshes that need to be removed from VRAm
std::set<std::string> MeshLibrary::popDeletedKeys(){
    return meshes.popDeletedKeys();
}

std::vector<std::string> MeshLibrary::getAllKeys(){
    return meshes.getAllKeys();
}
