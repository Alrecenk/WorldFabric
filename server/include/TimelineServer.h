#ifndef _TIMELINE_SERVER_H_
#define _TIMELINE_SERVER_H_ 1

//#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>
#include "Variant.h"
#include "Timeline.h"

#include <thread>
#include <memory>
#include <cstdint>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <map>
#include <vector>


typedef websocketpp::server<websocketpp::config::asio_tls> SecureSocketServer;
//typedef websocketpp::server<websocketpp::config::asio> SocketServer;
typedef SecureSocketServer::message_ptr MessagePointer;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> ContextPointer;


class TimelineServer {

    


    public:
        static Timeline* timeline; // TODO figure out how to make not static?
        static std::map<websocketpp::connection_hdl, long, std::owner_less<websocketpp::connection_hdl>> connections ; // map of open connections to the time of the last receieved packet

        //Starts the server on creation
        TimelineServer(int socket_port, Timeline* tl,
        std::string cert_file, std::string private_key_file, std::string dh_file, std::string password);

        // Starts the server socket
        static void start(int socket_port);

        // Stops the server
        static void stop();

        static void quickForwardEvents();

        static long timeMilliseconds();

  private:

    // See https://wiki.mozilla.org/Security/Server_Side_TLS for more details about
    // the TLS modes. The code below demonstrates how to implement both the modern
    enum tls_mode {
        MOZILLA_INTERMEDIATE = 1,
        MOZILLA_MODERN = 2
    };

    std::thread thread; // Thread that makes this server nonblocking
    static SecureSocketServer server; // The websocket++ server object
    static std::string password ;
    static std::string cert_file;
    static std::string private_key_file;
    static std::string dh_file;

    // Function called when the socket gets a message
    // Messages for TableServer consist of a VARIANT_ARRAY type Variant containing string keys
    // an OBJECT type Variant (map) is returned with the keys and values for each requested item
    static void onMessage(
            SecureSocketServer* s, websocketpp::connection_hdl hdl,
            MessagePointer msg);

    static ContextPointer on_tls_init(tls_mode mode, websocketpp::connection_hdl hdl);

    static std::string get_password();

};

#endif // #ifndef _TIMELINE_SERVER_H_
