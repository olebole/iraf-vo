/**
 * VOCLIST.H - Simple C Linked List Class.
 *
 * @file	vocList.h
 * @author	Doug Tody
 * @version	January 2014
 */

#ifndef VOCLIST_H
#define VOCLIST_H

#include <stdlib.h>
#include <string.h>

#define	DEF_LISTSIZE	1024

/*
 * Data Structures.
 */

/* Linked List node. */
typedef struct vocList_node_t {
    struct vocList_node_t *prev;  /* previous node */
    struct vocList_node_t *next;  /* next node */
    void *value;		  /* value for this node */
    long *index;		  /* index value for this node. */
} vocList_node_t;

/* Main descriptor for the List structure. */
typedef struct vocList_t {
    long size;			  /* index array size */
    long count;			  /* number of items in this list */
    long pos;			  /* current position in list */
    vocList_node_t **index;	  /* array index of list nodes */
    struct vocList_node_t *first; /* first node */
    struct vocList_node_t *last;  /* last node */
} vocList_t;

/* Iterator type for iterating through the list. */
typedef struct vocList_iter_t {
    struct vocList_node_t *current; /* current node */
} vocList_iter_t;


/*
 * Function signatures.
 */

/* vll_init -- Create a new, empty linked list. */
vocList_t *vll_init (long req_size);

/* vll_count -- Return a count of the current number of list elements. */
long vll_count (vocList_t *list);

/* vll_seek -- Seek to the indicated position in the list. */
void *vll_seek (vocList_t *list, long offset, int whence);

/* vll_find -- Seek to the position of the referenced object. */
void *vll_find (vocList_t *list, void *obj, long *index);

/* vll_prev -- Seek to the previous position in the list. */
void *vll_prev (vocList_t *list);

/* vll_next -- Seek to the next position in the list. */
void *vll_next (vocList_t *list);

/* vll_insert -- Insert a new object in the list. */
void *vll_insert (vocList_t *list, void *obj, long *index);

/* vll_append -- Append an object to the list. */
void *vll_append (vocList_t *list, void *obj, long *index);

/* vll_remove -- Remove the node at the given position from the list. */
void *vll_remove (vocList_t *list, long pos);

/* vll_destroy -- Destroy a List object and free all resources. */
void vll_destroy (vocList_t *list, void(*func)(void*ptr));

/* vll_rebuild_index -- Internal routine to reallocate and rebuild the array
 * index.
 */
int vll_rebuild_index (vocList_t *list, int req_size);

#endif
