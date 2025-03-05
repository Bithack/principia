#include "worker.hh"
#include "entity.hh"
#include "settings.hh"

#include <tms/core/graph.h>
#include <Box2D/Dynamics/b2Island.h>
#include <Box2D/Dynamics/b2ContactManager.h>
#include <Box2D/Dynamics/Contacts/b2Contact.h>

static int  _worker_main(void *in);
static void _w_do_solve(struct worker *w);
static void _w_do_collide(struct worker *w);
static void _w_do_mstep(struct worker *w);
static void _w_do_updatec(struct worker *w);

static void _w_finish_solve(struct worker *w);
static void _w_finish_collide(struct worker *w);
static void _w_finish_mstep(struct worker *w);
static void _w_finish_updatec(struct worker *w);

worker w_workers[MAX_WORKERS];

volatile bool   w_solve_island_slots[MAX_WORKERS+1];
b2TimeStep      w_solve_step;
b2Vec2          w_solve_gravity;
bool            w_solve_allow_sleep;

b2ContactManager *w_collide_contact_manager;
std::vector<b2Contact*>  w_collide_destroy_list;
SDL_mutex               *w_collide_destroy_lock;

std::set<entity*>       *w_updatec_set;
struct tms_graph        *w_updatec_graph;

std::set<entity*>       *w_mstep_set;

static bool initialized = false;
static int num_running_workers = 0;

bool
w_is_enabled()
{
    return num_running_workers > 0;
}

int
w_get_num_workers()
{
    return num_running_workers;
}

void
w_init()
{
    if (!initialized) {
        initialized = true;
        num_running_workers = 0;

        w_collide_destroy_lock = SDL_CreateMutex();

        for (int x=0; x<MAX_WORKERS; x++) {
            w_workers[x].mtx = SDL_CreateMutex();
            w_workers[x].cond = SDL_CreateCond();
        }
    }

    /* exit and wait for all running workers */
    for (int x=0; x<num_running_workers; x++) {
        SDL_LockMutex(w_workers[x].mtx);
        w_workers[x].msg = W_RUN_EXIT;
        SDL_UnlockMutex(w_workers[x].mtx);

        SDL_WaitThread(w_workers[x].thr, 0);
    }

    num_running_workers = settings["num_workers"]->v.i;
    if (num_running_workers > MAX_WORKERS)
        num_running_workers = MAX_WORKERS;

    for (int x=0; x<MAX_WORKERS+1; x++) {
        w_solve_island_slots[x] = true;
    }

    for (int x=0; x<num_running_workers; x++) {
        w_workers[x].status = W_RUNNING;
        w_workers[x].msg = W_RUN_NULL;
        w_workers[x].thr = SDL_CreateThread(_worker_main, "_worker_main", (void*)(intptr_t)x);
    }
}

void
w_wait(int w)
{
    if (w == -1) {
        /* wait for all workers */
        int num_running;
        do {
            num_running = num_running_workers;

            for (int x=0; x<num_running_workers; x++) {
                SDL_LockMutex(w_workers[x].mtx);
                if (w_workers[x].status == W_WAITING)
                    num_running --;
                SDL_UnlockMutex(w_workers[x].mtx);
            }
        } while (num_running > 0);
    } else {
        /* wait for specific worker */
        bool running = true;
        do {
            SDL_LockMutex(w_workers[w].mtx);
                if (w_workers[w].status == W_WAITING)
                    running = false;
            SDL_UnlockMutex(w_workers[w].mtx);
        } while (running);
    }
}

int
w_run(int runmode, void *data)
{
    int selected = -1;

    /* wait for an available thread */
    do {
        for (int x=0; x<num_running_workers; x++) {
            SDL_LockMutex(w_workers[x].mtx);
            if (w_workers[x].status == W_WAITING) {
                selected = x;

                switch (runmode) {
                    case W_RUN_SOLVE: w_workers[x].data.solve = *((struct wdata_solve*)data); break;
                    case W_RUN_COLLIDE: w_workers[x].data.collide = *((struct wdata_collide*)data); break;
                    case W_RUN_MSTEP: w_workers[x].data.mstep = *((struct wdata_mstep*)data); break;
                    case W_RUN_UPDATEC: w_workers[x].data.updatec = *((struct wdata_updatec*)data); break;
                }

                w_workers[x].status = W_RUNNING;
                w_workers[x].msg = runmode;
                SDL_CondSignal(w_workers[x].cond);
            }
            SDL_UnlockMutex(w_workers[x].mtx);

            if (selected != -1)
                break;
        }

    } while (selected == -1);

    return selected;
}

static int
_worker_main(void *in)
{
    int x = (int)(intptr_t)in;
    int r_msg = W_RUN_NULL;

    do {
        SDL_LockMutex(w_workers[x].mtx);

        /* report results if r_msg is not null */
        switch (r_msg) {
            case W_RUN_SOLVE: _w_finish_solve(&w_workers[x]); break;
            case W_RUN_COLLIDE: _w_finish_collide(&w_workers[x]); break;
            case W_RUN_MSTEP: _w_finish_mstep(&w_workers[x]); break;
            case W_RUN_UPDATEC: _w_finish_updatec(&w_workers[x]); break;
        }

        w_workers[x].status = W_WAITING;

        while (w_workers[x].msg == W_RUN_NULL) {
            SDL_CondWait(w_workers[x].cond, w_workers[x].mtx);
        }

        r_msg = w_workers[x].msg;
        w_workers[x].msg = W_RUN_NULL;

        SDL_UnlockMutex(w_workers[x].mtx);

        switch (r_msg) {
            case W_RUN_SOLVE: _w_do_solve(&w_workers[x]); break;
            case W_RUN_COLLIDE: _w_do_collide(&w_workers[x]); break;
            case W_RUN_MSTEP: _w_do_mstep(&w_workers[x]); break;
            case W_RUN_UPDATEC: _w_do_updatec(&w_workers[x]); break;
        }
    } while (r_msg != W_RUN_EXIT);

    return 0;
}

static void
_w_do_solve(struct worker *w)
{
    b2Profile profile;
    w->data.solve.island->Solve(&profile, w_solve_step, w_solve_gravity, w_solve_allow_sleep);
}

static void
_w_finish_solve(struct worker *w)
{
    w_solve_island_slots[w->data.solve.island_slot] = true;
}

static b2Contact* _collide_get_next(b2Contact *c)
{
    for (int x=0; c && x<num_running_workers; x++) {
        c = c->GetNext();
    }

    return c;
}

static void _collide_destroy(b2Contact *c)
{
    SDL_LockMutex(w_collide_destroy_lock);
    w_collide_destroy_list.push_back(c);
    SDL_UnlockMutex(w_collide_destroy_lock);
}

static void
_w_do_collide(struct worker *w)
{
    b2Contact* c = w->data.collide.contact;
    b2ContactManager *man = w_collide_contact_manager;

    while (c)
    {
        b2Fixture* fixtureA = c->GetFixtureA();
        b2Fixture* fixtureB = c->GetFixtureB();
        int32 indexA = c->GetChildIndexA();
        int32 indexB = c->GetChildIndexB();
        b2Body* bodyA = fixtureA->GetBody();
        b2Body* bodyB = fixtureB->GetBody();

        // Is this contact flagged for filtering?
        if (c->m_flags & b2Contact::e_filterFlag)
        {
            // Should these bodies collide?
            if (!bodyB->ShouldCollide(bodyA))
            {
                _collide_destroy(c);
                c = _collide_get_next(c);
                continue;
            }

            // Check user filtering.
            if (man->m_contactFilter && !man->m_contactFilter->ShouldCollide(fixtureA, fixtureB))
            {
                _collide_destroy(c);
                c = _collide_get_next(c);
                continue;
            }

            // Clear the filtering flag.
            c->m_flags &= ~b2Contact::e_filterFlag;
        }

        bool activeA = bodyA->IsAwake() && bodyA->m_type != b2_staticBody;
        bool activeB = bodyB->IsAwake() && bodyB->m_type != b2_staticBody;

        // At least one body must be awake and it must be dynamic or kinematic.
        if (!activeA && !activeB)
        {
            c = _collide_get_next(c);
            continue;
        }

        int32 proxyIdA = fixtureA->m_proxies[indexA].proxyId;
        int32 proxyIdB = fixtureB->m_proxies[indexB].proxyId;
        bool overlap = man->m_broadPhase.TestOverlap(proxyIdA, proxyIdB);

        // Here we destroy contacts that cease to overlap in the broad-phase.
        if (!overlap)
        {
            _collide_destroy(c);
            c = _collide_get_next(c);
            continue;
        }

        // The contact persists.
        c->Update(man->m_contactListener);
        c = _collide_get_next(c);
    }
}

static void
_w_finish_collide(struct worker *w)
{

}

static void
_w_do_updatec(struct worker *w)
{
    std::set<entity*>::iterator i = w_updatec_set->begin();
    std::set<entity*>::iterator end = w_updatec_set->end();

    if (w->data.updatec.i != 0)
        std::advance(i, w->data.updatec.i);

    while (i != end) {
        if (!w_updatec_graph || !tms_graph_is_entity_culled(w_updatec_graph, (*i)))
            tms_entity_update(static_cast<struct tms_entity*>(*i));

        for (int x=0; x<num_running_workers && i != end; x++)
            i++;
    }
}

static void
_w_finish_updatec(struct worker *w)
{

}

static void
_w_do_mstep(struct worker *w)
{
    std::set<entity*>::iterator i = w_mstep_set->begin();
    std::set<entity*>::iterator end = w_mstep_set->end();

    if (w->data.mstep.i != 0)
        std::advance(i, w->data.mstep.i);

    while (i != end) {
        (*i)->mstep();
        for (int x=0; x<num_running_workers && i != end; x++)
            i++;
    }

}

static void
_w_finish_mstep(struct worker *w)
{

}

