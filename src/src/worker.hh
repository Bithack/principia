#pragma once

#include <Box2D/Box2D.h>
#include <SDL.h>
#include <vector>
#include <set>

#define MAX_WORKERS 8

#define W_RUN_EXIT    -1
#define W_RUN_NULL     0
#define W_RUN_SOLVE    1
#define W_RUN_COLLIDE  2
#define W_RUN_MSTEP    3
#define W_RUN_UPDATEC  4

#define W_RUNNING 0
#define W_WAITING 1

class b2Island;
class b2ContactManager;
class b2Contact;
class entity;
struct tms_graph;

struct wdata_solve {
    b2Island *island;
    int       island_slot;
};

struct wdata_collide {
    b2Contact *contact;
};

struct wdata_mstep {
    int i;
};

struct wdata_updatec {
    int i;
};

struct worker {
    int        status;
    int        msg;
    SDL_mutex *mtx;
    SDL_cond  *cond;
    SDL_Thread *thr;

    union {
        struct wdata_solve   solve;
        struct wdata_collide collide;
        struct wdata_mstep   mstep;
        struct wdata_updatec updatec;
    } data;
};

extern worker   w_workers[MAX_WORKERS];

/* shared data between solve workers */
extern volatile bool     w_solve_island_slots[MAX_WORKERS+1];
extern b2TimeStep        w_solve_step;
extern b2Vec2            w_solve_gravity;
extern bool              w_solve_allow_sleep;

/* shared data between collide workers */
extern b2ContactManager        *w_collide_contact_manager;
extern std::vector<b2Contact*>  w_collide_destroy_list;
extern SDL_mutex               *w_collide_destroy_lock;

/* shared data between custom update workers */
extern std::set<entity*>       *w_updatec_set;
extern struct tms_graph        *w_updatec_graph;

/* shared data between mstep workers */
extern std::set<entity*>       *w_mstep_set;

void w_init(void);
bool w_is_enabled();
int  w_get_num_workers();
void w_wait(int w); /* send -1 to wait for all */
int  w_run(int runmode, void *data);
