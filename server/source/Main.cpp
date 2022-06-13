#include "WebServer.h"
#include "TableServer.h"
#include "TimelineServer.h"
#include "api.cpp"


#include <signal.h>
#include <vector>
#include <map>
#include <thread>
#include <chrono>

using glm::vec3;
using glm::mat4;
using std::vector;
using std::cout;
using std::endl;



// Add component data to the static table for instances
void addModels(map<string, Variant>& table) {
    
}

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


/*
    map<string, Variant> obj ;
    obj["width"] = Variant(1920);
    obj["height"] = Variant(1080);
    obj["amount"] = Variant(10);
    obj["min_radius"] = Variant(40.0);
    obj["max_radius"] = Variant(100.0);
    obj["max_speed"] = Variant(200.0);
    Variant params = Variant(obj);



    initialize2DBallTimeline(params.ptr);
    printf("initialized!\n");

    for(double time = 1 ; time < 1000; time +=0.02){
        map<string, Variant> obj ;
        obj["time"] = Variant(time);
        Variant params = Variant(obj);
        runTimeline(params.ptr) ;
    }

    printf("Run completed!\n");
    */
    
    //map<string, Variant> table;
    //addModels(table);

    // boot up a static webserver on a nonblocking thread to serve the frontend
    char http_address[] = "0.0.0.0";
    int http_port = 8080;
    char http_root[] = "hosted"; // relative path from server binary to hosted files
    cout << "Starting the webserver on port " << http_port << "..." << endl;
    WebServer web_server(http_address, http_port, http_root);

    // boot up the tableserver on a non-blocking thread
    int timeline_port = 9017;
    cout << "Starting the timeline server on port " << timeline_port << "..." << endl;
    Timeline* timeline = initializeBallTimeline() ;
    TimelineServer timeline_server(timeline_port, timeline , base_age);

    cout << "Starting main loop..." << endl;
    signal(SIGINT, quit); // Catch CTRL-C to exit gracefully
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        //TODO stuff
    }
    web_server.stop();
    timeline_server.stop();
    
}
