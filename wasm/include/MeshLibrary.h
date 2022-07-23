#ifndef _MESH_LIBRARY_H_
#define _MESH_LIBRARY_H_ 1

#include "GLTF.h"
#include "TableInterface.h"
#include "LRUCache.h"

#include <vector>
#include <memory>
#include <set>

class MeshLibrary : public TableInterface{


    private:
        LRUCache<std::string,GLTF> meshes ; // meshes currently in memory
        std::set<std::string> deleted_keys; // keys deleted from the cache to to overflow (need to be cleared from GPU memory in Javascript)
        std::set<std::string> missing_keys; // keys requested that were not available, queue for remote fetching
        bool request_is_pending = false ;

        // Request a fetch of a pending item if one is not already queued
        void tryRequestNext();
    
    public:

        MeshLibrary(int max_meshes):meshes(max_meshes){}

        // Gets a mesh from the cache
        // If the mesh is not present, requests it from the remote server and returns nullptr this time
        std::shared_ptr<GLTF> get(std::string key);

        //same as get
        std::shared_ptr<GLTF> operator [](std::string key);

        // Queues the download of a mesh in advance
        void requestRemoteMesh(std::string key);

        // TableIntrface method to catch downloaded mesh data
        void receiveTableData(std::string key, const Variant& data) override;

        // Uploads a GLB file to the server, making its mesh data available to all connected clients
        void uploadMesh(std::string key, const Variant& glb_file_data);

        // Returns and deletes the set of keys of meshes that need to be removed from VRAM
        std::set<std::string> popDeletedKeys();

        std::vector<std::string> getAllKeys();
      
};
#endif // #ifndef _MESH_LIBRARY_H_