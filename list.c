/**
 * Author: Charles
 * Date: 10, Nov, 2012
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "list.h"

void list_init(list_t **list)
{
	*list = (list_t *)malloc(sizeof(list_t));
	memset(*list, 0, sizeof(list_t));
}

void list_free_guests(list_t **list)
{
	if ((*list) != NULL) { 
		node_t *curr, *next;
		for (curr = (*list)->head; curr != NULL; curr = next) {
			next = curr->next;
                        free((guest_t*)curr->data);
			free(curr);
		}
		free(*list);
		*list = NULL;
	}
}

void list_append(list_t *list, void *data)
{
	node_t *new_node = (node_t *)malloc(sizeof(node_t));
	memset(new_node, 0, sizeof(node_t));
        new_node->list = list;
	new_node->data = data;
	
	if (list_empty(list)) {
		list->head = new_node;
	} else {
		node_t *curr;
		for (curr = list->head; curr->next != NULL; curr = curr->next);
		curr->next = new_node;
	}
        list->size++;
}

int list_empty (list_t *list)
{
	return list->head == NULL;
}

void list_remove_guest(list_t* list, node_t** node)
{
    //TODO: rewrite this function
    
    (*node)->list->size--;
    node_t * next_node = (*node)->next;
    if((*node)->next != NULL)
    {
        printf("1\n");
        memcpy((*node)->data, (*node)->next->data, sizeof(guest_t));
        (*node)->next = (*node)->next->next;
        free((guest_t*)next_node->data);
        free(next_node);
        assert((*node)->list->head != NULL);
    }
    else
    {
        printf("2\n");
        free((guest_t*)(*node)->data);
        free(*node);
        *node = NULL;
    }
    
}
