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
    models["eroom"] = "./models/room.glb";
    models["dragon"] = "./models/dragon.glb";
    models["bunny"] = "./models/bunny350.glb";
    models["sylveon"] = "./models/sylveon7k.glb";
    //models["sample"] = "./models/Rin.glb";
    for(auto& [key, path] : models){
        std::ifstream input( path.c_str(), std::ios::binary );
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});
        table[key] = Variant(&(buffer[0]), buffer.size());
        printf("Loaded model %s (%d bytes = %d)\n",key.c_str(), (int) buffer.size(), table[key].getArrayLength());
    }
}

void addRoom(Timeline* timeline){
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
    
    Variant bones;
    std::unique_ptr<MeshInstance> o = std::make_unique<MeshInstance>(glm::vec3(0,0,0), 2, "server", "eroom", pose, bones, false) ;
    timeline->createObject(std::move(o), std::unique_ptr<TEvent>(nullptr) , 0.01234);
    // Room models as a 3 wall diorama so mirror it and to make a complete room
    
    pose = mat4(0);
    pose[0][0] = 2/size  ;
    pose[1][2] = 2/size  ;
    pose[2][1] = 2/size  ;

    pose[3][0] = 0.0001; // tiny x offset prevents z fighting on overlap
    pose[3][1] = -1 ;
    pose[3][2] = 1.75;
    pose[3][3] = 1;
    
    o = std::make_unique<MeshInstance>(glm::vec3(0,0,0), 2, "server", "eroom", pose, bones, false) ;
    timeline->createObject(std::move(o), std::unique_ptr<TEvent>(nullptr) , 0.01235);
    
}

void addTestShapes(Timeline* timeline){

    double st = timeline->current_time ;
    
    std::unique_ptr<ConvexShape> shape0 = std::make_unique<ConvexShape>(ConvexShape::makeTetra(
        vec3(-.1,.1,0),vec3(-.1,-.1,0),vec3(.1,0,-.1),vec3(.1,0,.1)));
    float mass = 1;    
    timeline->createObject(std::move(shape0), std::unique_ptr<TEvent>(nullptr), "tetra_shape_created",st+ 0.01 + 0.01*randomFloat());
    timeline->subscribe("tetra_solid_maker", "tetra_shape_created",
    [timeline,st,mass](const string& subscriber, const string& trigger, const Variant& data){
        int shape_id = data.getInt();
        for(float x = -1.5; x < 1.5; x+=0.65f){
        for(float y = 0; y < 1; y+=0.65f){
            std::unique_ptr<ConvexSolid> solid = std::make_unique<ConvexSolid>(
                ConvexSolid(shape_id, mass, glm::vec3(x,y,-0.75f), glm::quat(0,0,0,1)));
            timeline->createObject(std::move(solid), std::make_unique<MoveSimpleSolid>(1.0/90), st+ 1.1 + 0.01*randomFloat() );
        }}
    });


    std::unique_ptr<ConvexShape> shape = std::make_unique<ConvexShape>(ConvexShape::makeAxisAlignedBox(vec3(0.25,0.25,0.25)));
    mass = 1;
    timeline->createObject(std::move(shape), std::unique_ptr<TEvent>(nullptr), "box_shape_created", st+ + 0.01*randomFloat());
    timeline->subscribe("box_solid_maker", "box_shape_created",
    [timeline,st,mass](const string& subscriber, const string& trigger, const Variant& data){
        int shape_id = data.getInt();
        for(float x = -1.5; x < 1.5; x+=0.65f){
        for(float y = 0; y < 1; y+=0.65f){
            std::unique_ptr<ConvexSolid> solid = std::make_unique<ConvexSolid>(
                ConvexSolid(shape_id, mass, glm::vec3(x,y,-0.25), glm::quat(0,0,0,1)));
            timeline->createObject(std::move(solid), std::make_unique<MoveSimpleSolid>(1.0/90), st+ 1.1 + 0.01*randomFloat() );
        }}
    });


    std::unique_ptr<ConvexShape> shape2 = std::make_unique<ConvexShape>(ConvexShape::makeCylinder(glm::vec3(0,0.2,0), glm::vec3(0,-0.2,0), 0.05, 16));
    mass = 1 ;
    timeline->createObject(std::move(shape2), std::unique_ptr<TEvent>(nullptr), "cylinder_shape_created", st+ + 0.01*randomFloat() );
    timeline->subscribe("cylinder_solid_maker", "cylinder_shape_created",
    [timeline,st,mass](const string& subscriber, const string& trigger, const Variant& data){
        int shape_id = data.getInt();
        for(float x = -1.5; x < 1.5; x+=0.65f){
        for(float y = 0; y < 1; y+=0.65f){
            std::unique_ptr<ConvexSolid> solid = std::make_unique<ConvexSolid>(
                ConvexSolid(shape_id, mass, glm::vec3(x,y,0.25), glm::quat(0,0,0,1)));
            timeline->createObject(std::move(solid), std::make_unique<MoveSimpleSolid>(1.0/90), st+ 1.1 + 0.01*randomFloat() );
        }}
    });
    

    std::unique_ptr<ConvexShape> shape3 = std::make_unique<ConvexShape>(ConvexShape::makeSphere(glm::vec3(0,0,0), 0.15, 2));
    mass = 1;
    timeline->createObject(std::move(shape3), std::unique_ptr<TEvent>(nullptr), "sphere_shape_created", st+ 0.01*randomFloat() );
    timeline->subscribe("sphere_solid_maker", "sphere_shape_created",
    [timeline,st, mass](const string& subscriber, const string& trigger, const Variant& data){
        int shape_id = data.getInt();
        for(float x = -1.5; x < 1.5; x+=0.65f){
        for(float y = 0; y < 1; y+=0.65f){
            std::unique_ptr<ConvexSolid> solid = std::make_unique<ConvexSolid>(
                ConvexSolid(shape_id, mass, glm::vec3(x,y,0.75), glm::quat(0,0,0,1)));
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


void addBreakable(Timeline* timeline, unordered_map<string, Variant>& table, string mesh_name, mat4 transform){
    double st = timeline->current_time ;
    std::shared_ptr<GLTF> mesh = meshes[mesh_name];
    if(mesh == nullptr){
         meshes.receiveTableData(mesh_name,table[mesh_name]);
         mesh = meshes[mesh_name];
    }

    
    mesh->transform = transform ;
    mesh->computeNodeMatrices();
    mesh->applyTransforms();

    std::unique_ptr<BSPNode> root = make_unique<BSPNode>(mesh);
    root->computeVolumeInside();
    //root->print("  ");
    map<string,vec3> part_position ;
    map<string,vec3>* part_ptr = &part_position ;

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
        }else if(node->leaf_inside){
            generate = true;
        }
        if(generate){
            
            //bool valid = Polygon::checkConvex(node->shape);
            std::unique_ptr<ConvexShape> shape = std::make_unique<ConvexShape>(node->shape);
            /*
            vector<Polygon> convert_back = shape->getPolygons();
            bool still_valid = Polygon::checkConvex(convert_back);
            if(valid && !still_valid){
                printf("Error in poly shape conversion!\n");
            }

            auto serial = shape->serialize();
            ConvexShape deserial;
            deserial.set(serial);
            convert_back = deserial.getPolygons();
            bool serial_valid = Polygon::checkConvex(convert_back);
            if(valid && !serial_valid){
                printf("broken by serialization!\n");
            }
            */
            
            vec3 part_position = -(shape->centerOnCentroid());
            if(shape->vertex.size() > 3){
                string trigger = mesh_name + " " + std::to_string(n) ;
                n++;
                
                //printf("position: %f, %f, %f \n", part_position[trigger][0], part_position[trigger][1], part_position[trigger][2]);
                float mass = shape->getVolume();
                
                //Variant(shape->serialize()).printFormatted();
                timeline->createObject(std::move(shape), std::unique_ptr<TEvent>(nullptr), trigger,st+ 0.01 + 0.01*randomFloat());
                
                timeline->subscribe(trigger, trigger,
                [timeline,st,mass, part_position](const string& subscriber, const string& trigger, const Variant& data){
                    int shape_id = data.getInt();
                    //printf("Trigger : %s \n", trigger.c_str());                   
                    
                    std::unique_ptr<ConvexSolid> solid = std::make_unique<ConvexSolid>(
                        ConvexSolid(shape_id, mass, part_position, glm::quat(1,0,0,0)));
                        solid->moveable = false;

                    //printf("id: %d  position : %f, %f, %f   radius: %f\n", shape_id, position[0], position[1], position[2], solid->radius);   
                    timeline->createObject(std::move(solid), std::unique_ptr<TEvent>(nullptr)/*std::make_unique<MoveSimpleSolid>(1.0/30)*/, st+ 1.1 + 0.01*randomFloat() );
                    
                });
                
            }
            
        }
    }
  
    printf("A1\n");
    timeline->run(st+ 1.0) ;// notifications go out after a call to run completes
    printf("A2\n");
    timeline->run(st+ 2.0) ; // run some more to get the solid made
    printf("A3\n");

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
    
   
    //Timeline* timeline = generateBallTimeline() ;
    Timeline* timeline = initializeChatTimeline() ;

    addRoom(timeline);
    addTestShapes(timeline);
  
    /*
    string mesh_name = "sylveon" ;
    std::shared_ptr<GLTF> mesh = meshes[mesh_name];
    if(mesh == nullptr){
         meshes.receiveTableData(mesh_name,table[mesh_name]);
         mesh = meshes[mesh_name];
    }

    float size = 0 ;
    vec3 center(0,0,0);
    for(int k=0;k<3;k++){
        center[k] = (mesh->max[k]+ mesh->min[k])*0.5f ;
        size = fmax(size , fabs(mesh->max[k]-center[k]));
        size = fmax(size , fabs(mesh->min[k]-center[k]));
        
    }

    mat4 bt = mat4(1);
    bt = glm::scale(mat4(1), {(1.0f/size),(1.0f/size),(1.0f/size)});
    bt = glm::translate(bt, center*-1.0f);


    addBreakable(timeline, table, mesh_name, bt);
*/
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
