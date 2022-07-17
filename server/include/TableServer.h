#ifndef _TABLESERVER_H_
#define _TABLESERVER_H_ 1

//#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>
#include "Variant.h"

#include <thread>
#include <memory>
#include <cstdint>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>
#include <unordered_set>
#include <unordered_map>


typedef websocketpp::server<websocketpp::config::asio_tls> SecureSocketServer;
//typedef websocketpp::server<websocketpp::config::asio> SocketServer;
typedef SecureSocketServer::message_ptr MessagePointer;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> ContextPointer;

// See https://wiki.mozilla.org/Security/Server_Side_TLS for more details about
// the TLS modes. The code below demonstrates how to implement both the modern
enum tls_mode {
    MOZILLA_INTERMEDIATE = 1,
    MOZILLA_MODERN = 2
};

class TableServer {
  private:
    // A pointer to table of data
    static std::unordered_map<std::string, Variant>* table; // TODO figure out how to make not static?
    std::thread thread; // Thread that makes this server nonblocking
    static SecureSocketServer server; // The websocket++ server object
    static std::string password ;
    static std::string cert_file;
    static std::string private_key_file;
    static std::string dh_file;


    // Function called when the socket gets a message
    // Messages for TableServer consist of a VARIANT_ARRAY type Variant containing string keys for reads or objects for writes
    // an OBJECT type Variant (map) is returned with the keys and values for each requested item
    static void onMessage(SecureSocketServer* s, websocketpp::connection_hdl hdl, MessagePointer msg);

    static ContextPointer on_tls_init(tls_mode mode, websocketpp::connection_hdl hdl);

    static std::string get_password();

  public:

    static std::unordered_set<std::string> null_requests; // keys requested that were not found get added to this
    static std::unordered_set<std::string> external_writes; // keys written to the table fro mthe websocket get added here

    //Starts the server on creation
    TableServer(int socket_port, std::unordered_map<std::string, Variant>* table,
        std::string cert_file, std::string private_key_file, std::string dh_file, std::string password);



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
