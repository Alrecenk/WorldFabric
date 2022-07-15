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






#define CPPHTTPLIB_OPENSSL_SUPPORT
// openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 365

#include <cstdio>
#include <httplib.h>
#include <iostream>

#define SERVER_CERT_FILE "./cert/cert.pem"
#define SERVER_PRIVATE_KEY_FILE "./cert/key.pem"

using namespace httplib;
using namespace std;

string dump_headers(const Headers &headers) {
  string s;
  char buf[BUFSIZ];

  for (const auto &x : headers) {
    snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
    s += buf;
  }

  return s;
}

string dump_multipart_files(const MultipartFormDataMap &files) {
  string s;
  char buf[BUFSIZ];

  s += "--------------------------------\n";

  for (const auto &x : files) {
    const auto &name = x.first;
    const auto &file = x.second;

    snprintf(buf, sizeof(buf), "name: %s\n", name.c_str());
    s += buf;

    snprintf(buf, sizeof(buf), "filename: %s\n", file.filename.c_str());
    s += buf;

    snprintf(buf, sizeof(buf), "content type: %s\n", file.content_type.c_str());
    s += buf;

    snprintf(buf, sizeof(buf), "text length: %zu\n", file.content.size());
    s += buf;

    s += "----------------\n";
  }

  return s;
}

string log(const Request &req, const Response &res) {
  string s;
  char buf[BUFSIZ];

  s += "================================\n";

  snprintf(buf, sizeof(buf), "%s %s %s", req.method.c_str(),
           req.version.c_str(), req.path.c_str());
  s += buf;

  string query;
  for (auto it = req.params.begin(); it != req.params.end(); ++it) {
    const auto &x = *it;
    snprintf(buf, sizeof(buf), "%c%s=%s",
             (it == req.params.begin()) ? '?' : '&', x.first.c_str(),
             x.second.c_str());
    query += buf;
  }
  snprintf(buf, sizeof(buf), "%s\n", query.c_str());
  s += buf;

  s += dump_headers(req.headers);
  s += dump_multipart_files(req.files);

  s += "--------------------------------\n";

  snprintf(buf, sizeof(buf), "%d\n", res.status);
  s += buf;
  s += dump_headers(res.headers);

  return s;
}

int httpsmain(int argc, const char **argv) {
  if (argc > 1 && string("--help") == argv[1]) {
    cout << "usage: simplesvr [PORT] [DIR]" << endl;
    return 1;
  }

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    printf("opening https server with certs!\n");
  SSLServer svr(SERVER_CERT_FILE, SERVER_PRIVATE_KEY_FILE);
#else
    printf("opening unsecured http server!\n");
  Server svr;
#endif

  svr.Post("/multipart", [](const Request &req, Response &res) {
    auto body = dump_headers(req.headers) + dump_multipart_files(req.files);

    res.set_content(body, "text/plain");
  });

  svr.set_error_handler([](const Request & /*req*/, Response &res) {
    const char *fmt = "<p>Error Status: <span style='color:red;'>%d</span></p>";
    char buf[BUFSIZ];
    snprintf(buf, sizeof(buf), fmt, res.status);
    res.set_content(buf, "text/html");
  });

  svr.set_logger(
      [](const Request &req, const Response &res) { cout << log(req, res); });

  auto port = 8080;
  if (argc > 1) { port = atoi(argv[1]); }

  auto base_dir = "./";
  if (argc > 2) { base_dir = argv[2]; }

  if (!svr.set_mount_point("/", base_dir)) {
    cout << "The specified base directory doesn't exist...";
    return 1;
  }

  cout << "The server started at port " << port << "..." << endl;

  //svr.listen("localhost", port);
    svr.listen("0.0.0.0",port);
  return 0;
}









int main(int argc, const char** argv) {
    //httpsmain(argc, argv);
    

    unsigned char* packet_ptr = (unsigned char*)malloc(50000000); // 50 megabytes
    setPacketPointer(packet_ptr);

    runUnitTests();
    
    
    // boot up a static webserver on a nonblocking thread to serve the frontend
    char http_address[] = "0.0.0.0";
    int http_port = 8080;
    char http_root[] = "hosted"; // relative path from server binary to hosted files
    cout << "Starting the webserver on port " << http_port << "..." << endl;
    WebServer web_server(http_address, http_port, http_root);

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
