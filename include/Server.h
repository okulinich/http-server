#pragma once

#include <netdb.h>
#include "ServerConfig.h"

class Server {
    private:
        int listenfd;
        struct addrinfo serveraddr;
        const ServerConfig config;
    public:
        bool createAndBind();
        bool run();
        Server(ServerConfig config);
};
