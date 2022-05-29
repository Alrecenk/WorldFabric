#ifndef _TABLESERVER_H_
#define _TABLESERVER_H_ 1

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "Variant.h"

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

class TableServer {
  private:
    // A pointer to table of data
    static std::map<std::string, Variant>* table_; // TODO figure out how to make not static?
    std::thread thread_; // Thread that makes this server nonblocking
    static SocketServer server_; // The websocket++ server object

    // Function called when the socket gets a message
    // Messages for TableServer consist of a VARIANT_ARRAY type Variant containing string keys
    // an OBJECT type Variant (map) is returned with the keys and values for each requested item
    static void onMessage(
            SocketServer* s, websocketpp::connection_hdl hdl,
            MessagePointer msg);

  public:
    //Starts the server on creation
    TableServer(int socket_port, std::map<std::string, Variant>* table);

    // Starts the server socket
    static void start(int socket_port);

    // Stops the server
    static void stop();

    // Returns a pointer to an entry from the table
    static Variant getEntry(const std::string& key);

    // Sets an entry in the table
    // Sets an entry in the table
    static void setEntry(const std::string& key, const Variant& data);

};

#endif // #ifndef _TABLESERVER_H_
