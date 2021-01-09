#pragma once

#include <stdbool.h>

// in most HTTP servers if URI > 2000 
// -> 414 status code returned
#define MAX_URI_LEN 2000
// maximum length of header or value
#define HEADER_LEN 256

/**
 * @desc linked list, intended to store
 * http header and it's value
*/
typedef struct s_header_list {
    char header[HEADER_LEN];
    char value[HEADER_LEN];
    struct s_header_list *next;
} t_header_list;

/**
 * @desc struct that stores request
 * parsig results. t_header_list *head
 * linked list that stores headers and
 * their values.
*/
struct s_request {
    char *method;
    char *uri;
    t_header_list *head;
    int http_ver[2];
};

enum ResultCode { SUCCESS = 0, 
                  CHUNKED = 199, 
                  SINGLE_RESPONSE = 200,
                  ERROR_BAD_REQUEST = 400,
                  ERROR_NOT_FOUND = 404,
                  ERROR_NO_PERMISSION = 403,
                  ERROR_URI_TOO_LONG = 414,
                  ERROR_INTERNAL_SERVER = 500,
                  ERROR_NOT_IMPLEMENTED = 501 };

//linked list functions
void push_back(t_header_list **head, const char *h, const char *v);
const char *find_in_list(const char *key, const char *val, t_header_list *head);
void delete_list(t_header_list *head);
void print_list(t_header_list *head);
//main parse function
int parse_request(char *req, struct s_request *request);
void free_used_mem(struct s_request *request);
bool checkForWildcards(const char *uri);
