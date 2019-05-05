#ifndef HASHTABLE_H
#define HASHTABLE_H

typedef struct {
    unsigned int key;
    void *value;
    void *next;
} hashtable_entry_t;

typedef struct {
    unsigned int (*hash)(void *);
    hashtable_entry_t **buckets;
    int bucket_count;
} hashtable_t;

hashtable_t *create_hashtable(int buckets, unsigned int (*hash_function)(void *));
void free_hashtable(hashtable_t *table);
void *hashtable_get(hashtable_t *table, void *key);
void *hashtable_set(hashtable_t *table, void *key, void *value);

#endif
