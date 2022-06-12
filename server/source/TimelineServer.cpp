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
double TimelineServer::base_age=0.25;

TimelineServer::TimelineServer(int socket_port, Timeline* tl, double sync_age) {
    TimelineServer::timeline = tl;
    TimelineServer::base_age = sync_age ;
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
    // Fetch raw bytes from binary packet
    const char* packet_bytes = msg->get_payload().c_str();
    Variant packet_variant(
            Variant::VARIANT_ARRAY, (unsigned char*) packet_bytes);

    Variant response = timeline->synchronize(packet_variant, TimelineServer::base_age) ;

    int response_size = response.getSize();
    //TODO this delay should be on the frontend, not the backend
    // TODO less hardcoded way to limit frequency of table packets
    sleep_for(std::chrono::milliseconds(5));
    // Send the data back to the client
    s->send(
            std::move(hdl), response.ptr, response_size,
            websocketpp::frame::opcode::binary);
}

void TimelineServer::stop() {
    TimelineServer::server.stop_listening(); // TODO verify this works
}