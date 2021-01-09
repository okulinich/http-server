#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "../include/parser.h"
#include "../include/sys_utils.h"

/**
 * @desc parses the first line of http request
 * and fills corresponding fields in passed object
 * of s_request struct
 * @params line - first line of the request, 
 * request - pointer to struct that will be filled
 * @return 0 in case of success, http error status 
 * code in case of failure
*/
static int parse_first_line(char *line, struct s_request *request, bool *host_required) {
    char *token = NULL;
    char major,
         minor;

    if(line == NULL || strlen(line) == 0)
        return ERROR_BAD_REQUEST;
    //starting whitespace -> return bad request
    if(isspace(line[0]))
        return ERROR_BAD_REQUEST;

    //method
    token = strtok(line, " ");
    if(!token)
        return ERROR_BAD_REQUEST;
    if(strlen(token) < 3 || strlen(token) > 7)
        return ERROR_BAD_REQUEST;
    if(strcmp(token, "GET") != 0) {
        if (strcmp(token, "POST") == 0 || strcmp(token, "HEAD") == 0 ||
            strcmp(token, "PUT") == 0 || strcmp(token, "DELETE") == 0 ||
            strcmp(token, "CONNECT") == 0 || strcmp(token, "TRACE") == 0 ||
            strcmp(token, "OPTIONS") == 0 || strcmp(token, "PATCH") == 0) {
                return ERROR_BAD_REQUEST;
        }
        else
            return ERROR_BAD_REQUEST;
    }
    request->method = strdup("GET");

    //uri: delims \r\n -> to read whole str if no uri
    token = strtok(NULL, " \r\n");
    if(!token)
        return ERROR_BAD_REQUEST;
    //if no uri specified
    if(strncmp(token, "HTTP/", 5) == 0) {
        request->uri = strdup("/");
    }
    else {
        if(strlen(token) > MAX_URI_LEN)
            return ERROR_URI_TOO_LONG;
        if(!checkForWildcards(token))
            return ERROR_NO_PERMISSION;
        request->uri = strdup(token);
        token = strtok(NULL, "\r\n");
    }

    //http-version
    if(!token)
        return ERROR_BAD_REQUEST;
    if(strncmp(token, "HTTP/", 5) != 0)
        return ERROR_BAD_REQUEST;
    major = *(strchr(token, '/') + 1);
    minor = *(strchr(token, '/') + 3);
    if(isdigit(minor) && isdigit(major)) {
        request->http_ver[0] = major - '0';
        request->http_ver[1] = minor - '0';
        // HTTP 1.1 -> Host header required
        if(request->http_ver[0] == 1 && request->http_ver[1] == 1)
            *host_required = true;
        // HTTP 2.0 -> Not supported
        else if(request->http_ver[0] == 2 && request->http_ver[1] == 0)
            return ERROR_NOT_IMPLEMENTED;
        // HTTP 1.0 or 0.9 -> OK, else -> Bad request
        else if((request->http_ver[0] != 0 && request->http_ver[1] == 9) ||
                (request->http_ver[0] != 1 && request->http_ver[1] != 0))
            return ERROR_BAD_REQUEST;
    }
    else
        return ERROR_BAD_REQUEST;

    return 0;
}

/**
 * @desc parses the http request
 * and fills passed object of s_request struct.
 * !Important: in case of success func allocates
 * dynamic memory for a list, method, uri, that 
 * should be freed by free_used_mem() function
 * @params req - http request that will be parsed,
 * request - pointer to struct that will be filled
 * @return 0 in case of success, http error status 
 * code in case of failure
*/
int parse_request(char *req, struct s_request *request) {
    char *token = NULL;
    //LEN+20 is enough to store the longest URI, method, http ver
    char first_line[MAX_URI_LEN + 20] = { 0 };
    char header[HEADER_LEN] = { 0 };
    char value[HEADER_LEN] = { 0 };
    bool host_required = false;
    char *indx = 0;
    int res = 0;

    indx = strchr(req, '\r');
    if(!indx)
        return ERROR_BAD_REQUEST;
    request->method = NULL;
    request->uri = NULL;
    request->head = NULL;

    //parse first line separately
    strncpy(first_line, req, indx - req);
    res = parse_first_line(first_line, request, &host_required);
    if(res != 0)
        return res;
    token = strtok(req, "\r\n");
    
    //save all headers to the linked list
    for (int i = 0; (token = strtok(NULL, "\r\n")) != NULL; i++) {
        indx = strchr(token, ':');
        if(!indx)
            continue;
        //copy & save every pair "header:value"
        strncpy(header, token, indx - token);
        strcpy(value, indx + 2);
        push_back(&request->head, header, value);
        //if http/1.1 used, host header MUST be in the 2th line
        if (host_required && i == 0 && 
            strcmp(header, "Host") != 0) {
            return ERROR_BAD_REQUEST;
        }
        else
            host_required = false;
        memset(header, '\0', HEADER_LEN);
        memset(value, '\0', HEADER_LEN);
    }
    // if not even entered the loop
    if(host_required)
        return ERROR_BAD_REQUEST;

    return 0;
}

/**
 * @desc frees memory allocated for field
 * of the passed struct s_request, if they
 * are not null
 * @params struct s_request item
 * @return void
*/
void free_used_mem(struct s_request *request) {
    if(request->method)
        free(request->method);
    if(request->uri)
        free(request->uri);
    if(request->head)
        delete_list(request->head);
}
