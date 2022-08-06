#ifndef _TLIST__H_
#define _TLIST__H_

/** @relates tlist @{ **/

/** 
 * General purpose linked list
 **/
struct tlist {
    struct tlist *next;
    void *data;
};

struct tlist* tlist_alloc();
void tlist_free(struct tlist *list);

struct tlist* tlist_append(struct tlist *list, void *data);
struct tlist* tlist_prepend(struct tlist *list, void *data);
struct tlist* tlist_insert(struct tlist *list, void *data, int position);
struct tlist* tlist_remove(struct tlist *list, void *data);
struct tlist* tlist_find(struct tlist *list, void *data);
struct tlist* tlist_find_custom(struct tlist *list, int (*cb)(void*));
struct tlist* tlist_last(struct tlist *list);
void tlist_foreach(struct tlist *list, void (*cb)(void*));
int tlist_length(struct tlist *list);

#endif
