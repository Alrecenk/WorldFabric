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

int main(int argc, const char** argv) {

    unsigned char* packet_ptr = (unsigned char*)malloc(50000000); // 50 megabytes
    setPacketPointer(packet_ptr);

    runUnitTests();
    
    // boot up a static webserver on a nonblocking thread to serve the frontend
    // command for generating key files in linux
    // openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 365
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

    // boot up the tableserver on a non-blocking thread
    int table_port = 9004;
    printf("Starting the table server on port %d...\n", table_port);
    TableServer table_server(table_port, &table);

    // boot up the timeline server on a non-blocking thread
    int timeline_port = 9017;
    cout << "Starting the timeline server on port " << timeline_port << "..." << endl;
    int width = 600;
    int height = 600;
    float min_radius = 10;
    float max_radius = 15 ;
    float max_speed = 50 ;
    Timeline* timeline = initialize2DBallTimeline(width, height, 50, min_radius, max_radius, max_speed) ;
    TimelineServer timeline_server(timeline_port, timeline);

    cout << "Starting main loop..." << endl;
    signal(SIGINT, quit); // Catch CTRL-C to exit gracefully
    
    timeline->auto_clear_history = true;
    timeline->observable_interpolation = false;
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        timeline->run();
        TimelineServer::quickForwardEvents();
    }
    //web_server.stop();
    timeline_server.stop();
    
    
}
