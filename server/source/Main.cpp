#include "WebServer.h"
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

using glm::vec3;
using glm::mat4;
using std::vector;
using std::cout;
using std::endl;
using std::unordered_map ;
using std::map ;

void runUnitTests(){
    bool g = UnitTests::runAll();
}

void loadModels(unordered_map<string, Variant>& table){
    map<string,string> models ;
    models["default_avatar"] = "./models/default_avatar.vrm";
    models["eroom"] = "./models/room.glb";
    models["dragon"] = "./models/dragon.glb";
    //models["sample"] = "./models/Rin.glb";
    for(auto& [key, path] : models){
        std::ifstream input( path.c_str(), std::ios::binary );
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});
        table[key] = Variant(&(buffer[0]), buffer.size());
        printf("Loaded model %s (%d bytes = %d)\n",key.c_str(), (int) buffer.size(), table[key].getArrayLength());
    }
}

void addRoom(Timeline* timeline){
    glm::mat4 pose(1);
    pose[0][0] = 2 ;
    pose[1][1] = 2 ;
    pose[2][2] = 2 ;
    Variant bones;
    std::unique_ptr<MeshInstance> o = std::make_unique<MeshInstance>(glm::vec3(0,0,0), 2, "server", "eroom", pose, bones, false) ;
    timeline->createObject(std::move(o), std::unique_ptr<TEvent>(nullptr) , 0.01234);
    // Room models as a 3 wall diorama so mirror it and to make a complete room
    pose[0][0] = 2 ;
    pose[1][1] = 2 ;
    pose[2][2] = -2 ;
    pose[3][2] = 3.5;
    pose[3][0] = 0.0001; // tiny x offset prevents z fighting on overlap
    o = std::make_unique<MeshInstance>(glm::vec3(0,0,0), 2, "server", "eroom", pose, bones, false) ;
    timeline->createObject(std::move(o), std::unique_ptr<TEvent>(nullptr) , 0.01235);
}

void addTestShapes(Timeline* timeline){

    double st = timeline->current_time ;
    
 std::unique_ptr<ConvexShape> shape0 = std::make_unique<ConvexShape>(ConvexShape::makeTetra(
        vec3(-.1,.1,0),vec3(-.1,-.1,0),vec3(.1,0,-.1),vec3(.1,0,.1)));
    timeline->createObject(std::move(shape0), std::unique_ptr<TEvent>(nullptr), "tetra_shape_created",st+ 0.01 + 0.01*randomFloat());
    timeline->subscribe("tetra_solid_maker", "tetra_shape_created",
    [timeline,st](const string& subscriber, const string& trigger, const Variant& data){
        int box_id = data.getInt();
        for(float x = -1.75; x < 1.75; x+=0.65f){
        for(float y = 0; y < 2.5; y+=0.65f){
            std::unique_ptr<ConvexSolid> solid = std::make_unique<ConvexSolid>(
                ConvexSolid(glm::vec3(x,y,0.75), 0.15, 1, box_id, glm::vec3(0,0,0), glm::quat(0,0,0,1), glm::vec3(0.5,0.5,1.3)));
            timeline->createObject(std::move(solid), std::make_unique<MoveSimpleSolid>(1.0/90), st+ 1.1 + 0.01*randomFloat() );
        }}
    });

    std::unique_ptr<ConvexShape> shape = std::make_unique<ConvexShape>(ConvexShape::makeAxisAlignedBox(vec3(0.25,0.25,0.25)));
    timeline->createObject(std::move(shape), std::unique_ptr<TEvent>(nullptr), "box_shape_created", st+ + 0.01*randomFloat());
    timeline->subscribe("box_solid_maker", "box_shape_created",
    [timeline,st](const string& subscriber, const string& trigger, const Variant& data){
        int box_id = data.getInt();
        for(float x = -1.75; x < 1.75; x+=0.65f){
        for(float y = 0; y < 2.5; y+=0.65f){
            std::unique_ptr<ConvexSolid> solid = std::make_unique<ConvexSolid>(
                ConvexSolid(glm::vec3(x,y,1.25), 0.15, 1, box_id, glm::vec3(0,0,0), glm::quat(0,0,0,1), glm::vec3(0.5,0.5,1.3)));
            timeline->createObject(std::move(solid), std::make_unique<MoveSimpleSolid>(1.0/90), st+ 1.1 + 0.01*randomFloat() );
        }}
    });


    std::unique_ptr<ConvexShape> shape2 = std::make_unique<ConvexShape>(ConvexShape::makeCylinder(glm::vec3(0,0.15,0), glm::vec3(0,-0.15,0), 0.03, 16));
    timeline->createObject(std::move(shape2), std::unique_ptr<TEvent>(nullptr), "cylinder_shape_created", st+ + 0.01*randomFloat() );
    timeline->subscribe("cylinder_solid_maker", "cylinder_shape_created",
    [timeline,st](const string& subscriber, const string& trigger, const Variant& data){
        int shape_id = data.getInt();
        for(float x = -1.75; x < 1.75; x+=0.65f){
        for(float y = 0; y < 2.5; y+=0.65f){
            std::unique_ptr<ConvexSolid> solid = std::make_unique<ConvexSolid>(
                ConvexSolid(glm::vec3(x,y,1.75), 0.15, 1, shape_id, glm::vec3(0,0,0), glm::quat(0,0,0,1), glm::vec3(0.4,0.9,0.3)));
            timeline->createObject(std::move(solid), std::make_unique<MoveSimpleSolid>(1.0/90), st+ 1.1 + 0.01*randomFloat() );
        }}
    });
    
    std::unique_ptr<ConvexShape> shape3 = std::make_unique<ConvexShape>(ConvexShape::makeSphere(glm::vec3(0,0,0), 0.15, 2));
    timeline->createObject(std::move(shape3), std::unique_ptr<TEvent>(nullptr), "sphere_shape_created", st+ 0.01*randomFloat() );
    timeline->subscribe("sphere_solid_maker", "sphere_shape_created",
    [timeline,st](const string& subscriber, const string& trigger, const Variant& data){
        int shape_id = data.getInt();
        for(float x = -1.75; x < 1.75; x+=0.65f){
        for(float y = 0; y < 2.5; y+=0.65f){
            std::unique_ptr<ConvexSolid> solid = std::make_unique<ConvexSolid>(
                ConvexSolid(glm::vec3(x,y,2.25), 0.15, 1, shape_id, glm::vec3(0,0,0), glm::quat(0,0,0,1), glm::vec3(0.0,3.0,0.0)));
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
    /*
    char http_address[] = "0.0.0.0";
    int http_port = 8080;
    char http_root[] = "hosted"; // relative path from server binary to hosted files
    cout << "Starting the webserver on port " << http_port << "..." << endl;
    WebServer web_server(http_address, http_port, http_root);
    */

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
