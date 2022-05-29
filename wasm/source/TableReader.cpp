#include "TableReader.h"

using std::map;
using std::vector;
using std::unique_ptr;
using std::string;

// Maps from string key to a list of Readers that requested that key
map<string, vector<TableReader*>> TableReader::pending_requests_;
//Maps a key to a cached Variant
map<string, Variant> TableReader::cache_;
// Maps a key item already fetched to its dependencies required before it can be served
std::map<std::string, std::set<std::string>> TableReader::dep_on_;
// Maps items not yet fetched to items that depend on them before being served
std::map<std::string, std::set<std::string>> TableReader::dep_to_;
// An item currently being initialized (keys requested during initialization of an item become dependencies)
string TableReader::loading_key_ = "" ; 

Variant TableReader::null_variant =  Variant() ; // allows to return a reference to a null

// virtual destructor makes sure the right destructor is called on child classes
TableReader::~TableReader() {
    // Make sure element has no pending requests with a pointer to it
    if (pending_ != 0) {
        for (const auto&[key, value]: TableReader::pending_requests_) {
            vector<TableReader*> &requesters = TableReader::pending_requests_[key];
            for (int k = 0; k < requesters.size(); k++) {
                if (requesters[k] == this) {
                    requesters.erase(requesters.begin() + k);
                    break;
                }
            }
        }

    }
}

// Call this function to attempt to imemdiately fetch table data that has been fetched before from the cache
// Returns nullptr if the data is not in the cache, and then you'll need to use the async method
const Variant& TableReader::getCachedTableData(string key) {
    // in the cache
    if (TableReader::cache_.find(key) != TableReader::cache_.end() && dep_on_.find(key) == dep_on_.end()) {
        return TableReader::cache_[key];
    } else {
        return null_variant;
    }
}

// Call this function to asynchronously request data from the remote table
// recieveTableData will be called when the data is returned
void TableReader::requestTableData(string key) {
    pending_++;
    TableReader::pending_requests_[key].push_back(this);
    if(loading_key_.length()>0){ // Track dependencies if this request was made during loading of another entry
        dep_on_[loading_key_].insert(key);
        dep_to_[key].insert(loading_key_) ;
    }
}

// Returns a list of all pending requested keys in no particular order
// Used by the table socket client to batch and send requests to the server
vector<string> TableReader::getRequestedKeys() {
    vector<string> keys;
    keys.reserve(TableReader::pending_requests_.size());
    for (const auto &pair : TableReader::pending_requests_) {
        if(dep_on_.find(pair.first) == dep_on_.end()){ // Don't rerequest keys pending due to dependencies
            keys.push_back(pair.first);
        }
    }
    return keys;
}

// Called by the table socket client when a batch of data comes back from the server
// Distributes the data to all the objects that made requests
void TableReader::receiveTableData(const map<string, Variant> &data) {
    // Pass the data to requesters
    for (const auto&[key, value]: data) {
        if(TableReader::cache_.find(key) == TableReader::cache_.end()){
            // deserialize the data when received
            loading_key_ = key;
            //TODO intiialize element from value here that may request more keys to track dependencies
            loading_key_ = "";
            TableReader::cache_.emplace(key, value); // cache the data
            if(dep_on_.find(key) == dep_on_.end()){ // did not request any dependencies
                serveItem(key);
            }
        }
    }
}

//Serves an item to its requesters, may also trigger the sending of items that were waiting on the given item
void TableReader::serveItem(std::string key){
    Variant& s = TableReader::cache_[key];
    // Serve item to requesters
    vector<TableReader*> &requesters = TableReader::pending_requests_[key];
    for (auto &requester : requesters) {
        requester->pending_--;
        requester->receiveTableData(key, s);
    }
    // Remove all the served requests from pending
    TableReader::pending_requests_.erase(key);
    if(dep_to_.find(key) != dep_to_.end()){ // if had entries waiting on it
        for(const auto& dt : dep_to_[key]){
            dep_on_[dt].erase(key); // remove this as a pending dependency
            if(dep_on_[dt].size() == 0){ // last dependency met
                dep_on_.erase(dt);
                serveItem(dt);
            }
        }
        dep_to_.erase(key);
    }
}

// resolves all pending requests whose data is already in the cache
void TableReader::serveCachedRequests() {
    vector<string> served_keys;
    for (const auto &[key, requesters] : TableReader::pending_requests_) {
        // in the cache
        if (TableReader::cache_.find(key) != TableReader::cache_.end() && 
            dep_on_.find(key) == dep_on_.end()){// Not waiting on dependency
                Variant& s = TableReader::cache_[key];
                for (auto &requester : requesters) {
                    requester->pending_--;
                    requester->receiveTableData(key, s);
                }
            // can't remove keys while iterating the same map
            served_keys.push_back(key);
        }
    }
    // Remove all the served requests from pending requests
    for (const auto &key: served_keys) {
        TableReader::pending_requests_.erase(key);
    }
}

//Clears all the items in the cache
// You may need to call this before program exit to prevent a segfault on exit.
void TableReader::clearCache(){
    cache_.clear();
}