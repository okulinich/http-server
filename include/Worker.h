#pragma once

class ServerConfig;

/**
  * @desc this class used to precess connections
  * sockfd stores descriptor of socket that 
  * should be processed, newfd - would store
  * new socket descriptor of new connection
*/
class Worker {
    private:
        int newfd;
    public:
        Worker(int newfd);
        void processConnection(const ServerConfig config);
};
