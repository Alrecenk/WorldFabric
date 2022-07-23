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
    models["room"] = "./models/room.glb";
    models["sample"] = "./models/Rin.glb";
    for(auto& [key, path] : models){
        std::ifstream input( path.c_str(), std::ios::binary );
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});
        table[key] = Variant(&(buffer[0]), buffer.size());
        printf("Loaded model %s (%d bytes = %d)\n",key.c_str(), (int) buffer.size(), table[key].getArrayLength());
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

    GLTF room ;
    room.receiveTableData("room", table["room"]) ;
    glm::mat4 pose(1);
    pose[0][0] = 2 ;
    pose[1][1] = 2 ;
    pose[2][2] = 2 ;

    Variant bones; // = room.getBoneData();
    std::unique_ptr<MeshInstance> o = std::make_unique<MeshInstance>(glm::vec3(0,0,0), 2, "server", "room", pose, bones, false) ;
    
    /*
    auto serial = o->serialize();
    Variant(serial).printFormatted();
    
    o->set(serial);
    serial = o->serialize();
    Variant(serial).printFormatted();
    */
    
    timeline->createObject(std::move(o), std::unique_ptr<TEvent>(nullptr) , 0.01234);
    timeline->run(1.0) ;
    


    TimelineServer timeline_server(timeline_port, timeline,
        "./cert/cert.pem", "./cert/key.pem", "./cert/dh.pem", "");

    cout << "Starting main loop..." << endl;
    signal(SIGINT, quit); // Catch CTRL-C to exit gracefully
    
    timeline->auto_clear_history = true;
    timeline->observable_interpolation = false;
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        pose[3][0] = sin(timeline->current_time);
        pose[3][2] = cos(timeline->current_time);
        //timeline->addEvent(std::make_unique<SetMeshInstance>(1, glm::vec3(0,0,0), 2, "default_avatar", pose, bones),  timeline->current_time+0.1) ;

        timeline->run();
        TimelineServer::quickForwardEvents();
    }
    //web_server.stop();
    timeline_server.stop();
    
    
}
