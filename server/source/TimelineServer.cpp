#include "TimelineServer.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <utility>

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
SocketServer TimelineServer::server;

TimelineServer::TimelineServer(int socket_port, Timeline* tl) {
    TimelineServer::timeline = tl;
    this->thread = std::thread(TimelineServer::start, socket_port);
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
            //printf("Got message!\n");
    // Fetch raw bytes from binary packet
    const char* packet_bytes = msg->get_payload().c_str();
    Variant packet_variant(
            Variant::OBJECT, (unsigned char*) packet_bytes);
    //printf("got packet:\n");
    //packet_variant.printFormatted();
    std::map<std::string, Variant> packet_map = packet_variant.getObject() ;
    std::map<std::string, Variant> response_map = TimelineServer::timeline->synchronize(packet_map, true) ;
    Variant response = Variant(response_map);
    int response_size = response.getSize();
    //printf("response packet:\n");
    //response.printFormatted();
    //TODO this delay should be on the frontend, not the backend
    // TODO less hardcoded way to limit frequency of table packets
    //sleep_for(std::chrono::milliseconds(20));
    // Send the data back to the client
    s->send(
            std::move(hdl), response.ptr, response_size,
            websocketpp::frame::opcode::binary);
}

void TimelineServer::stop() {
    TimelineServer::server.stop_listening(); // TODO verify this works
}