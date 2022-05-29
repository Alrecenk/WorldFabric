#include "WebServer.h"
#include "TableServer.h"
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

long timeMilliseconds() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}

float randomFloat() {
    return (float) ((rand() % 1000000) / 1000000.0);
}

// Add component data to the static table for instances
void addModels(map<string, Variant>& table) {
    
}


bool running = true;

// Define the function to be called when ctrl-c (SIGINT) is sent to process
void quit(int signum) {
    printf("Shutting down...");
    running = false;
}

int main(int argc, char** argv) {
    map<string, Variant> table;
    addModels(table);
    // boot up a static webserver on a nonblocking thread to serve the frontend
    char http_address[] = "0.0.0.0";
    int http_port = 8080;
    char http_root[] = "hosted"; // relative path from server binary to hosted files
    cout << "Starting the webserver on port " << http_port << "..." << endl;
    WebServer web_server(http_address, http_port, http_root);

    // boot up the tableserver on a non-blocking thread
    int table_port = 9004;
    cout << "Starting the table server on port " << table_port << "..." << endl;
    TableServer table_server(table_port, &table);

    cout << "Starting main loop..." << endl;
    signal(SIGINT, quit); // Catch CTRL-C to exit gracefully
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        //TODO stuff
    }
    web_server.stop();
    table_server.stop();
}
