
#include "hash.h"
#include <tms/backend/print.h>

/**
 * Round up to the nearest power of 2
 * works only on 32 bit integers
 **/
static inline uint32_t tnpo2_uint32(uint32_t i)
{
    i--;
    i |= i >> 1;
    i |= i >> 2;
    i |= i >> 4;
    i |= i >> 8;
    i |= i >> 16;
    i ++;

    return i;
}

static int add_ptrdata(struct thash *h, void* key, void *data);
static int rm_ptrdata(struct thash *h, void* key);
static void* get_ptrdata(struct thash *h, void* key);

uint32_t
thash_fn_jenkins(void *data, uint32_t len)
{
    uint32_t h, x;
    char *dd = (char*)data;

    for (h = x = 0; x<len; x++) {
        h += dd[x];
        h += (h << 10);
        h ^= (h >> 6);
    }

    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);

    return h;
}

void thash_free(struct thash *t)
{
    if (t->tbl) free(t->tbl);
    free(t);
}

static inline struct thash *
thash_create(uint32_t tbl_sz)
{
    struct thash *th = calloc(1, sizeof(struct thash));

    if (!th) {
        tms_fatalf("out of mem (thc)");
    }

    th->size = tnpo2_uint32(tbl_sz);
    th->mask = th->size - 1;
    th->hash_fn = thash_fn_jenkins;

    (th->tbl = calloc(th->size, sizeof(struct thash_entry*)))
        || tms_fatalf("out of mem (thc_tbl)");

    return th;
}

/**
 * Create a hash table of pointers to data.
 *
 * @relates thash
 **/
struct thash *
thash_create_ptrdata_table(uint32_t tbl_sz)
{
    struct thash *th = thash_create(tbl_sz);

    th->add = add_ptrdata;
    th->rm = rm_ptrdata;
    th->get = get_ptrdata;

    return th;
}

static int
rm_ptrdata(struct thash *h, void *key)
{
    return 1;
}

static void*
get_ptrdata(struct thash *h, void *key)
{
    struct thash_entry_ptrdata **tbl = (struct thash_entry_ptrdata **)h->tbl;
    uint32_t hash = h->hash_fn(&key, sizeof(void*)) & h->mask;
    struct thash_entry_ptrdata *e = tbl[hash];

    if (e) {
        do {
            if (e->key == key) return e->data;
        } while ((e = e->next));
    }

    return 0;
}

static int
add_ptrdata(struct thash *h, void *key, void *data)
{
    struct thash_entry_ptrdata **tbl = (struct thash_entry_ptrdata **)h->tbl;

    uint32_t hash = h->hash_fn(&key, sizeof(void*)) & h->mask;
    struct thash_entry_ptrdata *e = tbl[hash];

    if (e) {
        do {
            if (e->key == key) return -1;
        } while ((e = e->next));
    }

    struct thash_entry_ptrdata *n = malloc(sizeof(struct thash_entry_ptrdata));
    n->next = tbl[hash];
    n->key = key;
    n->data = data;
    tbl[hash] = n;

    return 0;
}
