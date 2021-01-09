#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../include/parser.h"

/**
 * @desc allocates memory and initializes
 * linked list node
 * @params const char h, v - headers and
 * corresponding value
 * @return dynamically allocated linked list 
 * node
*/
static t_header_list *create_node(const char *h, const char *v) {
    t_header_list *node = (t_header_list *)malloc(sizeof(t_header_list));

    strncpy(node->header, h, HEADER_LEN - 1);
    strncpy(node->value, v, HEADER_LEN - 1);
    node->next = NULL;
    return node;
}

/**
 * @desc adds new node at the end of the 
 * linked list
 * @params **head - pointer to pointer to 
 * head of the list, const char h, v - 
 * headers and corresponding value
 * @return void
*/
void push_back(t_header_list **head, const char *h, const char *v) {
    if(!(*head)) {
        *head = create_node(h, v);
    }
    else {
        t_header_list *tmp = *head;
        while (tmp->next)
            tmp = tmp->next;
        tmp->next = create_node(h, v);
    }
}

/**
 * @desc frees all used memory by the
 * linked list
 * @params *head - pointer to the head
 * of the list
 * @return void
*/
void delete_list(t_header_list *head) {
    t_header_list *tmp;

    while (head) {
        tmp = head;
        head = head->next;
        free(tmp);
        if(!head)
            break;
    }
}

/**
 * @desc prints info stored in list
 * to the stdout in format "header: value"
 * @params *head - pointer to the head 
 * of the linked list
 * @return void
*/
void print_list(t_header_list *head) {
    while (head) {
        printf("%s: %s\n", head->header, head->value);
        head = head->next;
    }
}

/**
 * @desc searches for list item with header 
 * equal to key and value equel to val in
 * list specified by head, if val is not 
 * specified (==NULL), only header if checksed 
 * for equality
 * @params *head - pointer to the head 
 * of the linked list, key - const c-string
 * that means header name, val - const c-string
 * that means header value
 * @return pointer to value in case of success 
 * and NULL otherwise
*/
const char *find_in_list(const char *key, const char *val, t_header_list *head) {
    while (head) {
        if (strcmp(head->header, key) == 0) {
            if(val == NULL || strcmp(head->value, val) == 0)
                return head->value;
        }
        head = head->next;
    }
    return NULL;
}
