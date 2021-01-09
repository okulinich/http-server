#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>
#include <fstream>

#include "../include/Worker.h"
#include "../include/Response.h"
#include "../include/Log.h"
#include "../include/ServerConfig.h"
#include "../include/file_utils.h"

extern "C" {
    #include "../include/parser.h"
}

static int recvall(int sockfd, void *buf, int flags);
static void setSocketTimeout(int newfd, int timeSec);
static void enableKeepAlive(Response::respParams &param, struct s_request req, int &maxRequests);
static int sendAll(int newfd, const char *resp, int len);
static int sendRestChunks(int newfd, std::string fileName, std::string serverRoot, unsigned long &filePos);

using namespace Log;

/**
  * @desc receives data from client,
  * parses request and sends corresponding response
  * @params
  * @return true in case of success, false in case of
  * any errors
*/
void Worker::processConnection(const ServerConfig config) {
    constexpr int max_request_size = 2048;
    int maxRequests = config.maxReq;
    int rec_bytes = 0;
    char buffer[max_request_size] = { 0 };
    int reqInCurConnect = 0;
    char *fullResp = nullptr;
    Response::respParams param = { false, false, config.timeout, 
                                   maxRequests, 0, 0, 0, "-" };
    
    do {
        setSocketTimeout(newfd, param.timeout);
        rec_bytes = recvall(newfd, buffer, 0);
        // an error occured or connection closed
        if(rec_bytes < 0 && rec_bytes != -2) {
            if(rec_bytes == -3)
                writeLog(Level::DEBUG, "CLient closed connection!\n");
            else if(errno == EAGAIN || errno == EWOULDBLOCK)
                writeLog(Level::DEBUG, "!Timeout gone...");
            else
                writeLog(Level::WARNING, "Receive from client error: %s\n", strerror(errno));
            param.keepAlive = false;
            errno = 0;
        }
        //received some data from client
        else {
            ++reqInCurConnect; // keep track of max requests amount in current connection
            // requests left untill the connection will be closed
            param.maxReqLeft = maxRequests - reqInCurConnect;
            if(rec_bytes == -2)
                // buffer is ful, request too long
                fullResp = Response::createErrorResp(param, ERROR_BAD_REQUEST);
            else {
                //request parsing
                struct s_request req;
                int parseRes = parse_request(buffer, &req);

                if(parseRes != 0)
                    fullResp = Response::createErrorResp(param, parseRes);
                else { // check if keep-alive present among the headers
                    if(find_in_list("Connection", "keep-alive", req.head))
                        enableKeepAlive(param, req, maxRequests);
                    else // if keep-alive is absent -> server closes connection
                        param.keepAlive = false;
                    fullResp = Response::createResponse(param, &req, config.rootFolder.c_str(), 
                                                        config.defaultFile.c_str());
                }
                free_used_mem(&req);
            } // parsing ends
            // send created response to client
            sendAll(newfd, fullResp, param.totalLen);
            delete[] fullResp;
            // if chunked transfer encodding enabled
            if(param.chunked) { // simply send rest chunks
                param.totalLen = sendRestChunks(newfd, param.fileName, 
                                    config.rootFolder, param.filePos);
                param.chunked = false;
            }
            Log::writeLogW3c("GET", param.fileName.c_str(), param.respStatusCode, param.totalLen);
        } // processing received data ends
    } while(param.keepAlive && reqInCurConnect < maxRequests);
    // server doesn't close socket untill keep-alive header present 
    // and amount of requests in current connection is less than max
    writeLog(Level::DEBUG, "<Server closed the connection>\n");
    close(newfd);
}

Worker::Worker(int newfd) {
    this->newfd = newfd;
}

/**
 * @desc sets timeout on socket for keep-alive
 * @param newfd - file descriptor of socket, 
 * timesec - timeout in seconds
 * @return void
*/
static void setSocketTimeout(int newfd, int timeSec) {
    struct timeval tv = { timeSec, 0 };

    if(setsockopt(newfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1)
        writeLog(Level::ERROR, "Setsockopt (timeout): %s\n", strerror(errno));
}

/**
 * @desc sets socket option that turns on timeout after 
 * connection became idle, changes states of passed fields
 * to keep current connection alive
 * @params flag - bool value that defines whether keep-alive
 * option is enabled for current connection or not, newfd -
 * file descriptor of socket in current connection, param -
 * struct from Response namespace that stores timeout values,
 * that will be send to the client
 * @return void
*/
static void enableKeepAlive(Response::respParams &param, struct s_request req, int &maxRequests) {
    const char *keepAliveValues = nullptr;

    // if Keep-Alive header found -> set it's values for timeout
    // and max requests as default for current connection
    keepAliveValues = find_in_list("Keep-Alive", NULL, req.head);
    if(keepAliveValues != nullptr) {
        const char *timeoutVal = strstr(keepAliveValues, "timeout=");
        if(timeoutVal)
            param.timeout = atoi(timeoutVal + strlen("timeout="));
        const char *reqAmount = strstr(keepAliveValues, "max=");
        if(reqAmount)
            param.maxReqLeft = atoi(reqAmount + 4);
    }

    // raise all the flags & save timeout
    param.keepAlive = true;
    maxRequests = param.maxReqLeft;
}

/**
 * @desc reads msg from a socket as follows: 
 * receives ALL data from client by calling
 * recv() function in loop
 * @params sockfd - socket to read from, 
 * buf - large buffer, where all the data
 * would be placed, len - size of this buffer,
 * flags - additional options for recv() func.
 * @return number of bytes in last portion in
 * case of success, -1 - recv() error, -2 -
 * buffer overflow, -3 - connection closed.
*/
static int recvall(int sockfd, void *buf, int flags) {
    int nbytes = 0,
        pos = 0;
    constexpr int chunk_request_size = 256;
    constexpr int max_request_size = 2048;
    char littleBuf[chunk_request_size] = { 0 };

    while((nbytes = recv(sockfd, littleBuf, chunk_request_size - 1, flags)) > 0) {
        //copy received bytes into large buffer
        memcpy(&((char *)buf)[pos], littleBuf, nbytes);
        //check if reached the end of request
        if(strstr(littleBuf, "\r\n\r\n"))
            break;
        memset(littleBuf, 0, chunk_request_size);
        //monitor buffer filling
        pos += nbytes;
        //buffer is full
        if(pos >= max_request_size)
            return -2;
    }
    //connection was closed
    if(nbytes == 0 && strlen(littleBuf) == 0)
        return -3;

    return nbytes;
}

/**
 * @desc reads parts of the requested resourse and 
 * performs multiple send operations while the whole 
 * file wouldn't be sent
 * @ newfd - file descriptor of socket, fileName - string
 * that stores name of the resourse extracted from the uri,
 * serverRoot - string that stores server root folder
 * @return void
*/
static int sendRestChunks(int newfd, std::string fileName, std::string serverRoot, unsigned long &filePos) {
    int statusCode = 200;
    size_t bodyLen = 0;
    int totalLen = 0;
    char *body = nullptr;
    char *fullResp = nullptr;
    int res = 0;

    while (statusCode != 0 && res == 0) {
        // read next chunk from the file
        statusCode = getFileContent((serverRoot + fileName), &body, &bodyLen, filePos);
        // add closign "\r\n" to the end of chunk
        fullResp = new char[bodyLen + 2];
        memcpy(fullResp, body, bodyLen);
        memcpy(&fullResp[bodyLen], "\r\n", 2);
        // simply send body without any headers
        res = sendAll(newfd, fullResp, bodyLen + 2);
        // get ready to read next part
        totalLen += bodyLen + 2; // collecting logging data
        delete[] body;
        delete[] fullResp;
        body = nullptr;
        fullResp = nullptr;
    }
    // whole file was send -> send zero-chunk, the last one
    sendAll(newfd, "0\r\n\r\n", 5);
    return totalLen;
}

/**
 * @desc It's only a wrapper over the library send() function
 * its main goal - handle broken pipe signal
 * @params newfd - socket file descriptor, resp - c-string
 * with msg that would be sent, len - int that stores exact
 * length of the message
 * @return void
*/
int sendAll(int newfd, const char *resp, int len) {
    int res = 0;

    res = send(newfd, resp, len, MSG_NOSIGNAL);
    if(res < 0) {
        Log::writeLog(Log::Level::ERROR, "Sending to client error: %s\n", strerror(errno));
        if(errno == 32) // Broken pipe
            return -1;
    }
    return 0;
}
