#include <fstream>
#include <sstream>
#include <errno.h>
#include <sys/socket.h>

#include <cstring>
#include <string>

#include "../include/Response.h"
#include "../include/Log.h"
#include "../include/file_utils.h"
#include "../include/cookiesResponse.h"

extern "C" {
    #include "../include/parser.h"
}

static std::string createHeaders(Response::respParams &param, int contentLen, 
                                t_header_list *listWithCookieHeader = nullptr);

/**
 * @desc creates corresponding response to
 * the request and sends it to the client
 * @params serverRoot - c-string, that stores
 * current server root folder for resourses,
 * defaultFile - c-string that stores name of
 * the default file that would be returned when
 * root uri ('/') requested, req - struct filled
 * by request parser, param - struct with 
 * additional server params (Log object, timeout, etc.)
 * @return amount of bytes sent to the client
*/
char *Response::createResponse(Response::respParams &param, struct s_request *req, 
                                std::string serverRoot, std::string defaultFile) {
    std::string fileName;
    std::stringstream res;
    size_t bodyLen = 0;
    char *body = nullptr;
    int statusCode = 0;

    // if uri is root folder without or with parameters ->
    // generate server starting page html
    if(strcmp(req->uri, "/") == 0 || strchr(req->uri, '?')) {
        statusCode = createRootResponseBody(bodyLen, &body, req, serverRoot);
        if(statusCode != 0)
            return createErrorResp(param, ERROR_INTERNAL_SERVER);
    }
    // otherwise, return requested resource to the user
    else {
        // get resource name from uri
        if((statusCode = filenameFromUri(req->uri, param.fileName, defaultFile)) != ResultCode::SUCCESS)
            return createErrorResp(param, statusCode);
        // get resource content
        if((statusCode = getFileContent((serverRoot + param.fileName), 
                                        &body, &bodyLen, param.filePos)) == ResultCode::CHUNKED)
            param.chunked = true;
        else if(statusCode != ResultCode::SINGLE_RESPONSE)
            return createErrorResp(param, statusCode);
    }

    //adding first line and headers
    res << "HTTP/" << req->http_ver[0] << "." << req->http_ver[1]
        << " 200 OK" << "\r\n" << createHeaders(param, bodyLen, req->head);

    // res.str() - headers, len - body
    param.totalLen = res.str().length() + bodyLen;
    param.respStatusCode = ResultCode::SINGLE_RESPONSE;
    if(param.chunked) // add 2 bytes for closing '\r\n'
        param.totalLen += 2;
    
    char *fullResp = new char[param.totalLen];
    // copy first line & headers
    memcpy(fullResp, res.str().c_str(), res.str().length());
    // copy file content [file opened as binary]
    memcpy(&fullResp[res.str().length()], body, bodyLen);
    if(param.chunked) // copy closing CRLF to response
        memcpy(&fullResp[param.totalLen - 2], "\r\n", 2);

    delete[] body;
    return fullResp;
}

/**
 * @desc constructs nice html-error page
 * that corresponds to status code and
 * sends it to the client
 * @params code - http status code, param
 * - struct with additional server params 
 * (Log object, timeout, etc.)
 * @return amount of bytes sent to the client
*/
char *Response::createErrorResp(Response::respParams &param, int code) {
    std::stringstream htmlBody,
                      res;
    std::string startString = "HTTP/1.1 ";
    const char *explanation = Response::codeToStr.at(code).c_str();

    startString.append(std::to_string(code)).append(" ");
    startString.append(explanation).append("\r\n");

    htmlBody << "<html>\n<head><title>" << code << " " << explanation
             << "</title></head>\n<body bgcolor=\"white\">\n<center><h1>"
             << "Sorry, an error occured: " << code << " " << explanation
             << "</h1></center>\n<hr><center>server alpha</center>\n</body>\n</html>";
    
    res << startString << createHeaders(param, htmlBody.str().length())
        << htmlBody.str();

    param.respStatusCode = code;
    param.totalLen = res.str().length() + 1;
    char *fullResp = new char[param.totalLen];
    strcpy(fullResp, res.str().c_str());

    return fullResp;
}

/**
 * @desc creates headers for response
 * @params param - struct with 
 * additional server params (Log object, 
 * timeout, etc.), contentLen - corresponding
 * value to header Content-Length, 
 * listWithCookieHeader - t_header_list where
 * "Set-Cookie" header may be placed (default = null)
 * @return string with headers ending with two CRLF
*/
static std::string createHeaders(Response::respParams &param, int contentLen, t_header_list *listWithCookieHeader) {
    std::stringstream headers;

    if(param.keepAlive)
        headers << "Connection: " << "keep-alive\r\n"
                << "Keep-Alive: max=" << param.maxReqLeft
                << ", timeout=" << param.timeout << "\r\n";
    if(listWithCookieHeader) {
        // extract cookie value from list and add to the response Set-cookie header
        const char *cookieValue = find_in_list("Set-Cookie", nullptr, listWithCookieHeader);
        if(cookieValue)
            headers << "Set-Cookie: " << cookieValue << "\r\n";
    }
    if(param.chunked)
        headers << "Transfer-Encoding: chunked" << "\r\n\r\n";
    else
        headers << "Content-Length: " << contentLen << "\r\n\r\n";
    return headers.str();
}
