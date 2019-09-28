#ifndef LIST_H
#define LIST_H

#include <Zany80/internal/dllports.h>

typedef struct {
    int capacity;
    int length;
    void **items;
} list_t;

#ifdef __cplusplus
extern "C" {
#endif
ZANY_DLL list_t *create_list();
ZANY_DLL void list_free(list_t *list);
ZANY_DLL void list_add(list_t *list, void *item);
ZANY_DLL void list_del(list_t *list, int index);
ZANY_DLL void list_cat(list_t *list, list_t *source);
ZANY_DLL void list_foreach(list_t *list, void (*callback)(void *item));
ZANY_DLL int list_seq_find(list_t *list, int compare(const void *item, const void *data), const void *data);
ZANY_DLL void list_addunique(list_t *list, int compare(const void *item, const void *data), void *item);
ZANY_DLL void list_insert(list_t *list, int index, void *item);
ZANY_DLL int list_cmp_pointer(const void *item, const void *data);
ZANY_DLL int list_cmp_string(const void *item, const void *data);

#ifdef __cplusplus
}
#endif

#endif
