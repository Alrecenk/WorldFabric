#include "TableServer.h"
#include "TableInterface.h"
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
using std::unordered_map;
using std::map;
using std::this_thread::sleep_for;

// Initialize static members
std::unordered_map<string, Variant>* TableServer::table_;
std::unordered_set<string> TableServer::null_requests ;
std::unordered_set<string> TableServer::external_writes ;
SocketServer TableServer::server_;

TableServer::TableServer(
        const int socket_port, std::unordered_map<string, Variant>* table_ptr) {
    TableServer::table_ = table_ptr;
    this->thread_ = std::thread(TableServer::start, socket_port);
}

void TableServer::start(const int socket_port) {
    try {
        // Disable logging to console
        TableServer::server_.clear_access_channels(
                websocketpp::log::alevel::all);
        TableServer::server_.init_asio();
        TableServer::server_.set_message_handler(
                bind(&TableServer::onMessage, &server_, ::_1, ::_2));
        TableServer::server_.listen(socket_port);
        TableServer::server_.start_accept();
        TableServer::server_.run();
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
        SocketServer* s, websocketpp::connection_hdl hdl, MessagePointer msg) {
    // Fetch raw bytes from binary packet
    const char* packet_bytes = msg->get_payload().c_str();
    Variant packet_variant(
            Variant::VARIANT_ARRAY, (unsigned char*) packet_bytes);
    vector<Variant> packet = packet_variant.getVariantArray();
    map<string, Variant> result;
    for (auto& request : packet) {
        if(request.type_ == Variant::STRING){ // Read requests are a single string
            string key_string = request.getString();
            result[key_string] = TableServer::getEntry(key_string);
            if(!result[key_string].defined()){
                null_requests.insert(key_string);
            }
        }else if(request.type_ == Variant::OBJECT){ //write requests are an object with keyand value
            //printf("Got write:\n");
            //request.printFormatted();
            string key_string = request[TableInterface::KEY].getString();
            (*table_)[key_string] = request[TableInterface::VALUE].clone(); // TODO move operation is probably possible since this is from getVariantArray local
            external_writes.insert(key_string);
        }
    }
    Variant response(result);
    int response_size = response.getSize();
    //printf("Response size: %d\n", response_size);
    // TODO less hardcoded way to limit frequency of table packets
    sleep_for(std::chrono::milliseconds(50));
    // Send the data back to the client
    s->send(
            std::move(hdl), response.ptr, response_size,
            websocketpp::frame::opcode::binary);
}

void TableServer::stop() {
    TableServer::server_.stop_listening(); // TODO verify this works
}

// Returns a pointer to an entry from the table
Variant TableServer::getEntry(const string& key) {
    if (table_->find(key) == table_->end()) {
        return Variant(); // NULL
    } else {
        return (*table_)[key]; // TODO can we avoid this copy since it ends up a const pointer before it gets exposed?
    }
}