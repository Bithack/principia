#include <stdlib.h>
#include <assert.h>

#include "list.h"
#include "../core/err.h"

struct tlist*
tlist_alloc()
{
    struct tlist *list = malloc(sizeof(struct tlist));

    return list;
}

void
tlist_free(struct tlist *list)
{
    /* XXX */
}

/** 
 * returns the new start of the tlist
 **/
struct tlist*
tlist_append(struct tlist *list, void *data)
{
    struct tlist *new_list;
    struct tlist *last;

    new_list = tlist_alloc();
    new_list->data = data;
    new_list->next = NULL;

    if (list) {
        last = tlist_last(list);
        last->next = new_list;

        return last;
    } else
        return new_list;
}

struct tlist*
tlist_prepend(struct tlist *list, void *data)
{
    struct tlist *new_list;

    new_list = tlist_alloc();
    new_list->data = data;
    new_list->next = list;

    return new_list;
}

struct tlist*
tlist_insert(struct tlist *list, void *data, int position)
{
    return 0;
}

struct tlist*
tlist_remove(struct tlist *list, void *data)
{
    struct tlist *tmp, *prev = NULL;

    tmp = list;
    while (tmp) {
        if (tmp->data == data) {
            if (prev)
                prev->next = tmp->next;
            else
                list = tmp->next;

            tlist_free(tmp);
            break;
        }
        prev = tmp;
        tmp = prev->next;
    }

    return list;
}

struct tlist*
tlist_find(struct tlist *list, void *data)
{
    while (list) {
        if (list->data == data)
            break;
        list = list->next;
    }

    return list;
}

struct tlist*
tlist_find_custom(struct tlist *list, int (*cb)(void*))
{
    assert((cb != NULL));

    while (list) {
        /* XXX: is one argument enough? */
        if (cb(list->data) == 0)
            break;

        list = list->next;
    }

    return list;
}

struct tlist*
tlist_last(struct tlist *list)
{
    if (list) {
        while (list->next)
            list = list->next;
    }

    return list;
}

void
tlist_foreach(struct tlist *list, void (*cb)(void*))
{
    while (list) {
        struct tlist *next = list->next;
        (*cb)(list->data);
        list = next;
    }
}

int
tlist_length(struct tlist *list)
{
    int length = 0;

    while (list) {
        ++length;
        list = list->next;
    }

    return length;
}
