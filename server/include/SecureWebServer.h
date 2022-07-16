#ifndef _SECURE_WEBSERVER_H_
#define _SECURE_WEBSERVER_H_ 1

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <thread>
#include <memory>
#include <cstdint>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <httplib.h>

class SecureWebServer {
  private:
    // Thread that makes this server nonblocking
    std::thread thread;


  public:
    //Start the server on creation
    SecureWebServer(int port, const std::string base_dir, std::string cert_file, std::string private_key_file) ;

    static std::string dump_headers(const httplib::Headers &headers) ;

    static std::string dump_multipart_files(const httplib::MultipartFormDataMap &files);

    // Runs the server
    static int run(int port, const std::string base_dir, std::string cert_file, std::string private_key_file);

};

#endif // #ifndef _SECURE_WEBSERVER_H_