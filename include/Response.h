#pragma once

#include <iostream>
#include <map>

extern "C" {
    #include "parser.h"
}

namespace Response {

    constexpr int maxChunkSize = 1024;

    struct respParams {
        bool keepAlive,       // enable (true) / disable (false) keep-alive attribute
             chunked;         // enable (true) / disable (false) chunked transfer encoding
        int timeout,          // default timeout for keep-alive
            maxReqLeft,       // max number of requests allowed in current connection
            totalLen,         // length of created response (filled automatically)
            respStatusCode;   // http status code of created response
        unsigned long filePos;// last position in the file where reading ended
        std::string fileName; // real file name extracted from uri (filled automatically)
    };

    const std::map<int, std::string> codeToStr = 
    {
    {400, "Bad request"},
    {403, "Forbidden"},
    {404,"Not found"},
    {414, "URI to long"},
    {500, "Internal server error"},
    {501, "Not implemented"} 
    };

    char *createResponse(respParams &param, struct s_request *req, 
                        std::string serverRoot, std::string defaultFile);
    char *createErrorResp(respParams &param, int code);
}
