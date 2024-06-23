#ifndef _TMS_HASH__H_
#define _TMS_HASH__H_

#include <stdint.h>
#include <stdlib.h>

struct thash {
    void **tbl;
    int (*add)(struct thash *, void *, void *);
    int (*rm)(struct thash *, void *);
    void* (*get)(struct thash *, void *);
    uint32_t size;
    uint32_t mask;

    uint32_t (*hash_fn)(void *, uint32_t sz);
};

struct thash_entry_ptrdata {
    void *key;
    void *data;
    void *next;
};

struct thash_entry_string {
    char    *key;
    size_t key_len;
    void *data;
    void *next;
};

struct thash_entry_uint32 {
    uint32_t key;
    void *data;
    void *next;
};

struct thash_entry_pointer {
    void *ptr;
    void *next;
};

struct thash *thash_create_ptrdata_table(uint32_t tbl_sz);
void thash_set_hash_fn(struct thash *h, uint32_t (*fn)(void *, uint32_t sz));
void thash_free(struct thash *h);

/* available builting hash functions */
uint32_t thash_fn_jenkins(void *key, uint32_t key_size);
/* lol jk theres only one builtin hash function */

#define thash_add(h, ... ) h->add(h, __VA_ARGS__ )
#define thash_rm(h, ... ) h->rm(h, __VA_ARGS__ )
#define thash_get(h, ... ) h->get(h, __VA_ARGS__ )

#endif
