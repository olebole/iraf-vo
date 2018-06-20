/* vocHash.h - Simple C Hashtable Class
 */

/* Only copies of keys and values are stored in the table, not the
 * orignals.
 *
 * The hashtable does not grow automatically, but only when the hashtable
 * grow function is called. Growing the hashtable is a safe operation:
 * if growing the hashtable fails, the existing hashtable is not
 * destroyed or modified.
 *
 * This hashtable is not thread-safe.
 */

#ifndef VOCHASH_H
#define VOCHASH_H

#include <stdlib.h>
#include <string.h>

/* Hash table node; one per key-value pair. */
typedef struct vocHash_node_t {
    void *key;			/* key for the node */
    size_t keylen;		/* length of the key */
    void *value;		/* value for this node */
    size_t vallen;		/* length of the value */
    struct vocHash_node_t *next;	/* next node (open hashtable) */
} vocHash_node_t;

/* Main descriptor for hash table. */
typedef struct vocHash_t {
    vocHash_node_t **arr;
    size_t size;		/* size of the hash */
    int count;			/* number if items in this table */
    int (*hash_func) (void *, size_t, size_t);	/* hash function */
    int nocase;			/* use case-insensitive compares */
} vocHash_t;

/* Iterator type for iterating through the hashtable. */
typedef struct vocHash_iter_t {
    /* key and value of current item */
    void *key;
    void *value;
    size_t keylen;
    size_t vallen;

    /* bookkeeping data */
    struct vocHash_internal_t {
	vocHash_t *hashtable;
	vocHash_node_t *node;
	int index;
    } internal;

} vocHash_iter_t;

/* Initialize a new hashtable (set bookingkeeping data) and return a
 * pointer to the hashtable. A hash function may be provided. If no
 * function pointer is given (a NULL pointer), then the built in hash
 * function is used. A NULL pointer returned if the creation of the
 * hashtable failed due to a failed malloc().
 */
vocHash_t *vht_init(size_t size,
    int (*hash_func) (void *key, size_t keylen, size_t ht_size), char *flag);

/* Fetch a value from table matching the key. Returns a pointer to
 * the value matching the given key.  The searchKey variant takes
 * a string-valued key.
 */
void *vht_search(vocHash_t *hashtable, void *key, size_t keylen);
void *vht_searchKey(vocHash_t *hashtable, char *key);

/* Put a value into the table with the given key. Returns NULL if
 * malloc() fails to allocate memory for the new node.  The insertKey
 * variant takes a string-valued key.
 */
void *vht_insert(vocHash_t *hashtable,
    void *key, size_t keylen, void *value, size_t vallen);
void *vht_insertKey(vocHash_t *hashtable, char *key, void *value);

/* Delete the given key and value pair from the hashtable. If the key
 * does not exist, no error is given.  The removeKey variant takes a
 * string-valued key.
 */
void vht_remove(vocHash_t *hashtable, void *key, size_t keylen);
void vht_removeKey(vocHash_t *hashtable, char *key);

/* Change the size of the hashtable. It will allocate a new hashtable
 * and move all keys and values over. The pointer to the new hashtable
 * is returned. Will return NULL if the new hashtable fails to be
 * allocated. If this happens, the old hashtable will not be altered
 * in any way. The old hashtable is destroyed upon a successful grow.
 */
void *vht_grow(vocHash_t * hashtable, size_t new_size);

/* Free all resources used by the hashtable. */
void vht_destroy(vocHash_t * hashtable);

/* Initialize the given iterator. It will point to the first element
 * in the hashtable.
 */
void vht_iter_init(vocHash_t * hashtable, vocHash_iter_t * ii);

/* Increment the iterator to the next element. The iterator key and
 * value will point to NULL values when the iterator has reached the
 * end of the hashtable.
 */
void vht_iter_inc(vocHash_iter_t * ii);

/* Default hashtable hash function. */
int vht_hash(void *key, size_t key_size, size_t hashtab_size);

#endif
