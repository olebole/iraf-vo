/* htest.c - Hashtable test
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vocHash.h"

#define _VOCLIENT_LIB_
#include "VOClient.h"
VOClient voc;

int
main() {
    char *keyval;

    /* Create the hash table. */
    vocHash_t *test_ht = ht_init(2, NULL);

    /* Stick some data into the table (using the string front-end) */
    keyval = "Language";
	ht_insertKey(test_ht, "Perl", keyval, strlen(keyval)+1);
    keyval = "System";
	ht_insertKey(test_ht, "GNU", keyval, strlen(keyval)+1);
    keyval = "Verbose";
	ht_insertKey(test_ht, "Java", keyval, strlen(keyval)+1);
    keyval = "Instant Messenger";
	ht_insertKey(test_ht, "Pidgin", keyval, strlen(keyval)+1);
    keyval = "Web Browser";
	ht_insertKey(test_ht, "Firefox", keyval, strlen(keyval)+1);

    /* Display table data */
    vocHash_iter_t ii;
    ht_iter_init(test_ht, &ii);
    for (;  ii.key != NULL;  ht_iter_inc(&ii)) {
	printf("%s => %s\n", (char *) ii.key, (char *) ii.value);
    }

    /* Grow the table */
    printf("---\nGROW!\n---\n");
    test_ht = ht_grow(test_ht, 10);

    /* Print the table contents again */
    ht_iter_init(test_ht, &ii);
    for (;  ii.key != NULL;  ht_iter_inc(&ii)) {
	printf("%s => %s\n", (char *) ii.key, (char *) ii.value);
    }

    /* Free the hashtable */
    ht_destroy(test_ht);

    return (EXIT_SUCCESS);
}
