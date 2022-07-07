#include "WebServer.h"
#include "TableServer.h"
#include "TimelineServer.h"
#include "api.cpp"


#include <signal.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <thread>
#include <chrono>

using glm::vec3;
using glm::mat4;
using std::vector;
using std::cout;
using std::endl;
using std::unordered_map ;


void runUnitTests(){
    bool g = UnitTests::runAll();
}

bool running = true;

// Define the function to be called when ctrl-c (SIGINT) is sent to process
void quit(int signum) {
    printf("Shutting down...");
    running = false;
}

int main(int argc, char** argv) {

    byte* packet_ptr = (byte*)malloc(50000000); // 50 megabytes
    setPacketPointer(packet_ptr);

    runUnitTests();
    
    
    // boot up a static webserver on a nonblocking thread to serve the frontend
    char http_address[] = "0.0.0.0";
    int http_port = 8080;
    char http_root[] = "hosted"; // relative path from server binary to hosted files
    cout << "Starting the webserver on port " << http_port << "..." << endl;
    WebServer web_server(http_address, http_port, http_root);

    unordered_map<string, Variant> table;

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
    
    timeline->auto_clear_history=true;
        timeline->observable_interpolation = false;
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        timeline->run();
        TimelineServer::quickForwardEvents();
    }
    web_server.stop();
    timeline_server.stop();
    
}
