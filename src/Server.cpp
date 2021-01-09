#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <iostream>
#include <thread>

#include "../include/Server.h"
#include "../include/Worker.h"
#include "../include/ServerConfig.h"
#include "../include/Log.h"

extern "C" {
    #include "../include/sys_utils.h"
}

static void printClientInfo(struct sockaddr_storage& client_addr, socklen_t addrlen);
static bool setSocketOptions(int listenfd, int ipv, std::string interface);

using namespace Log;

Server::Server(ServerConfig conf) : config(conf) {
    listenfd = 0;

    memset(&serveraddr, 0, sizeof(serveraddr));

    serveraddr.ai_flags = AI_PASSIVE;
    serveraddr.ai_family = AF_UNSPEC;
    serveraddr.ai_socktype = SOCK_STREAM; 
}

/**
  * @desc creates listening socket and 
  * binds it to a port or interface
  * depending on configuration
  * @params config - ServerConfig 
  * class object
  * @return true in case of success 
*/
bool Server::createAndBind() {
    struct addrinfo* serv_info = nullptr; // list with resulted info for creating a socket
    struct addrinfo* ptr = nullptr;       // pointer for iterating in linked list

    if(config.ipv == 4)
        serveraddr.ai_family = AF_INET;
    else if(config.ipv == 6)
        serveraddr.ai_family = AF_INET6;

    if(int r = (getaddrinfo(nullptr, config.port.c_str(), &serveraddr, &serv_info) != 0)) {
        writeLog(Level::ERROR, "getaddrinfo error: %s", gai_strerror(r));
        return false;
    }
    //going through the list of possible addresses
    for(ptr = serv_info; ptr != nullptr; ptr = ptr->ai_next) {
        listenfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if(listenfd== -1) {
            writeLog(Level::ERROR, "Error creating socket: %s\n", strerror(errno));
            continue;
        }
        //setting up some socket options (reuse socket, only IPv6 addr, bind to interface)
        if(!setSocketOptions(listenfd, config.ipv, config.interface.c_str()))
            continue;
        //binding socket with real port or interface
        if(bind(listenfd, ptr->ai_addr, ptr->ai_addrlen) == -1) {
            writeLog(Level::ERROR, "binding error: %s\n", strerror(errno));
            continue;
        }
        //if we got here -> socket created successfully
        break;
    }
    //if we reached the end of the list -> some errors occered
    if(ptr == nullptr) {
        writeLog(Level::WARNING, "Failed to create and bind socket!\n");
        return false;
    }
    //avioding memmory leak
    freeaddrinfo(serv_info);

    return true;
}

/**
  * @desc starts server main infinite
  * loop, listens socket, accepts
  * new connections and process them
  * one by one in the same thread and proc
  * @params config - ServerConfig 
  * class object
  * @return never returns
*/
bool Server::run() {
    struct sockaddr_storage client_addr;
    socklen_t addrlen = sizeof(client_addr);

    if(listen(listenfd, config.backlog) == -1) {
        writeLog(Level::ERROR, "Listen error: %s\n", strerror(errno));
        return false;
    }
    startLoggingW3c();

    auto newConnection = [](int newfd, ServerConfig config) {
        Worker worker(newfd);
        worker.processConnection(config);
    };

    //MAIN SERVER LOOP
    while(true) {
        if(sigintSent(false)) {
            writeLog(Level::DEBUG, "\n\t\t* SERVER INTERRUPTED BY SIGINT *");
            break;
        }

        int newfd = accept(listenfd, (sockaddr *)&client_addr, &addrlen);
        
        if(newfd == -1) {
            writeLog(Level::ERROR, "accept: %s\n", strerror(errno));
            continue;
        }
        printClientInfo(client_addr, addrlen);
        
        std::thread newThread(newConnection, newfd, config);
        newThread.detach();
    }

    closeLogFile();
    return true;
}

/**
  * @desc getnameinfo() converts address located in 
  * sockaddr struct to text format (NULL-terminated string)
  * BUT service address is in weird numeric 
  * format, so inet_ntop() function should be used to convert
  * address of service from binary to nice text  form.
  * !Both getnameinfo() and inet_ntop() functions
  * are IP-version independent.
  * @params client_addr - struct of type sockaddr,
  * filled fith client info by accept() call,
  * addrlen - size of this struct
*/
static void printClientInfo(struct sockaddr_storage& client_addr, socklen_t addrlen) {
    char host[INET6_ADDRSTRLEN] = { 0 };
    char service[INET6_ADDRSTRLEN] = { 0 };
    char service_in_text[INET6_ADDRSTRLEN] = { 0 };

    if (getnameinfo((sockaddr *)&client_addr, addrlen, host,
        sizeof(host), service, sizeof(service), 0) == 0) {
        //convert client's addr to tex form and print it
        inet_ntop(client_addr.ss_family, service, service_in_text, addrlen);
        writeLog(Level::DEBUG, "!>\nGot new connection!\nfrom host : service = %s : %s\n!>", 
                                                        host, service_in_text);
        // save client IP
        setClientIp(service_in_text);
    }
    else
        writeLog(Level::ERROR, "getnameinfo: %s\n", strerror(errno));
}

/**
 * @desc sets up some socket options depending on
 * current configuration
 * @params listenfd - listening socket, ipv - integer
 * number that means ip version, if 0 - both ipv4 and ipv6
 * used, if 6 - only ipv6, if 4 - only ipv4. intName -
 * const c-string that stores interface the name
 * @return true in case of successfull binding a socket,
 * false otherwise 
*/
static bool setSocketOptions(int listenfd, int ipv, std::string intName) {
    int optval = 1;

    //setting up option to bind socket to the interface
    if(intName.compare("0") != 0 && intName.compare("none") != 0) {
        if(setsockopt(listenfd, SOL_SOCKET, SO_BINDTODEVICE, intName.c_str(), strlen(intName.c_str())) == -1) {
            writeLog(Level::ERROR, "Setsockopt (interface): %s\n", strerror(errno));
            return false;
        }
    }
    //setting up option to reuse the socket even if some errors ocurred
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        writeLog(Level::ERROR, "Setsockopt (reuse): %s\n", strerror(errno));
        return false;
    }
    //setting up option to accept only ipv6 connections
    if(ipv == 6) {
        if(setsockopt(listenfd, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(optval)) < 0) {
            writeLog(Level::ERROR, "Setsockopt (ipv6): %s\n", strerror(errno));
            return false;
        }
    }

    return true;
}
