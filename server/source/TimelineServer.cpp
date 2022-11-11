#include "TimelineServer.h"
//#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

#include <utility>
#include <chrono>

typedef websocketpp::server<websocketpp::config::asio_tls> SecureSocketServer;
//typedef websocketpp::server<websocketpp::config::asio> SocketServer;
typedef SecureSocketServer::message_ptr MessagePointer;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> ContextPointer;

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
SecureSocketServer TimelineServer::server;
int TimelineServer::bytes_in = 0 ;
int TimelineServer::bytes_out = 0 ;

map<websocketpp::connection_hdl, std::unordered_map<int, TObject*>, std::owner_less<websocketpp::connection_hdl>> TimelineServer::descriptor_caches ;

std::string TimelineServer::password ;
std::string TimelineServer::cert_file;
std::string TimelineServer::private_key_file;
std::string TimelineServer::dh_file;

TimelineServer::TimelineServer(int socket_port, Timeline* tl,
        std::string cert_file, std::string private_key_file, std::string dh_file, std::string password) {
    TimelineServer::timeline = tl;
    TimelineServer::cert_file = cert_file;
    TimelineServer::private_key_file = private_key_file;
    TimelineServer::dh_file = dh_file;
    TimelineServer::password = password;
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
        TimelineServer::server.set_tls_init_handler(bind(&TimelineServer::on_tls_init,MOZILLA_INTERMEDIATE,::_1));
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
        SecureSocketServer* s, websocketpp::connection_hdl hdl, MessagePointer msg) {
            TimelineServer::connections[hdl] = TimelineServer::timeMilliseconds();
            //printf("Got message!\n");
    // Fetch raw bytes from binary packet
    auto payload = msg->get_payload() ;
    const char* packet_bytes = payload.c_str();
    bytes_in += payload.length();
    Variant packet_variant(
            Variant::OBJECT, (unsigned char*) packet_bytes);
    //printf("got packet:\n");
    //packet_variant.printFormatted();
    std::map<std::string, Variant> packet_map = packet_variant.getObject() ;
    std::map<std::string, Variant> response_map = TimelineServer::timeline->synchronize(packet_map, true, TimelineServer::descriptor_caches[hdl]) ;
    if(response_map.size()>0){
        Variant response = Variant(response_map);
        int response_size = response.getSize();
        // Send the data back to the client
        websocketpp::lib::error_code ec; // TODO catch error?
        s->send(
                std::move(hdl), response.ptr, response_size,
                websocketpp::frame::opcode::binary,ec);
        bytes_out += response_size;
    }
}


std::string TimelineServer::get_password() {
    return password;
}



ContextPointer TimelineServer::on_tls_init(tls_mode mode, websocketpp::connection_hdl hdl) {
    namespace asio = websocketpp::lib::asio;

    //std::cout << "on_tls_init called with hdl: " << hdl.lock().get() << std::endl;
    //std::cout << "using TLS mode: " << (mode == MOZILLA_MODERN ? "Mozilla Modern" : "Mozilla Intermediate") << std::endl;

    ContextPointer ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);

    try {
        if (mode == MOZILLA_MODERN) {
            // Modern disables TLSv1
            ctx->set_options(asio::ssl::context::default_workarounds |
                             asio::ssl::context::no_sslv2 |
                             asio::ssl::context::no_sslv3 |
                             asio::ssl::context::no_tlsv1 |
                             asio::ssl::context::single_dh_use);
        } else {
            ctx->set_options(asio::ssl::context::default_workarounds |
                             asio::ssl::context::no_sslv2 |
                             asio::ssl::context::no_sslv3 |
                             asio::ssl::context::single_dh_use);
        }
        ctx->set_password_callback(bind(&TimelineServer::get_password));
        ctx->use_certificate_chain_file(TimelineServer::cert_file);
        ctx->use_private_key_file(TimelineServer::private_key_file, asio::ssl::context::pem);
        
        // Example method of generating this file:
        // `openssl dhparam -out dh.pem 2048`
        // Mozilla Intermediate suggests 1024 as the minimum size to use
        // Mozilla Modern suggests 2048 as the minimum size to use.
        ctx->use_tmp_dh_file(TimelineServer::dh_file);
        
        std::string ciphers;
        
        if (mode == MOZILLA_MODERN) {
            ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK";
        } else {
            ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA";
        }
        
        if (SSL_CTX_set_cipher_list(ctx->native_handle() , ciphers.c_str()) != 1) {
            std::cout << "Error setting cipher list" << std::endl;
        }
    } catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    return ctx;
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
                websocketpp::lib::error_code ec; // TODO catch error
                TimelineServer::server.send(
                    connection, qs.ptr, packet_size,
                    websocketpp::frame::opcode::binary, ec);
                bytes_out += packet_size;
            }
        }
    }
}