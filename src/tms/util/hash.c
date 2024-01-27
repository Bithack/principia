
#include <tms/core/err.h>
#include "hash.h"
#include "util.h"
#include <string.h>

static int add_string(struct thash *h, char* key, size_t key_len, void *data);
static int rm_string(struct thash *h, char* key, size_t key_len);
static void* get_string(struct thash *h, char *key, size_t key_len);

static int add_ptr(struct thash *h, void *ptr);
static int rm_ptr(struct thash *h, void *ptr);
static void* get_ptr(struct thash *h, void *ptr);

static int add_uint32(struct thash *h, uint32_t key, void *data);
static int rm_uint32(struct thash *h, uint32_t key);
static void* get_uint32(struct thash *h, uint32_t key);

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

/**
 * Create a hash table of pointers.
 * The added pointers will act as both value and key.
 *
 * @relates thash
 **/
struct thash *
thash_create_pointer_table(uint32_t tbl_sz)
{
    struct thash *th = thash_create(tbl_sz);

    th->add = add_ptr;
    th->rm = rm_ptr;
    th->get = get_ptr;

    return th;
}

/**
 * Create a hash table with 32 bit usigned integers
 * as keys.
 *
 * @relates thash
 **/
struct thash *
thash_create_uint32_table(uint32_t tbl_sz)
{
    struct thash *th = thash_create(tbl_sz);

    th->add = add_uint32;
    th->rm = rm_uint32;
    th->get = get_uint32;

    return th;
}

/**
 * Create a hash table with strings as keys
 *
 * @relates thash
 **/
struct thash *
thash_create_string_table(uint32_t tbl_sz)
{
    struct thash *th = thash_create(tbl_sz);

    th->add = add_string;
    th->rm = rm_string;
    th->get = get_string;

    return th;
}

static int
rm_ptr(struct thash *h, void *ptr)
{
    tms_fatalf("tja");
    return 1;
}

static void*
get_ptr(struct thash *h, void *ptr)
{
    struct thash_entry_pointer **tbl = h->tbl;
    uint32_t hash = h->hash_fn(&ptr, sizeof(void*)) & h->mask;
    struct thash_entry_pointer *e = tbl[hash];

    if (e) {
        do {
            if (e->ptr == ptr) return ptr;
        } while ((e = e->next));
    }

    return 0;
}

static int
add_ptr(struct thash *h, void *ptr)
{
    /*
    va_list ap;
    va_start(ap, h);
    void *ptr = va_arg(ap, void*);
    va_end(ap);
    */
    struct thash_entry_pointer **tbl = h->tbl;

    uint32_t hash = h->hash_fn(&ptr, sizeof(void*)) & h->mask;
    struct thash_entry_pointer *e = tbl[hash];

    if (e) {
        do {
            if (e->ptr == ptr) return -1;
        } while ((e = e->next));
    }

    struct thash_entry_pointer *n = malloc(sizeof(struct thash_entry_pointer));
    n->next = tbl[hash];
    n->ptr = ptr;
    tbl[hash] = n;

    return 0;
}

static int
add_uint32(struct thash *h, uint32_t key, void *data)
{
    struct thash_entry_uint32 **tbl = h->tbl;
    uint32_t hash = h->hash_fn(&key, sizeof(uint32_t)) & h->mask;
    struct thash_entry_uint32 *e = tbl[hash];

    if (e) {
        do {
            if (e->key == key)return -1;
        } while ((e = e->next));
    }

    struct thash_entry_uint32 *n = malloc(sizeof(struct thash_entry_uint32));
    n->next = tbl[hash];
    n->key = key;
    n->data = data;
    tbl[hash] = n;

    return 0;
}

static void*
get_uint32(struct thash *h, uint32_t key)
{
    struct thash_entry_uint32 **tbl = h->tbl;
    uint32_t hash = h->hash_fn(&key, sizeof(uint32_t)) & h->mask;
    struct thash_entry_uint32 *e = tbl[hash];

    if (e) {
        do {
            if (e->key == key) return e->data;
        } while ((e = e->next));
    }

    return 0;
}

static int
rm_uint32(struct thash *h, uint32_t key)
{
    tms_fatalf("not implemented");
    return 0;
}

static int
add_string(struct thash *h, char* key, size_t key_len, void *data)
{
    struct thash_entry_string **tbl = h->tbl;
    uint32_t hash = h->hash_fn(key, key_len) & h->mask;
    struct thash_entry_string *e = tbl[hash];

    if (e) {
        do {
            if (e->key_len == key_len
                    && memcmp(e->key, key, key_len) == 0)
                return -1;
        } while ((e = e->next));
    }

    struct thash_entry_string *n = malloc(sizeof(struct thash_entry_string));
    n->next = tbl[hash];
    n->key = key; /* strdup? */
    n->key_len = key_len;
    n->data = data;
    tbl[hash] = n;

    return 0;
}

static void*
get_string(struct thash *h, char *key, size_t key_len)
{
    struct thash_entry_string **tbl = h->tbl;
    uint32_t hash = h->hash_fn(key, key_len) & h->mask;
    struct thash_entry_string *e = tbl[hash];

    if (e) {
        do {
            if (e->key_len == key_len
                    && memcmp(e->key, key, key_len) == 0)
                return e->data;
        } while ((e = e->next));
    }

    return 0;
}

static int
rm_string(struct thash *h, char *key, size_t key_len)
{
    tms_fatalf("not implemented");
    return 0;
}

static int
rm_ptrdata(struct thash *h, void *key)
{
    return 1;
}

static void*
get_ptrdata(struct thash *h, void *key)
{
    struct thash_entry_ptrdata **tbl = h->tbl;
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
    struct thash_entry_ptrdata **tbl = h->tbl;

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
