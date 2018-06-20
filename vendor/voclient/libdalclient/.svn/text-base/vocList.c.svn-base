/**
 * VOCLIST.C - Simple C Array-List Class.
 *
 * Lists are doubly-linked, with the value being a pointer to an arbitrary
 * external object.  The life cycle of the external object is not managed
 * by the list.  An unusual feature is that an array index is supported to
 * permit randomly accessing the list as an array.
 *
 *   list = vll_init (size)			# create with initial size
 *   len = vll_count (list)			# get current list length
 *    obj = vll_seek (list, offset, whence)	# seek to offset from whence
 *    obj = vll_find (list, obj, index)		# find next node with object
 *    obj = vll_prev (list)			# back up one node
 *    obj = vll_next (list)			# advance to next node
 *  obj = vll_insert (list, obj, index)		# insert at current pos
 *  obj = vll_append (list, obj, index)		# append to the list
 *  obj = vll_remove (list, pos)		# remove node from list
 *       vll_destroy (list, func)		# free all list resources
 *
 * The list may be traversed in either direction or may be randomly
 * accessed like an array.  The access methods (e.g., seek, prev, next)
 * return a pointer to the object associated with the current list node.
 *
 * Note: This code is optimized for fairly static lists to which items are
 * mainly appended.  The need to maintain an index into the list requires
 * that the index be rebuilt if elements are inserted or removed anywhere
 * other than at the end of the list.  Note that rebuilding the index can
 * cause index values for nodes to change.  Appending to the list has no
 * performance penalty.  These considerations affect only efficiency;
 * inserting and removing elements at random is otherwise not affected.
 *
 * @file	vocList.c
 * @author	Doug Tody
 * @version	January 2014
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "vocList.h"


/*
 * vll_init -- Create a new, empty linked list.
 *
 * @brief   Create a new linked ist
 * @fn	    list = vll_init (long size)
 *
 * @param   size	Initial max size of list, or 0 for default
 * @returns		Reference to List object
 *
 * A new, empty list is created.  Initial storage for the index array
 * used to randomly access the list may optionally be specified as SIZE.
 * If SIZE=0, a default maximum size will be used.  The maximum size of
 * the list is a soft maximum, i.e., the list will be grown automatically
 * as needed.
 */
vocList_t *
vll_init (long req_size)
{
    vocList_t *list;

    /* List descriptor */
    list = (vocList_t *) malloc(sizeof(vocList_t));
    if (list == NULL)
	return (NULL);

    /* Initial index array storage. */
    long size = (req_size <= 0) ? DEF_LISTSIZE : req_size;
    list->size = size;
    list->index =
	(vocList_node_t **) calloc((size_t)size, sizeof(vocList_node_t *));
    if (list->index == NULL)
	return (NULL);

    /* Init node list to empty */
    list->count = 0;
    list->pos = 0;
    list->first = list->last = NULL;

    return (list);
}


/*
 * vll_count -- Return a count of the current number of list elements.
 *
 * @brief   Get the current size of the list
 * @fn	    long = vll_count (vocList_t *list)
 *
 * @param   list	Linked list to be accessed
 * @returns		Number of items currently in the List
 */
long
vll_count (vocList_t *list)
{
    return (list->count);
}


/*
 * vll_seek -- Seek to the indicated position in the list.
 *
 * @brief   Seek to the indicated position in the list
 * @fn	    obj = vll_seek (vocList_t *list, long offset, int whence)
 *
 * @param   list	Linked list to be accessed
 * @param   offset	Offset from the given position
 * @param   whence	Reference position for seek
 * @returns		Pointer to the object stored at that node
 *
 * The current position in the list is set to the node OFFSET positions
 * from the reference point indicated by WHENCE: SEEK_SET (beginning of
 * list), SEEK_CUR (current position), SEEK_END (end of list).  The values
 * of WHENCE are as defined in <stdio.h>.  A seek to (0,SEEK_SET) positions
 * to the beginning of the list, whereas (0,SEEK_END) positions ot the end
 * of the list.
 *
 * If the seek position is valid a pointer to the object at that location
 * is returned, otherwise NULL.  A seek on an empty list always returns
 * NULL since there is no object pointer to return.  The current list
 * position is unaffected if NULL is returned.
 */
void *
vll_seek (vocList_t *list, long offset, int whence)
{
    long pos;

    /* Get an absolute position. */
    switch (whence) {
	case SEEK_SET:
	    pos = offset;
	    break;
	case SEEK_CUR:
	    pos = list->pos + offset;
	    break;
	case SEEK_END:
	    pos = (list->count - 1) + offset;
	    break;
	default:
	    return (NULL);
    }

    /* Check for a value outside the range of the list.  Valid values of POS
     * are between -1 and EOL (count-1).
     */
    if (pos < 0) {
	list->pos = -1;
	return (NULL);
    } else if (pos > (list->count - 1)) {
	list->pos = list->count - 1;
	return (NULL);
    }

    vocList_node_t *node = list->index[pos];
    if (node == NULL)
	return (NULL);
    else
	list->pos = pos;

    return (node->value);
}


/*
 * vll_find -- Find the next node containing the given Obj value.
 *
 * @brief   Find the next node containing the given Obj value
 * @fn	    obj = vll_find (vocList_t *list, (void *)obj)
 *
 * @param   list	Linked list to be accessed
 * @param   obj		Object value to search for
 * @param   index	Pointer to location storing index value
 * @returns		Pointer to the object stored at that node
 *
 * If index values for nodes are being cached with the external object
 * then the list index value for an object can be determined directly.
 * Otherwise the list is searched *from the current position* for a node
 * with the given object value (pointer).  The list is left positioned
 * at that node.  NULL is returned if the object is not found.
 * (Note: this is a linear search).
 */
void *
vll_find (vocList_t *list, void *obj, long *index)
{
    /* Use the cached index value, if given. */
    if (index)
	vll_seek (list, *index, SEEK_SET);

    /* Get node at current position. */
    vocList_node_t *node = list->index[list->pos];

    /* Search forward for the given Obj value. */
    for (;  node != NULL;  node=node->next) {
	if (node->value == obj)
	    return (node->value);
    }

    return (NULL);
}


/*
 * vll_prev -- Seek to the previous position in the list.
 *
 * @brief   Get the previous element in the list
 * @fn	    obj = vll_prev (vocList_t *list)
 *
 * @param   list	Linked list to be accessed
 * @returns		Pointer to the object stored at that node
 *
 * If the seek position is valid a pointer to the object at that location
 * is returned, otherwise NULL.  Attempting to seek beyond the beginning 
 * of the list results in NULL being returned.
 */
void *
vll_prev (vocList_t *list)
{
    if (list->pos <= 0)
	return (NULL);
    else
	return (vll_seek (list, -1, SEEK_CUR));
}


/*
 * vll_next -- Seek to the next position in the list.
 *
 * @brief   Get the next element in the list
 * @fn	    obj = vll_next (vocList_t *list)
 *
 * @param   list	Linked list to be accessed
 * @returns		Pointer to the object stored at that node
 *
 * If the seek position is valid a pointer to the object at that location
 * is returned, otherwise NULL.  Attempting to seek beyond the end of the
 * list results in NULL being returned.
 */
void *
vll_next (vocList_t *list)
{
    if (list->pos >= (list->count - 1))
	return (NULL);
    else
	return (vll_seek (list, 1, SEEK_CUR));
}


/*
 * vll_insert -- Insert a new object in the list.
 *
 * @brief   Insert a new object into the list
 * @fn	    obj = vll_insert (vocList_t *list, void *obj)
 *
 * @param   list	Linked list to be accessed.
 * @param   obj		Pointer to the external object
 * @param   index	Pointer to location to store index value
 * @returns		Pointer to the inserted object
 *
 * A new node is inserted into the list containing a reference to the given
 * object.  The new element is inserted after the node at the current
 * position.  A prior seek to (-1,SEEK_SET) causes the new node to be
 * inserted at the head of the list, whereas an insert following a seek to
 * (0,SEE_END) is the same as an append.  A pointer to the inserted object is
 * returned upon success, otherwise NULL.  The list is not modified if the
 * insert fails.  The list is left positioned to the inserted element.
 *
 * A pointer to a location (&long) to store the index value for the node
 * may optionally be provided.  If this feature is enabled, the List will
 * store a copy of the index value for the node in the given location,
 * updating it whenever the index is rebuilt.  This speeds up operations
 * where one has the external object and wants to perform some list operation
 * upon it, but does not know its position in the list.  Otherwise,
 * vll_find() has to perform a linear search of the list to find the
 * referenced object.
 */
void *
vll_insert (vocList_t *list, void *obj, long *index)
{
    vocList_node_t *prev, *next;
    long pos = list->pos;

    /* Allocate the new node. */
    vocList_node_t *node =
	(vocList_node_t *) malloc(sizeof(vocList_node_t));
    if (node == NULL)
	return (NULL);

    /* Initialize the new node. */
    if (pos < 0) {
	prev = NULL;
	next = list->first;
    } else {
	prev = list->index[pos];
	next = (prev != NULL) ? prev->next : NULL;
    }

    node->prev = prev;
    node->next = next;
    node->value = obj;
    node->index = index;

    /* Prepare to modify the list. */
    if (list->count+1 > list->size)
	if (vll_rebuild_index (list, list->size * 2) < 0) {
	    free ((void *)node);
	    return (NULL);
	}

    /* Insert it into the list. */
    if (prev != NULL)
	prev->next = node;
    if (next != NULL)
	next->prev = node;

    if (prev == NULL)
	list->first = node;
    if (next == NULL)
	list->last = node;

    list->count++;
    list->pos++;

    /* Insert it into the index. */
    list->index[list->pos] = node;
    if (index)
	*index = list->pos;

    /* If the node was not appended rebuild the index. */
    if (prev != NULL && next != NULL)
	vll_rebuild_index (list, 0);

    return (obj);
}


/*
 * vll_append -- Append an object to the list.
 * 
 * @brief   Append an object to the list
 * @fn	    vll_append (vocList_t *list, void *obj)
 *
 * @param   list	Linked list to be accessed.
 * @param   obj		Pointer to the external object
 * @param   index	Pointer to location to store index value
 * @returns		Pointer to the appended object
 *
 * A new node is inserted into the list containing a reference to the given
 * object.  The new element is inserted after the node at the current position.
 * A pointer to the inserted object is returned upon success, otherwise NULL.
 * The list is not modified if the insert fails.  The list is left positioned
 * to the inserted element.
 */
void *
vll_append (vocList_t *list, void *obj, long *index)
{
    list->pos = list->count - 1;
    return (vll_insert (list, obj, index));
}


/*
 * vll_remove -- Remove the node at the given position from the list.
 *
 * @brief   Remove a node from the list
 * @fn	    vll_remove (vocList_t *list, long req_pos)
 *
 * @param   list	Linked list to be accessed.
 * @param   pos		Position of item to be removed.
 * @returns		Pointer to the object stored in the node
 *
 * POS should be a valid node index from 0 to count-1, or the value -1
 * which causes the node at the current position to be removed (this allows
 * a node to be removed without having to know its absolute position in the
 * list).  A pointer to the object referenced by the deleted node is
 * returned upon successful completion, otherwise NULL is returned.
 * POS is left pointing to the element preceding the deleted element.
 */
void *
vll_remove (vocList_t *list, long req_pos)
{
    /* Locate the node to be removed, or do nothing if current node. */
    if (req_pos == -1) {
	/* Check that we are positioned to a valid node. */
	if (list->pos < 0 || list->pos >= list->count)
	    return (NULL);
    } else {
	if (vll_seek (list, req_pos, SEEK_SET) == NULL)
	    return (NULL);
    }

    /* The actual node position may differ from req_pos. */
    long pos = list->pos;
    vocList_node_t *node = list->index[pos];
    if (node == NULL)
	return (NULL);

    /* Remove it from the list. */
    if (node->prev != NULL)
	node->prev->next = node->next;
    if (node->next != NULL)
	node->next->prev = node->prev;

    if (node->prev == NULL)
	list->first = node->next;
    if (node->next == NULL)
	list->last = node->prev;

    list->count--;
    list->pos--;
    if (list->pos < -1)
	list->pos = -1;

    /* Free the node. */
    list->index[pos] = NULL;
    free ((void *)node);

    /* If the node was not at the end of list, rebuild the index. */
    if (node->next != NULL)
	vll_rebuild_index (list, 0);

    return (node->value);
}


/*
 * vll_destroy -- Destroy a List object and free all resources.
 *
 * @brief   Destroy the List object and free resources
 * @fn	    vll_destroy (vocList_t *list, void(*func)(void *ptr))
 *
 * @param   list	Linked List object to be destroyed.
 * @param   func	Optional object delete function
 * @returns		No return value
 *
 * A pointer to a function to delete the "value" elements of nodes may
 * optionally be specified, otherwise this operation only affects the list
 * itself, and the external objects referenced in list elements are not
 * affected.  The signature for FUNC is the same as free().
 */
void
vll_destroy (vocList_t *list, void(*func)(void *ptr))
{
    vocList_node_t *node, *next;

    /* Free all the node structures. */
    for (node=next=list->first;  node != NULL;  node=next) {
	next = node->next;
	if (node != NULL) {
	    if (func != NULL)
		func (node->value);
	    free ((void *)node);
	}
    }

    /* Free the List structure and index. */
    free ((void *)list->index);
    free ((void *)list);
}


/*
 * vll_rebuild_index -- Internal routine to reallocate and rebuild the array
 * index for the list if it overflows, or if elements are deleted other than
 * at the end.
 *
 * @brief   Rebuild the array index
 * @fn	    vll_rebuild_index (vocList_t *list, int req_size)
 *
 * @param   list	Linked List object to be destroyed.
 * @param   size	Size for the new list, or zero if no change.
 * @returns		Execution status code
 *
 * EXIT_SUCCESS is returned if the new index is created successfully,
 * otherwise EXIT_FAILURE.  If an error occurs the old index is not replaced,
 * but may no longer be valid (depending upon the calling routine).
 */
int
vll_rebuild_index (vocList_t *list, int req_size)
{
    vocList_node_t **new_index = NULL;
    vocList_node_t *node;
    long new_size;

    /* Allocate a new index array if required. */
    if (req_size != 0 && req_size != list->size) {
	new_size = req_size;
	new_index =
	    (vocList_node_t **) calloc((size_t)req_size, sizeof(vocList_node_t *));
	if (new_index == NULL)
	    return (EXIT_FAILURE);
    } else {
	new_size = list->size;
	new_index = list->index;
    }

    /* Compute the new index. */
    int count = 0;
    for (node=list->first;  node != NULL;  node=node->next) {
	if (node->index)
	    *(node->index) = count;
	new_index[count++] = node;
    }

    /* Install the new index. */
    if (new_index != list->index) {
	free ((void *)list->index);
	list->index = new_index;
    }
    list->size = new_size;
    list->count = count;
    if (list->pos > count)
	list->pos = count - 1;

    return (EXIT_SUCCESS);
}
