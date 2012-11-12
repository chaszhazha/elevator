/**
 * Author: Charles
 * Date: 10, Nov, 2012
 */

#ifndef _IP_LIST_H_
#define _IP_LIST_H_


/**
 * This list provides a minimal set of functionality as needed
 * by the elevator simulator.
 */
struct list_t;

typedef struct node_t {
    void *data;
    struct node_t *next;
} node_t;

typedef struct list_t {
        node_t *head;
        int size;
} list_t;


typedef struct
{
    int request_time;
    int off_time;
    int at;
    int to;  
    int num;
    int total_time;
}guest_t;


/**
 * Allocates memory for the list and does any necessary setup.
 */
void list_init(list_t **list);

/** 
 * Frees all memory explicitly allocated by the list and sets the 
 * pointer to null.
 */
void list_free_guests(list_t **list);

/**
 * Inserts a new node holding the data at the end of the list
 */
void list_append(list_t *list, void *data);

/** 
 * Returns 1 if the list is empty, 0 if not.
 */
int list_empty(list_t *list);

/**
 * Removes the node from the list that it belongs to
 */
void list_remove_guest(list_t*, node_t** node);

#endif