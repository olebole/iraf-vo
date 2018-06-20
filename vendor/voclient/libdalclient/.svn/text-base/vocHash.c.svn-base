/*
 * VOCHASH - Simple C Hashtable Class.
 *
 *	  vht = vht_init (size, hash_func, flag)
 *	obj = vht_search (vht, key, keylen)
 *     obj = vht_findKey (vht, stringKey)
 *	obj = vht_insert (vht, key, keylen, value, vallen)
 *   obj = vht_insertKey (vht, stringKey, value, vallen)
 *	      vht_remove (vht, key, keylen)
 *	   vht_removeKey (vht, stringKey)
 *		vht_grow (vht, new_size)
 *	     vht_destroy (vht)
 *
 *	   vht_iter_init (vht, iterator)
 *	    vht_iter_inc (iterator)
 *		vht_hash (key, keylen, hashtab_size
 *
 * The hash table may be any size, set when initially created, and may be
 * nondestructively grown during usage if desired.  Growing does not happen
 * automatically, nor is it strictly required, but the hash table can become
 * inefficient if multiple keys has to the same hash slot.
 *
 * Both keys and values are opaque objects and may be any size.  Convenience
 * routines are provided for string-valued keys.  A custom hash function may
 * be supplied at init time to hash whatever type of key is supplied.
 * A default has function that merely hashes a sequence of bytes is provided
 * and is the default if no hash function is supplied.
 *
 * Original by C.Wellons; adapted for VOClient usage by D.Tody Dec 2013
 *
 * @file	vocHash.c
 * @author	Doug Tody
 * @version	December 2013
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vocHash.h"


/*
 * vht_init -- Create a new, empty hash table.
 *
 * 	size		N slots in initial hash table
 * 	hash_func	Hash function (optional, may be NULL)
 * 	flag		Set to "case" (default) or "nocase"
 *
 * A unique key is determined by the hash value of the key data
 * and the keylen in bytes.  In this implementation, only a single
 * instance of a key is permitted; inserts for the same key
 * overwrite the old instance.  A possible future enhancement
 * would be to permit multiple instances with the same key.
 */
vocHash_t *
vht_init (size_t size, int(*hash_func)(void *,size_t,size_t), char *flag)
{
    vocHash_t *new_ht;

    /* Hashtab descriptor */
    new_ht = (vocHash_t *) malloc(sizeof(vocHash_t));

    /* Initial hash array storage. */
    new_ht->arr =
	(vocHash_node_t **) malloc(sizeof(vocHash_node_t *) * size);

    new_ht->size = size;
    new_ht->count = 0;

    /* Init all entries to empty */
    int i = 0;
    for (i=0;  i < (int)size;  i++) {
	new_ht->arr[i] = NULL;
    }

    /* Init hash function. */
    if (hash_func == NULL)
	new_ht->hash_func = &vht_hash;
    else
	new_ht->hash_func = hash_func;

    /* Set any flags. */
    if (flag && strcmp(flag,"nocase") == 0)
	new_ht->nocase = 1;
    else
	new_ht->nocase = 0;

    return (new_ht);
}


/*
 * vht_search -- Look up the given key in the hash table, returning
 * a pointer to the found entry, or NULL.
 *
 * 	hashtable	Hashtable to be searched
 * 	key		Hash key
 * 	keylen		Hash key length
 *
 * The hash key datatype and size is arbitrary but must agree with
 * what the hash function expects.
 */
void *
vht_search (vocHash_t *hashtable, void *key, size_t keylen)
{
    int index = vht_hash(key, keylen, hashtable->size);

    /* Key not found. */
    if (hashtable->arr[index] == NULL)
	return NULL;

    /* Search for matching entry at this hash index.
     */
    vocHash_node_t *last_node = hashtable->arr[index];
    while (last_node != NULL) {
	/* Only compare matching keylens */
	if (last_node->keylen == keylen) {
	    /* Compare keys */
	    if (memcmp(key, last_node->key, keylen) == 0) {
		return (last_node->value);
	    }
	}

	last_node = last_node->next;
    }

    return (NULL);
}


/*
 * vht_findKey -- Convenience routine to search for a string-valued key.
 *
 * 	hashtable	Hashtable to be used
 * 	key		Hash key to search for
 *
 * This routine is limited to string-valued keys.  What is returned is
 * a pointer to the external Value object originally input to vht_insertKey.
 */
void *
vht_findKey (vocHash_t *hashtable, char *key)
{
    char *ip, *op, keyval[128];

    /* If keys are case-insensitive, enter keys into table as lower case. */
    if (hashtable->nocase) {
	for (ip=key, op=keyval;  (*op = tolower(*ip));  ip++, op++)
	    ;
    } else
	strcpy (keyval, key);

    void **obj = (void **) vht_search (hashtable, (void *)keyval, strlen(keyval)+1);

    return (obj ? *obj : NULL);
}


/*
 * vht_insert -- Insert a key, value pair in the hash table.
 *
 * 	hashtable	Hashtable to be used
 * 	key		Ptr to the key
 * 	keylen		Size in bytes of the key
 * 	value		Ptr to the value
 * 	vallen		Size in bytes of value data
 *
 * If a matching key,keylen entry already exists (key and keylen
 * identical) it is updated.  If the insertion is successful a
 * pointer to the value element is returned, otherwise NULL is
 * returned.
 */
void *
vht_insert (vocHash_t *hashtable,
    void *key, size_t keylen, void *value, size_t vallen)
{

    vocHash_node_t *next_node, *last_node;
    int index;

    index = vht_hash(key, keylen, hashtable->size);
    next_node = hashtable->arr[index];
    last_node = NULL;

    /* Search for an existing key. */
    while (next_node != NULL) {
	/* Only compare matching keylens */
	if (next_node->keylen == keylen) {
	    /* Compare keys */
	    if (memcmp(key, next_node->key, keylen) == 0) {
		/* This key already exists, replace it */
		if (next_node->vallen != vallen) {
		    /* New value is a different size */
		    free(next_node->value);
		    next_node->value = malloc(vallen);
		    if (next_node->value == NULL)
			return (NULL);
		}
		memcpy(next_node->value, value, vallen);
		next_node->vallen = vallen;
		return (next_node->value);
	    }
	}

	last_node = next_node;
	next_node = next_node->next;
    }

    /* Create a new node. */
    vocHash_node_t *new_node;
    new_node = (vocHash_node_t *) malloc(sizeof(vocHash_node_t));
    if (new_node == NULL)
	return (NULL);

    /* Get some memory for the new node data. */
    new_node->key = malloc(keylen);
    new_node->value = malloc(vallen);
    if (new_node->key == NULL || new_node->key == NULL) {
	free(new_node->key);
	free(new_node->value);
	free(new_node);
	return (NULL);
    }

    /* Copy over the value and key. */
    memcpy(new_node->key, key, keylen);
    memcpy(new_node->value, value, vallen);
    new_node->keylen = keylen;
    new_node->vallen = vallen;

    /* No next node. */
    new_node->next = NULL;

    /* Tack the new node on the end or right on the table. */
    if (last_node != NULL)
	last_node->next = new_node;
    else
	hashtable->arr[index] = new_node;

    hashtable->count++;
    return (new_node->value);
}


/*
 * vht_insertKey -- Convenience routine to insert a string-valued key.
 *
 * 	hashtable	Hash table to be used
 * 	key		Key to be hashed
 * 	value		Pointer to external data object
 *
 * This routine is limited to string-valued keys.  Value is a pointer to some
 * external object.  Unlike vht_insert, the content of the Value object is not
 * copied into internal storage, rather only the value of the Value object
 * pointer is stored.
 */
void *
vht_insertKey (vocHash_t *hashtable, char *key, void *value)
{
    char *ip, *op, keyval[128];

    /* Ignore the request if the key is null or the null string. */
    if (!key || strlen(key) == 0)
	return (NULL);

    /* If keys are case-insensitive, enter keys into table as lower case. */
    if (hashtable->nocase) {
	for (ip=key, op=keyval;  (*op = tolower(*ip));  ip++, op++)
	    ;
    } else
	strcpy (keyval, key);

    return (vht_insert(hashtable,
	(void *)keyval, strlen(keyval)+1, &value, sizeof(value)));
}


/*
 * vht_remove - Delete the given key from the hashtable.
 *
 * 	hashtable	Hash table to be used
 * 	key		Key to be removed
 * 	keylen		Key length (must match)
 *
 * It is not an error if the key is not found.
 */
void
vht_remove (vocHash_t *hashtable, void *key, size_t keylen)
{
    vocHash_node_t *last_node, *next_node;
    int index = vht_hash(key, keylen, hashtable->size);
    next_node = hashtable->arr[index];
    last_node = NULL;

    while (next_node != NULL) {
	if (next_node->keylen == keylen) {
	    /* Compare keys */
	    if (memcmp(key, next_node->key, keylen) == 0) {
		/* Free node memory */
		free(next_node->value);
		free(next_node->key);

		/* Adjust the list pointers */
		if (last_node != NULL)
		    last_node->next = next_node->next;
		else
		    hashtable->arr[index] = next_node->next;

		/* Free the node */
		free(next_node);
		break;
	    }
	}

	last_node = next_node;
	next_node = next_node->next;
    }
}


/*
 * vht_removeKey -- Convenience routine to remove a string-valued key.
 */
void
vht_removeKey (vocHash_t *hashtable, char *key)
{
    char *ip, *op, keyval[128];
    /* If keys are case-insensitive, enter keys into table as lower case. */
    if (hashtable->nocase) {
	for (ip=key, op=keyval;  (*op = tolower(*ip));  ip++, op++)
	    ;
    } else
	strcpy (keyval, key);

    vht_remove(hashtable, (void *)keyval, strlen(keyval)+1);
}


/*
 * ht_grow -- Grow the hashtable.
 *
 * 	old_ht		Hash table to be modified.
 * 	new_size	N hash slots in new table.
 *
 * A pointer to the new hash table is returned upon successful
 * completion, otherwise NULL.  If an error occors the old
 * hash table is unmodified.
 */
void *
vht_grow (vocHash_t *old_ht, size_t new_size)
{
    /* Create new hashtable. */
    char *flag = old_ht->nocase ? "nocase" : NULL;
    vocHash_t *new_ht = vht_init(new_size, old_ht->hash_func, flag);
    if (new_ht == NULL)
	return (NULL);

    void *ret;			/* captures return values */

    /* Iterate through the old hashtable and insert all entries
     * into the new table.
     */
    vocHash_iter_t ii;
    vht_iter_init(old_ht, &ii);

    for (; ii.key != NULL; vht_iter_inc(&ii)) {
	ret = vht_insert(new_ht, ii.key, ii.keylen, ii.value, ii.vallen);
	if (ret == NULL) {
	    /* Insert failed. Destroy new hashtable and return. */
	    vht_destroy(new_ht);
	    return (NULL);
	}
    }

    /* Destroy the old hashtable. */
    vht_destroy(old_ht);

    return (new_ht);
}


/* vht_destroy -- Free all resources used by the hashtable.
 *
 * 	hashtable	Hashtable to be destroyed.
 */
void
vht_destroy (vocHash_t *hashtable)
{
    vocHash_node_t *next_node, *last_node;

    /* Free each linked list in hashtable. */
    int i;
    for (i=0;  i < (int)hashtable->size;  i++) {
	next_node = hashtable->arr[i];
	while (next_node != NULL) {
	    /* Destroy node */
	    free(next_node->key);
	    free(next_node->value);
	    last_node = next_node;
	    next_node = next_node->next;
	    free(last_node);
	}
    }

    free(hashtable->arr);
    free(hashtable);
}


/*
 * vht_iter_init -- Initialize a new hash table iterator.
 *
 * 	hashtable	Hashtable to be iterated
 * 	ii		Iterator
 */
void
vht_iter_init (vocHash_t *hashtable, vocHash_iter_t *ii)
{
    /* Stick in initial bookeeping data. */
    ii->internal.hashtable = hashtable;
    ii->internal.node = NULL;
    ii->internal.index = -1;

    /* Iave iterator point to first element. */
    vht_iter_inc(ii);
}


/*
 * vht_iter_inc -- Incrememt the iterator.
 *
 * 	ii		Active iterator
 */
void
vht_iter_inc (vocHash_iter_t *ii)
{
    vocHash_t *hashtable = ii->internal.hashtable;
    int index = ii->internal.index;

    /* Attempt to grab the next node. */
    if (ii->internal.node == NULL || ii->internal.node->next == NULL)
	index++;
    else {
	/* Next node in the list. */
	ii->internal.node = ii->internal.node->next;
	ii->key = ii->internal.node->key;
	ii->value = ii->internal.node->value;
	ii->keylen = ii->internal.node->keylen;
	ii->vallen = ii->internal.node->vallen;

	return;
    }

    /* Find next node */
    while (hashtable->arr[index] == NULL && index < (int) hashtable->size)
	index++;

    if (index >= (int) hashtable->size) {
	/* End of hashtable */
	ii->internal.node = NULL;
	ii->internal.index = (int) hashtable->size;

	ii->key = NULL;
	ii->value = NULL;
	ii->keylen = 0;
	ii->vallen = 0;

	return;
    }

    /* Point to the next item in the hashtable. */
    ii->internal.node = hashtable->arr[index];
    ii->internal.index = index;
    ii->key = ii->internal.node->key;
    ii->value = ii->internal.node->value;
    ii->keylen = ii->internal.node->keylen;
    ii->vallen = ii->internal.node->vallen;
}


/*
 * vht_hash -- Default, generic hash function.
 *
 * 	key		Key to be hashed
 * 	keylen		Size in bytes of the key
 * 	hashtab_size	Number of slots in hash table
 */
int
vht_hash (void *key, size_t keylen, size_t hashtab_size)
{
    int sum = 0;

    /* Very simple hash function for now. */
    int i;
    for (i=0;  i < (int)keylen;  i++) {
	sum += ((unsigned char *) key)[i] * (i + 1);
    }

    return (sum % (int)hashtab_size);
}
