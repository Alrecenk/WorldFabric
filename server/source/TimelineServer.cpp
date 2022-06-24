#include "TimelineServer.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <utility>
#include <chrono>

typedef websocketpp::server<websocketpp::config::asio> SocketServer;
typedef SocketServer::message_ptr MessagePointer;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using std::string;
using std::vector;
using std::map;
using std::this_thread::sleep_for;

// Initialize static members
Timeline* TimelineServer::timeline;
map<websocketpp::connection_hdl, long, std::owner_less<websocketpp::connection_hdl>> TimelineServer::connections ;
SocketServer TimelineServer::server;

TimelineServer::TimelineServer(int socket_port, Timeline* tl) {
    TimelineServer::timeline = tl;
    this->thread = std::thread(TimelineServer::start, socket_port);
}

long TimelineServer::timeMilliseconds() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}

void TimelineServer::start(const int socket_port) {
    try {
        // Disable logging to console
        TimelineServer::server.clear_access_channels(
                websocketpp::log::alevel::all);
        TimelineServer::server.init_asio();
        TimelineServer::server.set_message_handler(
                bind(&TimelineServer::onMessage, &server, ::_1, ::_2));
        TimelineServer::server.listen(socket_port);
        TimelineServer::server.start_accept();
        TimelineServer::server.run();
    } catch (websocketpp::exception const& e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
}

// Function called when the socket gets a message
// Messages for TimelineServer consist of a VARIANT_ARRAY type Variant containing string keys
// an OBJECT type Variant (map) is returned with the keys and values for each requested item
void TimelineServer::onMessage(
        SocketServer* s, websocketpp::connection_hdl hdl, MessagePointer msg) {
            TimelineServer::connections[hdl] = TimelineServer::timeMilliseconds();
            //printf("Got message!\n");
    // Fetch raw bytes from binary packet
    const char* packet_bytes = msg->get_payload().c_str();
    Variant packet_variant(
            Variant::OBJECT, (unsigned char*) packet_bytes);
    //printf("got packet:\n");
    //packet_variant.printFormatted();
    std::map<std::string, Variant> packet_map = packet_variant.getObject() ;
    std::map<std::string, Variant> response_map = TimelineServer::timeline->synchronize(packet_map, true) ;
    if(response_map.size()>0){
        Variant response = Variant(response_map);
        int response_size = response.getSize();
        // Send the data back to the client
        s->send(
                std::move(hdl), response.ptr, response_size,
                websocketpp::frame::opcode::binary);
    }
}

void TimelineServer::stop() {
    TimelineServer::server.stop_listening(); // TODO verify this works
}

void TimelineServer::quickForwardEvents(){
    Variant qs = timeline->popQuickSends() ;
    if(qs.defined()){
        long current_time = TimelineServer::timeMilliseconds();
        int packet_size = qs.getSize();
        // Send to all connected clients
        for(auto& [connection, time] : TimelineServer::connections){
            if(time > current_time-1000){
                TimelineServer::server.send(
                    connection, qs.ptr, packet_size,
                    websocketpp::frame::opcode::binary);
            }
        }
    }
}