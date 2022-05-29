#include "WebServer.h"

char* WebServer::root_ = nullptr;

WebServer::WebServer(
        const char http_address[], const int http_port, char http_root[]) {
    // Save the root folder to the object for use in responses
    WebServer::root_ = http_root;
    // Start the libevent handler on a held thread
    thread_ = std::thread(WebServer::run, http_address, http_port);

}

void WebServer::run(const char http_address[], const int http_port) {
    if (!event_init()) {
        std::cerr << "Failed to init libevent for WebServer." << std::endl;
    } else {
        std::unique_ptr<evhttp, decltype(&evhttp_free)> server(
                evhttp_start(http_address, (std::uint16_t) http_port),
                &evhttp_free);
        if (!server) {
            std::cerr << "Failed to init http server." << std::endl;
        } else {
            // Link the server to the file serving function
            evhttp_set_gencb(server.get(), WebServer::serveFile, nullptr);
            event_dispatch();
        }
    }

}

void WebServer::stop() {
    event_loopbreak();
}

//TODO there seems to be a bug around the executable being in a folder but the path going from where it's ran and not where it is (sometimes?).
// Acts as a static file server
void WebServer::serveFile(evhttp_request* req, void*) {
    const char* uri = evhttp_request_get_uri(req);
    struct evhttp_uri* decoded = NULL;
    const char* path;
    char* decoded_path;
    char local_path[255];

    if (req == NULL) return; // req is null after a timeout

    // Decode the URL into a file path
    decoded = evhttp_uri_parse(uri);
    if (!decoded) {
        printf("It's not a good URI. Sending BADREQUEST\n");
        evhttp_send_error(req, HTTP_BADREQUEST, 0);
        return;
    }
    path = evhttp_uri_get_path(decoded);
    if (!path) path = "/";
    decoded_path = evhttp_uridecode(path, 0, NULL);

    // Append local folder we're serving files from
    sprintf(local_path, "%s%s", WebServer::root_, decoded_path);

    // Prepare an output buffer to put the response in
    auto* out_buffer = evhttp_request_get_output_buffer(req);
    if (!out_buffer)
        return;

    int fd = open(local_path, O_RDONLY); // Open the file
    struct stat stat_buffer{};
    int status = fstat(fd, &stat_buffer);
    if (status == -1) {
        printf("File not found %s. Sending BADREQUEST\n", local_path);
        evhttp_send_error(req, HTTP_BADREQUEST, 0);
        return;
    }
    // Send the file
    int size = stat_buffer.st_size;
    //printf("serving %s (%i)\n", local_path, size);
    evbuffer_add_file(out_buffer, fd, 0, size);
    evhttp_send_reply(req, HTTP_OK, "", out_buffer);
    delete path;
    delete decoded_path;
}


  
