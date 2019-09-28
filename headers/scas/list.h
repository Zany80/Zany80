#ifndef LIST_H
#define LIST_H

typedef struct {
    int capacity;
    int length;
    void **items;
} list_t;

list_t *create_list();
void list_free(list_t *list);
void list_add(list_t *list, void *item);
void list_del(list_t *list, int index);
void list_cat(list_t *list, list_t *source);
void list_addunique(list_t *list, int compare(const void *item, const void *data), void *item);
int list_cmp_pointer(const void *item, const void *data);
int list_cmp_string(const void *item, const void *data);
#endif
