#include "TableServer.h"
#include "TableInterface.h"
//#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

#include <iostream>


#include <utility>

typedef websocketpp::server<websocketpp::config::asio_tls> SecureSocketServer;
//typedef websocketpp::server<websocketpp::config::asio> SocketServer;
typedef SecureSocketServer::message_ptr MessagePointer;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> ContextPointer;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using std::string;
using std::vector;
using std::unordered_map;
using std::map;
using std::this_thread::sleep_for;

// Initialize static members
std::unordered_map<string, Variant>* TableServer::table;
std::unordered_set<string> TableServer::null_requests ;
std::unordered_set<string> TableServer::external_writes ;
SecureSocketServer TableServer::server;

std::string TableServer::password ;
std::string TableServer::cert_file;
std::string TableServer::private_key_file;
std::string TableServer::dh_file;

TableServer::TableServer(
        const int socket_port, std::unordered_map<string, Variant>* table_ptr,
        std::string cert_file, std::string private_key_file, std::string dh_file, std::string password) {
    TableServer::table = table_ptr;
    TableServer::cert_file = cert_file;
    TableServer::private_key_file = private_key_file;
    TableServer::dh_file = dh_file;
    TableServer::password = password;
    this->thread = std::thread(TableServer::start, socket_port);
}

void TableServer::start(const int socket_port) {
    try {
        // Disable logging to console
        TableServer::server.clear_access_channels(
                websocketpp::log::alevel::all);
        TableServer::server.init_asio();
        TableServer::server.set_message_handler(
                bind(&TableServer::onMessage, &server, ::_1, ::_2));
        TableServer::server.set_tls_init_handler(bind(&TableServer::on_tls_init,MOZILLA_INTERMEDIATE,::_1));
        TableServer::server.listen(socket_port);
        TableServer::server.start_accept();
        TableServer::server.run();
    } catch (websocketpp::exception const& e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
}

// Function called when the socket gets a message
// Messages for TableServer consist of a VARIANT_ARRAY type Variant containing string keys for reads or objects for writes
// an OBJECT type Variant (map) is returned with the keys and values for each requested item
void TableServer::onMessage(
        SecureSocketServer* s, websocketpp::connection_hdl hdl, MessagePointer msg) {
    // Fetch raw bytes from binary packet
    const char* packet_bytes = msg->get_payload().c_str();
    Variant packet_variant(
            Variant::VARIANT_ARRAY, (unsigned char*) packet_bytes);
    vector<Variant> packet = packet_variant.getVariantArray();
    map<string, Variant> result;
    for (auto& request : packet) {
        if(request.type == Variant::STRING){ // Read requests are a single string
            string key_string = request.getString();
            result[key_string] = TableServer::getEntry(key_string);
            if(!result[key_string].defined()){
                null_requests.insert(key_string);
            }
        }else if(request.type == Variant::OBJECT){ //write requests are an object with keyand value
            //printf("Got write:\n");
            //request.printFormatted();
            string key_string = request[TableInterface::KEY].getString();
            (*table)[key_string] = request[TableInterface::VALUE].clone(); // TODO move operation is probably possible since this is from getVariantArray local
            external_writes.insert(key_string);
        }
    }
    Variant response(result);
    int response_size = response.getSize();
    //printf("Response size: %d\n", response_size);
    // TODO less hardcoded way to limit frequency of table packets
    sleep_for(std::chrono::milliseconds(50));
    // Send the data back to the client
    websocketpp::lib::error_code ec; // TODO catch error
    s->send(
            std::move(hdl), response.ptr, response_size,
            websocketpp::frame::opcode::binary, ec);
}


std::string TableServer::get_password() {
    return password;
}



ContextPointer TableServer::on_tls_init(tls_mode mode, websocketpp::connection_hdl hdl) {
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
        ctx->set_password_callback(bind(&TableServer::get_password));
        ctx->use_certificate_chain_file(TableServer::cert_file);
        ctx->use_private_key_file(TableServer::private_key_file, asio::ssl::context::pem);
        
        // Example method of generating this file:
        // `openssl dhparam -out dh.pem 2048`
        // Mozilla Intermediate suggests 1024 as the minimum size to use
        // Mozilla Modern suggests 2048 as the minimum size to use.
        ctx->use_tmp_dh_file(TableServer::dh_file);
        
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



void TableServer::stop() {
    TableServer::server.stop_listening(); // TODO verify this works
}

// Returns a pointer to an entry from the table
Variant TableServer::getEntry(const string& key) {
    if (table->find(key) == table->end()) {
        return Variant(); // NULL
    } else {
        return (*table)[key]; // TODO can we avoid this copy since it ends up a const pointer before it gets exposed?
    }
}