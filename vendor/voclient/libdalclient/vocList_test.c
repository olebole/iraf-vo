/* vocList_test.c - Array-List Class Test
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "vocList.h"


typedef struct node_t {
    int		number;
    long	index;
    char	name[64];
} node_t;

void *newNode (char *name);
void printNode (FILE *out, void *obj);
void printList (vocList_t *list);
void printBack (vocList_t *list);


int
main(int argc, char *argv[])
{
    struct node_t *node;
    int delay = 0;
    char *tag;

    /* Sleep at end to permit memory leak analysis (Leaks tool) */
    if (argc > 1 && strcmp (argv[1], "-sleep") == 0)
	delay = 60;

    /* Create the linked list. */
    vocList_t *list = vll_init(2);
    if (list == NULL) {
	fprintf (stderr, "cannot initialize List object\n");
	exit (errno);
    }

    /* Stick some data into the list. */
    tag = "aaa";  node = newNode (tag);
	vll_append (list, node, &node->index);
    tag = "bbb";  node = newNode (tag);
	vll_append (list, node, &node->index);
    tag = "ccc";  node = newNode (tag);
	vll_append (list, node, &node->index);
    tag = "ddd";  node = newNode (tag);
	vll_append (list, node, &node->index);
    tag = "eee";  node = newNode (tag);
	vll_append (list, node, &node->index);
    tag = "fff";  node = newNode (tag);
	vll_append (list, node, &node->index);
    tag = "ggg";  node = newNode (tag);
	vll_append (list, node, &node->index);
    tag = "hhh";  node = newNode (tag);
	vll_append (list, node, &node->index);

    /* Display the list. */
    printList (list);

    /* Make some edits. */
    vll_seek (list, -1, SEEK_SET);
    tag = "bol";  node = newNode (tag);
	vll_insert (list, node, &node->index);
    tag = "bol1";  node = newNode (tag);
	vll_insert (list, node, &node->index);
    tag = "bol2";  node = newNode (tag);
	vll_insert (list, node, &node->index);

    vll_seek (list, 6, SEEK_SET);
    tag = "after pos=6";  node = newNode (tag);
	vll_insert (list, node, &node->index);
    tag = "after pos=6+1";  node = newNode (tag);
	vll_insert (list, node, &node->index);
    tag = "after pos=6+2";  node = newNode (tag);
	vll_insert (list, node, &node->index);

    node_t *obj1 = vll_remove (list, 11);
    node_t *obj2 = vll_remove (list, 11);
    vll_append (list, (void *)obj1, &obj1->index);
    vll_append (list, (void *)obj2, &obj2->index);
    tag = "eol";  node = newNode (tag);
	vll_append (list, node, &node->index);
    tag = "eol1";  node = newNode (tag);
	vll_append (list, node, &node->index);
    tag = "eol2";  node = newNode (tag);
	vll_append (list, node, &node->index);

    /* Display the list again. */
    printf ("-------- after edits ----------\n");
    printList (list);

    /* Trim some nodes at EOL. */
    int count = vll_count (list);
    obj1 = vll_remove (list, count-1);
    count = vll_count (list);
    obj2 = vll_remove (list, count-1);
    printf ("-------- after trim at end ----------\n");
    printList (list);

    /* Delete entire list from the end. */
    while ((obj1 = vll_remove(list, vll_count(list)-1)) != NULL)
	;
    printf ("-------- empty list ----------\n");
    printList (list);

    /* Display the list backwards.
    printf ("-------- backwards ----------\n");
    printBack (list);
     */

    /* Free the list */
    vll_destroy(list, free);

    /* Leak some memory as a test.
    void *obj3 = newNode ("notfreed");
     */

    if (delay)
	sleep (delay);

    return (EXIT_SUCCESS);
}


/* newNode -- Create a test node.
 */
void *
newNode (char *name) {
    static int number = 1;

    node_t *node = (node_t *) malloc(sizeof(node_t));
    strcpy (node->name, name);
    node->number = number++;
    node->index = 0;

    return (node);
}


/* printNode -- Print a single node.
 */
void
printNode (FILE *out, void *obj) {
    node_t *node = (node_t *)obj;
    fprintf (out, "%3d %4ld %s\n", node->number, node->index, node->name);
}


/* printList -- Traverse the list and print the contents.
 */
void
printList (vocList_t *list) {
    void *obj;

    obj = vll_seek (list, 0, SEEK_SET);
    if (obj != NULL)
	printNode (stdout, obj);

    while ((obj = vll_next(list)) != NULL)
	printNode (stdout, obj);

    fflush (stdout);
}


/* printBack -- Traverse the list backwards and print the contents.
 */
void
printBack (vocList_t *list) {
    void *obj;

    obj = vll_seek (list, 0, SEEK_END);
    if (obj != NULL)
	printNode (stdout, obj);

    while ((obj = vll_prev(list)) != NULL)
	printNode (stdout, obj);

    fflush (stdout);
}
