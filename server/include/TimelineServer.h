#ifndef _TIMELINE_SERVER_H_
#define _TIMELINE_SERVER_H_ 1

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "Variant.h"
#include "Timeline.h"

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
#include <map>
#include <vector>


typedef websocketpp::server<websocketpp::config::asio> SocketServer;
typedef SocketServer::message_ptr MessagePointer;

class TimelineServer {

    public:
        static Timeline* timeline; // TODO figure out how to make not static?
        //Starts the server on creation
        TimelineServer(int socket_port, Timeline* tl);

        // Starts the server socket
        static void start(int socket_port);

        // Stops the server
        static void stop();

  private:
    // A pointer to table of data
    
    std::thread thread; // Thread that makes this server nonblocking
    static SocketServer server; // The websocket++ server object

    // Function called when the socket gets a message
    // Messages for TableServer consist of a VARIANT_ARRAY type Variant containing string keys
    // an OBJECT type Variant (map) is returned with the keys and values for each requested item
    static void onMessage(
            SocketServer* s, websocketpp::connection_hdl hdl,
            MessagePointer msg);

};

#endif // #ifndef _TIMELINE_SERVER_H_
