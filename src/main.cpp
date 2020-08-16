#include "HttpServer.h"


int main(int argc, char** argv) {

    int port = 8080;
    if (argc >= 2) {
        port = atoi(argv[1]);
    }
    int numThread = 4;

    if (argc >= 3) {
        numThread = atoi(argv[2]);
    }

   // int numThread = 4;
    HttpServer server(port, numThread);
    server.run();

    return 0;
}
