/*
 * hash.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "log.h"

#ifndef  __USE_GNU
#define  __USE_GNU
#endif

#include <search.h>

#include "hash.h"

pthread_mutex_t hash_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * hashtable_create, creates hashtable with # entries
 * * entries, no # of entries
 */

hashtable *hashtable_create(int entries) {

	hashtable *tmp = NULL;

	tmp = calloc(1, sizeof(hashtable));

	if (!tmp) {
		lerror("calloc");
		return NULL;
	}

	if (hcreate_r(entries, tmp) < 1) {
		return NULL;
	}

	return tmp;
}

/*
 * hashtable_destroy, destory hashtable
 * * hash, given hashtable
 */

void hashtable_destroy(hashtable *hash) {

	/* CRITICAL SECTION */
	pthread_mutex_lock(&hash_mutex);

	if (hash) {
		hdestroy_r(hash);
	}

	pthread_mutex_unlock(&hash_mutex);
	/* CRITICAL SECTION */

	return ;
}

/*
 * hashtable_delete, delete entry
 * * hash, given hashtable
 * * key, given key
 */

void hashtable_delete(hashtable *hash, char *key) {
	ENTRY e, *ep;

	e.key = key;

	/* CRITICAL SECTION */
	pthread_mutex_lock(&hash_mutex);
		
	if (!hash || hsearch_r(e, FIND, &ep, hash) < 1) {
		pthread_mutex_unlock(&hash_mutex);
		return ;
	}

	free(ep->key);

	if (ep->data) {
		free(ep->data);
		ep->data = NULL;
	}

	pthread_mutex_unlock(&hash_mutex);
	/* CRITICAL SECTION */

	return ;
}

/*
 * hashtable_insert, insert entry into hashtable
 * * hash, given hashtable
 * * key, given key
 * * data, given data
 */

int hashtable_insert(hashtable *hash, char *key, void *data) {
	ENTRY e, *ep;
	int retval;

	hashtable_delete(hash, key);

	/* CRITICAL SECTION */
	pthread_mutex_lock(&hash_mutex);

	retval = 1;

	e.key = strdup(key);

	if (!e.key) {
		lerror("calloc");
		pthread_mutex_unlock(&hash_mutex);
		return -1;
	}

	e.data = strdup(data);

	if (!e.data) {
		lerror("calloc");
		free(e.key);
		pthread_mutex_unlock(&hash_mutex);
		return -1;
	}

	if (!hash || hsearch_r(e, ENTER, &ep, hash) < 1) {
		free(e.key);
		free(e.data);
		lerror("hsearch_r");
		retval = -1;
	}

	if (ep) { 
		if (!ep->data) {
			ep->data = e.data;
		}
	}

	pthread_mutex_unlock(&hash_mutex);
	/* CRITICAL SECTION */

	return retval;
}

/*
 * hashtable_lookup, lookup key in hashtable
 * * hash, given hashtable
 * * key, given key
 */

void *hashtable_lookup(hashtable *hash, char *key) {
	ENTRY e, *ep;

	e.key = key;

	/* CRITICAL SECTION */
	pthread_mutex_lock(&hash_mutex);

	if (!hash || hsearch_r(e, FIND, &ep, hash) < 1) {
		pthread_mutex_unlock(&hash_mutex);
		return NULL;
	}

	pthread_mutex_unlock(&hash_mutex);
	/* CRITICAL SECTION */

	return ep->data;
}
