/*
 * hash.h
 */

typedef struct hsearch_data hashtable;

hashtable *hashtable_create(int);
int hashtable_insert(hashtable *, char *, void *);
void hashtable_destroy(hashtable *);
void *hashtable_lookup(hashtable *, char *);
void hashtable_delete(hashtable *, char *);
