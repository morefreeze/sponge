#include "socket.hh"
#include "util.hh"

#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace std;

void get_URL(const string &host, const string &path) {
    // Your code here.

    // You will need to connect to the "http" service on
    // the computer whose name is in the "host" string,
    // then request the URL path given in the "path" string.

    // Then you'll need to print out everything the server sends back,
    // (not just one call to read() -- everything) until you reach
    // the "eof" (end of file).
    const Address http_server(host, "http");
    TCPSocket http_sock;
    http_sock.connect(http_server);
    string req_str;
    req_str += "GET " + path + " HTTP/1.1\r\n";
    req_str += "Host: " + host + "\r\n";
    req_str += "Connection: close\r\n";
    req_str += "\r\n";
    cerr << "request line: " << req_str << endl;
    http_sock.write(req_str, true);
    string buf;
    while(1) {
        buf = http_sock.read(BUFSIZ);
        if (buf.size() > 0) {
            cout << buf;
        } else {
            break;
        }
    };
    // cerr << "Function called: get_URL(" << host << ", " << path << ").\n";
    // cerr << "Warning: get_URL() has not been implemented yet.\n";
}

int main(int argc, char *argv[]) {
    try {
        if (argc <= 0) {
            abort();  // For sticklers: don't try to access argv[0] if argc <= 0.
        }

        // The program takes two command-line arguments: the hostname and "path" part of the URL.
        // Print the usage message unless there are these two arguments (plus the program name
        // itself, so arg count = 3 in total).
        if (argc != 3) {
            cerr << "Usage: " << argv[0] << " HOST PATH\n";
            cerr << "\tExample: " << argv[0] << " stanford.edu /class/cs144\n";
            return EXIT_FAILURE;
        }

        // Get the command-line arguments.
        const string host = argv[1];
        const string path = argv[2];

        // Call the student-written function.
        get_URL(host, path);
    } catch (const exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
