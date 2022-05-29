#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_ 1

#include <thread>
#include <memory>
#include <cstdint>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <event2/http.h>
#include <evhttp.h>
#include <event.h>
#include <sys/stat.h>
#include <fcntl.h>

class WebServer {
  private:
    // Thread that makes this server nonblocking
    std::thread thread_;

    // Serves static files from the root directory
    static void serveFile(evhttp_request* req, void*);

  public:
    //Start the server on creation
    WebServer(const char http_address[], int http_port, char http_root[]);

    // Runs the server
    static void run(const char http_address[], int http_port);

    static void stop(); // Stop the server
    static char* root_;

};

#endif // #ifndef _WEBSERVER_H_
