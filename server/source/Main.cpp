#include "SecureWebServer.h"
#include "TableServer.h"
#include "TimelineServer.h"
#include "api.cpp"

#include <signal.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <fstream>
#include <iterator>
#include <string>
#include <stack>

using glm::vec3;
using glm::dvec3;
using glm::mat4;
using std::vector;
using std::cout;
using std::endl;
using std::unordered_map ;
using std::map ;
using std::stack ;
using std::string ;

void runUnitTests(){
    bool g = UnitTests::runAll();
}

void loadModels(unordered_map<string, Variant>& table){
    map<string,string> models ;
    models["default_avatar"] = "./models/default_avatar.vrm";
    models["alternate_avatar"] = "./models/alternate_avatar.vrm";
    models["avatar_3"] = "./models/avatar_3.vrm";
    models["avatar_4"] = "./models/avatar_4.vrm";
    models["eroom"] = "./models/room.glb";
    models["dragon"] = "./models/dragon.glb";
    models["bunny"] = "./models/bunny350.glb";
    models["default_world"] = "./models/default_world.glb";
    models["default_skybox"] = "./models/default_skybox.glb";
    for(auto& [key, path] : models){
        std::ifstream input( path.c_str(), std::ios::binary );
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});
        table[key] = Variant(&(buffer[0]), buffer.size());
        printf("Loaded model %s (%d bytes = %d)\n",key.c_str(), (int) buffer.size(), table[key].getArrayLength());
    }
}



void addScenery(Timeline* timeline, unordered_map<string, Variant>& table, string mesh_name, mat4 transform, int convex_cutoff, int convex_detail, float max_extent, bool debug_mode){
    double st = timeline->current_time ;
    std::shared_ptr<GLTF> mesh = meshes[mesh_name];
    if(mesh == nullptr){
         meshes.receiveTableData(mesh_name,table[mesh_name]);
         mesh = meshes[mesh_name];
    }


    mesh->transform = transform ;
    mesh->computeNodeMatrices();
    mesh->applyTransforms();

    Variant bones;
    std::unique_ptr<MeshInstance> o = std::make_unique<MeshInstance>(glm::vec3(0,0,0), 2, "server", mesh_name, transform , bones, false) ;
    if(!debug_mode){
        timeline->createObject(std::move(o), std::unique_ptr<TEvent>(nullptr) , st + 0.01234);
    }
    
    vector<vector<Polygon>> surfaces = Polygon::collectClosedSurfaces(mesh);
    map<string,vec3> part_position ;
    map<string,vec3>* part_ptr = &part_position ;
    map<string,float> part_radius ;
    map<string,float>* radius_ptr = &part_radius ;
    int total_solids = 0 ;
    int sn = 0 ;
    for(auto& surface : surfaces){
        
        std::unique_ptr<BSPNode> root ;
        if(surface.size() > convex_cutoff){
            std::vector<dvec3> points ;
            for(auto& poly : surface){
                for(auto& x : poly.p){
                    points.push_back(x);
                }
            }
            printf("Building approximate hull from %d points...\n", (int)points.size());
            std::vector<Polygon> shape = Polygon::buildApproximateHull(points, convex_detail, 3);
            printf("Result hull has %d faces...\n", (int)shape.size());

            root = make_unique<BSPNode>(vector<Polygon>(), shape);
            root->leaf_inside = true;
        }else{
            printf("Building full geometry for %d triangle surface...\n", (int)surface.size());
            root = make_unique<BSPNode>(surface);
        }
            
        sn++;
        
        root->computeVolumeInside();

        std::stack<BSPNode*> nodes ;
        nodes.push(root.get());
        int n = 0 ;
        while(!nodes.empty()){
            BSPNode* node = nodes.top();
            nodes.pop();
            bool generate = false;
            if(node == nullptr){
                generate = false;
            }else if(!node->leaf){
                
                if(node->volume_outside < 0.000001){
                    generate = true ;
                }else if(node->volume_inside < 0.000001){
                    generate = false;
                }else{
                    nodes.push(node->inner.get());
                    nodes.push(node->outer.get());
                }
            }else if(node->leaf_inside && node->volume_inside > 0.00001){
                generate = true;
            }
            if(generate){

                std::vector<std::vector<Polygon>> shapes = Polygon::splitToMaximumExtent(node->shape, max_extent) ;

                for(auto&surface: shapes){
                    std::unique_ptr<ConvexShape> shape = std::make_unique<ConvexShape>(surface);
                    if(shape->vertex.size() > 3 && shape->getVolume() > 0.0001){
                        total_solids++;
                        string trigger = mesh_name + " " + std::to_string(n) +"-" + std::to_string(sn) ;
                        part_position[trigger]= -(shape->centerOnCentroid());
                        part_radius[trigger] = shape->radius ;
                        n++;
                        shape->is_observable = debug_mode;
                        //printf("position: %f, %f, %f \n", part_position[trigger][0], part_position[trigger][1], part_position[trigger][2]);
                        float mass = shape->getVolume();
                        
                        //Variant(shape->serialize()).printFormatted();
                        timeline->createObject(std::move(shape), std::unique_ptr<TEvent>(nullptr), trigger,st+ 0.01 + 0.01*randomFloat());
                        timeline->subscribe(trigger, trigger,
                        [timeline,st,mass, part_ptr, radius_ptr](const string& subscriber, const string& trigger, const Variant& data){
                            int shape_id = data.getInt();
                            //printf("Trigger : %s \n", trigger.c_str());                   
                            
                            std::unique_ptr<ConvexSolid> solid = std::make_unique<ConvexSolid>(
                                ConvexSolid(shape_id, mass, (*part_ptr)[trigger], glm::quat(1,0,0,0)));
                                solid->moveable = false;
                                solid->radius = (*radius_ptr)[trigger] ;
                                solid->is_observable = false;

                            //printf("id: %d  position : %f, %f, %f   radius: %f\n", shape_id, position[0], position[1], position[2], solid->radius);   
                            timeline->createObject(std::move(solid), std::unique_ptr<TEvent>(nullptr), st+ 1.1 + 0.01*randomFloat() );     
                        });
                    }
                }
            }
        }
    }
    timeline->run(st+ 1.0) ;// notifications go out after a call to run completes
    timeline->run(st+ 2.0) ; // run some more to get the solid made
    printf("Total solids added to scenery: %d\n", total_solids);

}

void addRoom(Timeline* timeline, unordered_map<string, Variant>& table){
    float size = 14.087057 ;
    vec3 center( -0.197625,10.558842,1.469670);
    glm::mat4 pose(0);
    pose[0][0] = 2/size ;
    pose[1][2] = -2/size  ;
    pose[2][1] = 2/size  ;
    
    pose[3][0] = 0 ;
    pose[3][1] = -1 ;
    pose[3][2] = -1.75;
    pose[3][3] = 1;

    addScenery(timeline, table, "eroom", pose, 256,30, 10000,false);
    
    // Room models as a 3 wall diorama so rotate it and to make a complete room
    pose = mat4(0);
    pose[0][0] = -2/size  ;
    pose[1][2] = 2/size  ;
    pose[2][1] = 2/size  ;

    pose[3][0] = 0.0001; // tiny x offset prevents z fighting on overlap
    pose[3][1] = -1.0001 ;
    pose[3][2] = 1.75;
    pose[3][3] = 1;

    addScenery(timeline, table, "eroom", pose, 256, 30, 10000, false);
}

void addTestShapes(Timeline* timeline){
    double st = timeline->current_time ;

    std::unique_ptr<ConvexShape> shape0 = std::make_unique<ConvexShape>(ConvexShape::makeTetra(
        vec3(-.1,.1,0),vec3(-.1,-.1,0),vec3(.1,0,-.1),vec3(.1,0,.1)));
    shape0->is_observable = true;
    float mass = 1;    
    timeline->createObject(std::move(shape0), std::unique_ptr<TEvent>(nullptr), "tetra_shape_created",st+ 0.01 + 0.01*randomFloat());
    timeline->subscribe("tetra_solid_maker", "tetra_shape_created",
    [timeline,st,mass](const string& subscriber, const string& trigger, const Variant& data){
        int shape_id = data.getInt();
        for(float x = -1.5; x < 1.5; x+=0.75f){
        for(float y = 0; y < 0.5; y+=0.75f){
            std::unique_ptr<ConvexSolid> solid = std::make_unique<ConvexSolid>(
                ConvexSolid(shape_id, mass, glm::vec3(x,y,-0.75f), glm::quat(0,0,0,1)));
            //solid->velocity = vec3(10.0f*(randomFloat()-0.5f),10.0f*(randomFloat()-0.5f),10.0f*(randomFloat()-0.5f));
            timeline->createObject(std::move(solid), std::make_unique<MoveSimpleSolid>(1.0/90), st+ 1.1 + 0.01*randomFloat() );
        }}
    });

    std::unique_ptr<ConvexShape> shape = std::make_unique<ConvexShape>(ConvexShape::makeAxisAlignedBox(vec3(0.25,0.25,0.25)));
    shape->is_observable = true;
    mass = 1;
    timeline->createObject(std::move(shape), std::unique_ptr<TEvent>(nullptr), "box_shape_created", st+ + 0.01*randomFloat());
    timeline->subscribe("box_solid_maker", "box_shape_created",
    [timeline,st,mass](const string& subscriber, const string& trigger, const Variant& data){
        int shape_id = data.getInt();
        for(float x = -1.5; x < 1.5; x+=0.75f){
        for(float y = 0; y < 0.5; y+=0.75f){
            std::unique_ptr<ConvexSolid> solid = std::make_unique<ConvexSolid>(
                ConvexSolid(shape_id, mass, glm::vec3(x,y,-0.25), glm::quat(0,0,0,1)));
            //solid->velocity = vec3(10.0f*(randomFloat()-0.5f),10.0f*(randomFloat()-0.5f),10.0f*(randomFloat()-0.5f));
            timeline->createObject(std::move(solid), std::make_unique<MoveSimpleSolid>(1.0/90), st+ 1.1 + 0.01*randomFloat() );
        }}
    });


    std::unique_ptr<ConvexShape> shape2 = std::make_unique<ConvexShape>(ConvexShape::makeCylinder(glm::vec3(0,0.2,0), glm::vec3(0,-0.2,0), 0.05, 16));
    shape2->is_observable = true;
    mass = 1 ;
    timeline->createObject(std::move(shape2), std::unique_ptr<TEvent>(nullptr), "cylinder_shape_created", st+ + 0.01*randomFloat() );
    timeline->subscribe("cylinder_solid_maker", "cylinder_shape_created",
    [timeline,st,mass](const string& subscriber, const string& trigger, const Variant& data){
        int shape_id = data.getInt();
        for(float x = -1.5; x < 1.5; x+=0.75f){
        for(float y = 0; y < 0.5; y+=0.75f){
            std::unique_ptr<ConvexSolid> solid = std::make_unique<ConvexSolid>(
                ConvexSolid(shape_id, mass, glm::vec3(x,y,0.25), glm::quat(0,0,0,1)));
            //solid->velocity = vec3(10.0f*(randomFloat()-0.5f),10.0f*(randomFloat()-0.5f),10.0f*(randomFloat()-0.5f));
            timeline->createObject(std::move(solid), std::make_unique<MoveSimpleSolid>(1.0/90), st+ 1.1 + 0.01*randomFloat() );
        }}
    });
    

    std::unique_ptr<ConvexShape> shape3 = std::make_unique<ConvexShape>(ConvexShape::makeSphere(glm::vec3(0,0,0), 0.15, 2));
    shape3->is_observable = true;
    mass = 1;
    timeline->createObject(std::move(shape3), std::unique_ptr<TEvent>(nullptr), "sphere_shape_created", st+ 0.01*randomFloat() );
    timeline->subscribe("sphere_solid_maker", "sphere_shape_created",
    [timeline,st, mass](const string& subscriber, const string& trigger, const Variant& data){
        int shape_id = data.getInt();
        for(float x = -1.5; x < 1.5; x+=0.75f){
        for(float y = 0; y < 0.5; y+=0.75f){
            std::unique_ptr<ConvexSolid> solid = std::make_unique<ConvexSolid>(
                ConvexSolid(shape_id, mass, glm::vec3(x,y,0.75), glm::quat(0,0,0,1)));
            //solid->velocity = vec3(10.0f*(randomFloat()-0.5f),10.0f*(randomFloat()-0.5f),10.0f*(randomFloat()-0.5f));
            timeline->createObject(std::move(solid), std::make_unique<MoveSimpleSolid>(1.0/90), st+ 1.1+ 0.01*randomFloat() );
        }}
    });

    timeline->run(st+ 1.0) ;// notifications go out after a call to run completes
    timeline->run(st+ 2.0) ; // run some more to get the solid made
}

int dragon_id = -1; // ID of server created dragon
void addDragon(Timeline* timeline, unordered_map<string, Variant>& table){
    glm::mat4 pose(1);
    pose[0][0] = 2 ;
    pose[1][1] = 2 ;
    pose[2][2] = 2 ;

    pose[3][1] = 3;

    Variant bones;
    std::unique_ptr<MeshInstance> o = std::make_unique<MeshInstance>(glm::vec3(0,3,0), 2, "server", "dragon", pose, bones, true) ;
    timeline->createObject(std::move(o), std::unique_ptr<TEvent>(nullptr) , "dragon_created", timeline->current_time + 0.0017);
    int* dptr = &dragon_id;
    timeline->subscribe("dragon_id_catcher", "dragon_created",
    [dptr](const string& subscriber, const string& trigger, const Variant& data){
        (*dptr) = data.getInt();
    });

    meshes.receiveTableData("dragon",table["dragon"]);
    animation_start_time = now();
}

void animateDragon(Timeline* timeline){
    double t= timeline->current_time;
    std::shared_ptr<GLTF> model = meshes["dragon"];
    int selected_animation = 2;
    if(model != nullptr && dragon_id != -1){
        auto& animation = model->animations[selected_animation];
        float time = millisBetween(animation_start_time, now()) / 1000.0f; // TODO accept time as parameter
            if(time > animation.duration){
                time = 0 ;
                animation_start_time = now();
            }
        model->animate(animation,time);

        Variant bones = model->getCompressedBoneData();
        glm::mat4 pose(1);
        float scale = 2.5 ;
        float angle = timeline->current_time ;
        float height = 4;
        float r = 3 ;
        float s = sin(angle);
        float c= cos(angle);
        pose[0] = vec4(s*scale, 0, c*scale, 0);
        pose[1] = vec4(0,scale,0, 0);
        pose[2] = vec4(c*scale, 0, -s*scale, 0);
        pose[3] = vec4(r*s, height, r*c+1.75, 1);

        timeline->addEvent(std::make_unique<SetMeshInstance>(dragon_id, glm::vec3(0,3,0), 2, "dragon", pose, bones),  timeline->current_time+action_delay) ;
    }    
}

bool running = true;

// Define the function to be called when ctrl-c (SIGINT) is sent to process
void quit(int signum) {
    printf("Shutting down...");
    running = false;
}

Timeline* generateBallTimeline(){
    int width = 600;
    int height = 600;
    float min_radius = 10;
    float max_radius = 15 ;
    float max_speed = 50 ;
    Timeline* timeline = initialize2DBallTimeline(width, height, 50, min_radius, max_radius, max_speed) ;
    return timeline ;
}

int main(int argc, const char** argv) {

    unsigned char* packet_ptr = (unsigned char*)malloc(50000000); // 50 megabytes
    setPacketPointer(packet_ptr);

    runUnitTests();
    
    // boot up a static webserver on a nonblocking thread to serve the frontend
    // command for generating key files in linux:
    // openssl req -nodes -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 365
    SecureWebServer web_server(8080, "./hosted/", "./cert/cert.pem","./cert/key.pem");
 
    unordered_map<string, Variant> table;
    table["test"] = Variant("cactus") ;
    loadModels(table);

    //Timeline* timeline = generateBallTimeline() ;
    Timeline* timeline = initializeChatTimeline() ;
    addScenery(timeline, table, "default_world", mat4(1), 10000,20, 7, false);
    addTestShapes(timeline);
    mat4 sky_mat(1.0);
    sky_mat[0][0] = 0 ;
    sky_mat[0][2] =1;
    sky_mat[2][0] = -1;
    sky_mat[2][2] = 0 ;
    std::unique_ptr<MeshInstance> o = std::make_unique<MeshInstance>(glm::vec3(0,0,0), 100, "server", "default_skybox", sky_mat , Variant(), false) ;
    timeline->createObject(std::move(o), std::unique_ptr<TEvent>(nullptr) , timeline->current_time + 0.01);

    // Read the password from a file so we don't have to type it (and it's not included in the source repository)
    /*std::ifstream password_file("./cert/password.txt");
    std::string password;
    std::getline(password_file, password); */

    // boot up the tableserver on a non-blocking thread
    int table_port = 9004;
    printf("Starting the table server on port %d...\n", table_port);
    TableServer table_server(table_port, &table,
        "./cert/cert.pem", "./cert/key.pem", "./cert/dh.pem", "");

    // boot up the timeline server on a non-blocking thread
    int timeline_port = 9017;
    cout << "Starting the timeline server on port " << timeline_port << "..." << endl;
    
    //addDragon(timeline, table);

    TimelineServer timeline_server(timeline_port, timeline,
        "./cert/cert.pem", "./cert/key.pem", "./cert/dh.pem", "");

    cout << "Starting main loop..." << endl;
    signal(SIGINT, quit); // Catch CTRL-C to exit gracefully
    
    timeline->auto_clear_history = true;
    timeline->observable_interpolation = false;
    while (running) {
        //animateDragon(timeline);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        timeline->run();
        TimelineServer::quickForwardEvents();
    }
    //web_server.stop();
    timeline_server.stop();
}
