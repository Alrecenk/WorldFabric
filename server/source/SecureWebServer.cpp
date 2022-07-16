#include "SecureWebServer.h"

using namespace httplib;
using namespace std;

// command for generating key files in linux
// openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 365
SecureWebServer::SecureWebServer(int port, const string base_dir, string cert_file, string private_key_file) {
    thread = std::thread(SecureWebServer::run, port, base_dir, cert_file, private_key_file);
   //int ret = SecureWebServer::run(port, base_dir, cert_file, private_key_file);
}

string SecureWebServer::dump_headers(const Headers &headers) {
  string s;
  char buf[BUFSIZ];

  for (const auto &x : headers) {
    snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
    s += buf;
  }

  return s;
}

string SecureWebServer::dump_multipart_files(const MultipartFormDataMap &files) {
  string s;
  char buf[BUFSIZ];

  s += "--------------------------------\n";

  for (const auto &x : files) {
    const auto &name = x.first;
    const auto &file = x.second;

    snprintf(buf, sizeof(buf), "name: %s\n", name.c_str());
    s += buf;

    snprintf(buf, sizeof(buf), "filename: %s\n", file.filename.c_str());
    s += buf;

    snprintf(buf, sizeof(buf), "content type: %s\n", file.content_type.c_str());
    s += buf;

    snprintf(buf, sizeof(buf), "text length: %zu\n", file.content.size());
    s += buf;

    s += "----------------\n";
  }

  return s;
}


int SecureWebServer::run(int port, const string base_dir, string cert_file, string private_key_file) {

    SSLServer svr(cert_file.c_str(), private_key_file.c_str());

    svr.Post("/multipart", [](const Request &req, Response &res) {
        auto body = dump_headers(req.headers) + dump_multipart_files(req.files);
        res.set_content(body, "text/plain");
    });

    svr.set_error_handler([](const Request & /*req*/, Response &res) {
        const char *fmt = "<p>Error Status: <span style='color:red;'>%d</span></p>";
        char buf[BUFSIZ];
        snprintf(buf, sizeof(buf), fmt, res.status);
        res.set_content(buf, "text/html");
    });
    if (!svr.set_mount_point("/", base_dir)) {
        cout << "The specified base directory doesn't exist...";
        return 1;
    }

    //svr.listen("localhost", port);
    svr.listen("0.0.0.0",port);
    return 0;
}