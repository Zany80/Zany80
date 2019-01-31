#ifndef LIST_H
#define LIST_H

typedef struct {
    int capacity;
    int length;
    void **items;
} list_t;

#ifdef __cplusplus
extern "C" {
#endif
list_t *create_list();
void list_free(list_t *list);
void list_add(list_t *list, void *item);
void list_del(list_t *list, int index);
void list_cat(list_t *list, list_t *source);
void list_foreach(list_t *list, void (*callback)(void *item));
#ifdef __cplusplus
}
#endif

#endif
