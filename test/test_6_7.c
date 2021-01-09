#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

/**
 * @desc create connection to specified host and port
 * @params host - string with host name, port - string
 * with port number
 * @return socket fd in case of success, -1 otherwise
*/
int establishNewConnection(const char *host, const char *port) {
    struct addrinfo clientConfig,
                    *clientInfo,
                    *ptr;
    int res = 0;
    int sockfd = 0;

    memset(&clientConfig, 0, sizeof(clientConfig));
 
    clientConfig.ai_family = AF_UNSPEC;
    clientConfig.ai_socktype = SOCK_STREAM;
 
    // get list of structs with params to create socket
    if ((res = getaddrinfo(host, port, &clientConfig, &clientInfo)) != 0) {
        return -1;
    }

    for(ptr = clientInfo; ptr != NULL; ptr = ptr->ai_next) {
        // create socket
        if ((sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1) {
            continue;
        }
        // connect to host
        if (connect(sockfd, ptr->ai_addr, ptr->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
    
        break;
    }
    if (ptr == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return -1;
    }

    freeaddrinfo(clientInfo);
    return sockfd;
}

/**
 * @desc sends some requests with timeout ti check if keep-alive
 * works properly
 * @params none
 * @return void
*/
int testKeepAlive() {
    const char *request = "GET /index.html HTTP/1.0\r\nConnection: keep-alive\r\nKeep-Alive: timeout=5, max=3\r\n\r\n";
    int res = 0;
    int sockfd = establishNewConnection("localhost", "3819");
    
    if(sockfd == -1)
        return -1;

    // send request with timeout 10 sec
    if(send(sockfd, request, strlen(request), 0) == -1) {
        close(sockfd);
        return -1;
    }

    // wait untill it is 1sec to the end of timeout
    sleep(4);
    // and send more one request
    if(send(sockfd, request, strlen(request), MSG_NOSIGNAL) == -1)
        // client closed the connection before timeout
        return 1;
    // wait untill it is 1sec to the end of timeout
    sleep(4);
    // send last request only to check if connections is OK
    if(send(sockfd, request, strlen(request), MSG_NOSIGNAL) == -1)
        // client closed the connection before timeout
        res = 1;
    else 
        res = 0;

    close(sockfd);
    return res;
}

/**
 * @desc thread function that sends simple requests and 
 * received response to check if server works good
 * @params none
 * @return (void *)0 in case of error, (void *)1 in case of success
*/
void *getHomePage() {
    const char *request = "GET /index.html HTTP/1.0\r\n\r\n";
    int sockfd = establishNewConnection("localhost", "3819");
    struct timeval tv = { 10, 0 };
    char buffer[1024] = { 0 };

    if(sockfd == -1)
        return (void *)0;


    if(send(sockfd, request, strlen(request), 0) == -1) {
        close(sockfd);
        return (void *)0;
    }

    // set timeout on socket - give server a chance: 10 sec
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    // if server hasn't sent anything and timeout gone
    // probably server is overloaded
    if(recv(sockfd, buffer, 1024, 0) <= 0) {
        // not receives anything
        close(sockfd);
        return (void *)0;
    }
    else {
        close(sockfd);
        return (void *)1;
    }
}


/**
 * 
 * 
 * 
*/
int testParallelConnections(int amount) {
    pthread_t pids[amount];
    void *threadReturn;
    int total = 0;
    int res = 0;

    // establish amount of connections to server
    for(int i = 0; i < amount; i++) {
        res = pthread_create(&pids[i], NULL, getHomePage, NULL);
        if(res != 0) {
            perror("Error creating thread");
            continue;
        }
    }
    // count amount of successfull connections
    for(int i = 0; i < amount; i++) {
        pthread_join(pids[i], &threadReturn);
        total += (int)threadReturn;
    }

    return total;
}



int main() {
    int connections = 250;
    int res = 0;
    
    printf("\n\e[4mTEST 6\e[24m\n(keep-alive: timeout)\n");
    printf("\e[5mAnalysis...\e[0m");
    res = testKeepAlive();
    printf("\e[K\rAnalysis finished\nResult: ");

    if(res == 0)
        printf("\e[32mKeep-Alive works perfectly\e[39m\n");
    else if(res == 1)
        printf("\e[31mServer closed the connection before timeout\e[39m\n");
    else
        printf("\e[35mAn error ocured during Keep-Alive test!\e[39m\n");


    printf("\n\n\e[4mTEST 7\e[24m\n(parallel connect-s)\n");
    printf("Trying to establish 250 parallel connections\e[5m...\e[0m");

    res = testParallelConnections(connections);
    
    res == connections ? printf("\e[32m!SUCCESS\e[39m") : printf("e[31m!FAIL\e[39m");
    printf("\nSuccessfully processed = \e[32m%d\e[39m connections from \e[94m%d\e[39m\n", res, connections);
    
    return 0;
}
