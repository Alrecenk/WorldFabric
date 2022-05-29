#ifndef _TABLEREADER_H_
#define _TABLEREADER_H_ 1

#include "Variant.h"

#include <map>
#include <set>
#include <vector>
#include <memory>

class TableReader {
  public:
    // virtual destructor makes sure the right destructor is called on child classes
    virtual ~TableReader();

    // Call this function to attempt to imemdiately fetch table data that has been fetched before from the cache
    // Returns a NULL variant if the data is not in the cache, and then you'll need to use the async method
    static const Variant& getCachedTableData(std::string key);

    // Call this function to asynchronously request data from the remote table
    // recieveTableData will be called when the data is returned
    void requestTableData(std::string key);

    // Override this function to define what happens when table data requests come back
    virtual void receiveTableData(std::string key, const Variant& data) = 0;

    // Returns a list of all pending requested keys in no particular order
    // Used by the table socket client to batch and send requests to the server
    static std::vector<std::string> getRequestedKeys();

    // Called by the table socket client when a batch of data comes back from the server
    // Deserializes data to the cache and then
    // Distributes the data to all the objects that made requests
    static void receiveTableData(const std::map<std::string, Variant> &data);

    // resolves all pending requests whose data is already in the cache
    static void serveCachedRequests();

    //Clears all the items in the cache
    // You may need to call this before program exit to prevent a segfault on exit.
    static void clearCache();

  private:
    int pending_ = 0; // Number of pending requests this object is waiting on
    // Maps from string key to a list of Readers that requested that key
    static std::map<std::string, std::vector<TableReader*>> pending_requests_; // TODO investigate if this would be faster as an unordered map (it gets iterated)
    //Maps a key to cached data
    static std::map<std::string, Variant> cache_;
  // Maps a key item already fetched to its dependencies required before it can be served
    static std::map<std::string, std::set<std::string>> dep_on_;
    // Maps items not yet fetched to items that depend on them before being served
    static std::map<std::string, std::set<std::string>> dep_to_;
    // An item currently being initialized (keys requested during initialization of an item become dependencies)
    static std::string loading_key_ ; 
    // allows return by reference to return a valid Variant null value on failure
    static Variant null_variant;
    //Serves an item to its requesters, may also trigger the sending of items that were waiting on the given item
    static void serveItem(std::string key);
};

#endif // #ifndef _TABLEREADER_H_