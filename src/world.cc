#include "world.hh"
#include "escript.hh"
#include "eventlistener.hh"
#include "solver.hh"
#include "solver_ingame.hh"
#include "group.hh"
#include "debugdraw.hh"
#include "object_factory.hh"
#include "game.hh"
#include "receiver.hh"
#include "soundman.hh"
#include "screenshot_marker.hh"
#include "ui.hh"
#include "model.hh"
#include "connection.hh"
#include "ragdoll.hh"
#include "fxemitter.hh"
#include "adventure.hh"
#include "i0o1gate.hh"
#include "worker.hh"
#include "damper.hh"
#include "pivot.hh"
#include "rubberband.hh"
#include "soundmanager.hh"
#include "simplebg.hh"
#include "scup.hh"
#include "factory.hh"
#include "gravityman.hh"
#include "faction.hh"
#include "robot_base.hh"
#include "impact_sensor.hh"
#include "animal.hh"
#include "misc.hh"
#include "crane.hh"
#include "repair_station.hh"

#include <algorithm>

#define QUERY_EPS .5f
#define RAY_LENGTH 4.f

world *W;

world::world()
    : level_id_type(LEVEL_LOCAL)
{
    tms_debugf("world initing");
    this->score_helper = 0 ^ SCORE_XOR;
    this->first_solve = false;
    this->paused = true;

    this->destruction_listener = new world::b2_destruction_listener();
    this->sleep_listener = new world::b2_sleep_listener();

    this->b2 = new b2World(b2Vec2(0, 0.f));
    this->b2->SetDebugDraw(this->debug = new debugdraw());
    this->b2->SetDestructionListener(this->destruction_listener);
    this->b2->SetSleepListener(this->sleep_listener);

    this->lb.cap = 131072;
    this->lb.min_cap = this->lb.cap;
    this->lb.sparse_resize = true;
    this->lb.size = 0;
    this->lb.buf = (unsigned char*)malloc(this->lb.cap);
    this->lb.clear();

    this->contacts_playing = new solver_ingame();
    this->contacts_paused = new solver();

    this->cwindow = new chunk_window();
}

void
world::draw_debug(tms::camera *cam)
{
    glLineWidth(1.f);
    this->debug->begin(cam);
    this->b2->DrawDebugData();
    this->debug->end();
}

void
world::insert_connection(connection *cc)
{
    this->connections.insert(cc);
}

void
world::erase_connection(connection *cc)
{
    if (cc->j) {
        this->destructable_joints.erase(cc->j);
    }

    if (cc->self_ent && cc->self_ent->scene) {
        G->remove_entity(cc->self_ent);
    }

    this->connections.erase(cc);
}

void
world::insert(entity *e)
{
    if (this->paused) {
        if (e->flag_active(ENTITY_DO_TICK)) {
            this->tickable.insert(e);
        }
    } else {
        if (e->flag_active(ENTITY_DO_STEP)) {
            this->stepable.insert(e);
        }
        if (e->flag_active(ENTITY_DO_MSTEP)) {
            this->mstepable.insert(e);
        }
        if (e->flag_active(ENTITY_DO_PRE_STEP)) {
            this->prestepable.insert(e);
        }
        if (e->flag_active(ENTITY_HAS_ACTIVATOR)) {
            this->activators.insert(e->get_activator());
        }
    }


    if (e->type == ENTITY_EDEVICE && e->get_edevice()->do_solve_electronics) {
        std::vector<edevice*>::iterator i;
        i = std::find(this->electronics.begin(), this->electronics.end(), e->get_edevice());
        if (i == this->electronics.end())
            this->electronics.push_back(e->get_edevice());
    }

    switch (e->g_id) {
        case O_EVENT_LISTENER: this->eventlisteners.insert((eventlistener*)e); break;
        case O_ESCRIPT:        this->escripts.insert((escript*)e); break;
        case O_KEY_LISTENER:   this->key_listeners.insert((key_listener*)e); break;
        case O_ARTIFICIAL_GRAVITY:   this->localgravities.insert((localgravity*)e); break;
        case O_REPAIR_STATION: this->repair_stations.insert(e); break;
    }

    e->curr_update_method = e->update_method;

    /* owned entities, such as the plugs of cables, must not be added to all_entities */
    if (e->flag_active(ENTITY_IS_OWNED))
        return;

    switch (e->type) {
        case ENTITY_CABLE: this->cables.insert((cable*)e); break;
        case ENTITY_GROUP: this->groups.insert(std::make_pair(e->id, (group*)e)); break;
        default: this->all_entities.insert(std::make_pair(e->id, e)); /*tms_infof("adding %u to all_entities", e->id);*/break;
    }
}

/**
 * insert and call add_to_world
 **/
void
world::add(entity *e)
{
    this->insert(e);

    if (!e->gr) {
        if (e->flag_active(ENTITY_STATE_SLEEPING)) {
            tms_debugf("ADDING SLEEPING ENTITY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            b2World::force_sleep = true;
            e->add_to_world();
            b2World::force_sleep = false;
        } else {
            e->add_to_world();
        }
    }

    if (this->paused) {
        this->step_count = 0;
    }
}

void
world::erase(entity *e)
{
    if (this->paused) {
        this->tickable.erase(e);
    } else {
        this->stepable.erase(e);
        this->prestepable.erase(e);
        this->mstepable.erase(e);
        this->timed_absorb.erase(e->id);
        this->activators.erase(e->get_activator());
    }

    switch (e->type) {
        case ENTITY_CABLE: this->cables.erase((cable*)e); break;
        case ENTITY_GROUP: this->groups.erase(e->id); break;
        default: this->all_entities.erase(e->id); break;
    }

    switch (e->g_id) {
        case O_CAM_MARKER:     this->cam_markers.erase(e->id); break;
        case O_EVENT_LISTENER: this->eventlisteners.erase((eventlistener*)e); break;
        case O_ESCRIPT:        this->escripts.erase((escript*)e); break;
        case O_KEY_LISTENER:   this->key_listeners.erase((key_listener*)e); break;
        case O_ARTIFICIAL_GRAVITY:   this->localgravities.erase((localgravity*)e); break;
        case O_REPAIR_STATION: this->repair_stations.erase(e); break;
    }

    if (e->get_edevice() && e->get_edevice()->do_solve_electronics) {
        std::vector<edevice*>::iterator i;
        i = std::find(this->electronics.begin(), this->electronics.end(), e->get_edevice());
        if (i == this->electronics.end()) {
            tms_warnf("edevice not found in electronics");
        } else {
            this->electronics.erase(i);
        }
    }
}

/**
 * Erase and call remove_from_world
 *
 * return true if we removed an emitted entity
 **/
bool
world::remove(entity *e)
{
    bool ret = true;

    this->erase(e);

    if (this->is_paused()) {

    } else {
        e->signal(ENTITY_EVENT_REMOVE);
        G->destroy_possible_mover(e);

        if (G->follow_object == e) {
            G->follow_object = 0;
        }
    }

    e->remove_from_world();

    return ret;
}

void
world::add_gravity_force(int key, b2Vec2 force)
{
    std::pair<std::map<int, b2Vec2>::iterator, bool> ret;
    ret = this->gravity_forces.insert(std::pair<int, b2Vec2>(key, force));

    if (ret.second) {
        tms_infof("new object inserted");
    } else {
        (ret.first)->second = force;
    }
}

void
world::remove_gravity_force(int key)
{
    this->gravity_forces.erase(key);
}

void
world::add_receiver(uint32_t frequency, receiver_base *t)
{
    this->receivers.insert(std::pair<uint32_t, receiver_base*>(frequency, t));
}

void
world::remove_receiver(uint32_t frequency, receiver_base *t)
{
    typedef std::multimap<uint32_t, receiver_base*>::iterator iterator;
    std::pair<iterator, iterator> ip = this->receivers.equal_range(frequency);

    iterator it = ip.first;
    for (; it != ip.second; ++it) {
        if (it->second == t) {
            this->receivers.erase(it);
            break;
        }
    }
}

void
world::add_soundman(uint32_t sound_id, soundman *sm)
{
    this->soundmanagers.insert(std::pair<uint32_t, soundman*>(sound_id, sm));
}

void
world::remove_soundman(uint32_t sound_id, soundman *sm)
{
    typedef std::multimap<uint32_t, soundman*>::iterator iterator;
    std::pair<iterator, iterator> ip = this->soundmanagers.equal_range(sound_id);

    iterator it = ip.first;
    for (; it != ip.second; ++it) {
        if (it->second == sm) {
            this->soundmanagers.erase(it);
            break;
        }
    }
}

//#define PROFILING
//
void
world::reload_modified_chunks()
{
    for (std::set<level_chunk*>::iterator i = this->to_be_reloaded.begin();
            i != this->to_be_reloaded.end(); i++) {
        level_chunk *c = (*i);
        int slot = c->slot;

        c->recreate_fixtures(false);

        if (slot >= 0) {
            this->cwindow->unload_slot(slot);
            this->cwindow->load_slot(slot, c);
        }
    }
    this->to_be_reloaded.clear();
}

/* return true if we have another step to do */
bool
world::step()
{
    if (this->is_playing()) {
        /*struct timeval start;
        gettimeofday(&start, 0);*/

        if (_tms.time_accum > WORLD_STEP*3) {
            _tms.time_accum = WORLD_STEP*3;
        }

        if (_tms.time_accum >= WORLD_STEP) {
            this->step_count ++;

            this->locked = false;

#ifdef PROFILING
            Uint32 ss = SDL_GetTicks();
#endif
            this->cwindow->step();

#ifndef SCREENSHOT_BUILD
            if (!this->level.flag_active(LVL_DISABLE_PHYSICS)) {
                this->b2->Step(((float)(WORLD_STEP+WORLD_STEP_SPEEDUP) * .000001f) * G->get_time_mul(),
                        this->level.velocity_iterations,
                        this->level.position_iterations);
            }
#endif

            //if (adventure::player) tms_infof("post step %f", adventure::player->get_position().x);
            //
            this->reload_modified_chunks();

            for (std::set<entity*>::iterator i = this->prestepable.begin();
                    i != this->prestepable.end(); i++) {
                (*i)->pre_step();
            }


            this->apply_local_gravities();

#ifdef PROFILING
            /*
            b2Profile prof = this->b2->GetProfile();
            tms_infof("xxx step %f collide %f solve %f sI %f sV %f sP %f broad %f solveTOI %f",
                    prof.step, prof.collide, prof.solve, prof.solveInit, prof.solveVelocity, prof.solvePosition, prof.broadphase, prof.solveTOI);
             */
            tms_infof("world: box2d step: %d", SDL_GetTicks() - ss);
            ss = SDL_GetTicks();
#endif

#ifndef SCREENSHOT_BUILD
            if (w_is_enabled()) {
                w_mstep_set = &this->mstepable;

                std::set<entity*>::iterator i = this->mstepable.begin();
                for (int x=0; x<w_get_num_workers() && i != this->mstepable.end(); x++) {
                    struct wdata_mstep data;
                    data.i = x;
                    w_run(W_RUN_MSTEP, &data);
                    i ++;
                }
            } else {
                for (std::set<entity*>::iterator i = this->mstepable.begin();
                        i != this->mstepable.end(); ++i) {
                    (*i)->mstep();
                }
            }

            this->perform_actions();

            if (w_is_enabled()) {
                w_wait(-1);
            }
# ifdef PROFILING
            tms_infof("world: mstep entities: %d", SDL_GetTicks() - ss);
            ss = SDL_GetTicks();
# endif

            for (std::set<entity*>::iterator i = this->stepable.begin();
                    i != this->stepable.end(); i++) {
                (*i)->step();
            }
# ifdef PROFILING
            tms_infof("world: step entities: %d", SDL_GetTicks() - ss);
            ss = SDL_GetTicks();
# endif

            for (std::set<eventlistener*>::iterator i = this->eventlisteners.begin();
                    i != this->eventlisteners.end(); i++) {
                if (this->events[(*i)->event_id]) {
                    (*i)->triggered ++;
                }
            }

            for (std::set<escript*>::iterator i = this->escripts.begin();
                    i != this->escripts.end(); i++) {
                for (int x=0; x<WORLD_EVENT__NUM; x++) {
                    if (this->events[x]) {
                        (*i)->events[x] ++;
                    }
                }
            }

            /* decrement event counters */
            for (int x=0; x<WORLD_EVENT__NUM; x++) {
                if (this->events[x] > 0) {
                    this->events[x] --;
                }
            }

            b2Vec2 gravity(0,0);

            if (this->gravity_forces.size() > 0) {
                for (std::map<int, b2Vec2>::iterator i = this->gravity_forces.begin();
                        i != this->gravity_forces.end(); ++i) {
                    gravity += i->second;
                    //tms_infof("new gravity:        %.2f/%.2f", gravity.x, gravity.y);
                }

                gravity *= 1.f / this->gravity_forces.size();
            } else {
                gravity = b2Vec2(this->gravity_x, this->gravity_y);
            }

            for (std::map<b2Joint*, float>::iterator i = this->destructable_joints.begin();
                    i != this->destructable_joints.end();) {
                float max_force = i->second;
                float force_module = i->first->GetReactionForce(1.f/(G->timemul(WORLD_STEP) * .000001)).Length() * G->get_time_mul();

                if (force_module > max_force) {
                    this->to_be_destroyed.insert(i->first);
                    this->destructable_joints.erase(i++);
                } else {
                    ++i;
                }
            }
# ifdef PROFILING
            tms_infof("world: joints/events/misc: %d", SDL_GetTicks() - ss);
            ss = SDL_GetTicks();
# endif

            if (gravity.x != this->last_gravity.x || gravity.y != this->last_gravity.y) {
                this->b2->SetGravity(gravity);
                this->b2->SetAllowSleeping(false);
                this->b2->SetAllowSleeping(true);
                this->last_gravity = gravity;
            }
# ifdef PROFILING
            tms_infof("world: apply gravity: %d", SDL_GetTicks() - ss);
            ss = SDL_GetTicks();
# endif

            this->emit_all();
            for (std::map<uint32_t, int64_t>::iterator it = this->timed_absorb.begin();
                    it != this->timed_absorb.end();) {
                if (it->second <= 0) {
                    entity *e = W->get_entity_by_id(it->first);
                    if (e) {
                        G->absorb(e);
                        this->timed_absorb.erase(it++);
                    } else {
                        it->second += WORLD_STEP * 60;
                    }
                } else {
                    it->second -= G->timemul(WORLD_STEP);
                    ++it;
                }
            }
            this->absorb_all();
            this->destroy_joints();
            for (std::set<entity*>::iterator it = this->post_interact.begin();
                    it != this->post_interact.end(); ++it) {
                G->interact_select(*it);
            }
            this->post_interact.clear();
# ifdef PROFILING
            tms_infof("world: emit/absorb: %d", SDL_GetTicks() - ss);
            ss = SDL_GetTicks();
# endif

#endif /* ifndef SCREENSHOT_BUILD */
            this->solve_electronics();
#ifdef PROFILING
            tms_infof("world: solve electronics: %d", SDL_GetTicks() - ss);
            ss = SDL_GetTicks();
#endif

            _tms.time_accum -= WORLD_STEP;

            return true;
        }

    } else {
        if (G->get_mode() != GAME_MODE_EDIT_PANEL && G->get_mode() != GAME_MODE_EDIT_GEARBOX
                && G->get_mode() != GAME_MODE_SELECT_SOCKET&&G->get_mode() != GAME_MODE_SELECT_CONN_TYPE) {

            this->cwindow->step();

#if 0
            if (this->step_count < 80) {
                this->b2->Step(.001f, 20, 20);
            }
#elif !defined(SINGLE_STEP_WORLD)
            this->b2->Step(.001f, 20, 20);
#endif

            for (std::set<entity*>::iterator i = this->tickable.begin();
                    i != this->tickable.end(); i++) {
                (*i)->tick();
            }

            this->perform_actions();

#if 0
            if (this->step_count < 20) {
                this->b2->Step(.001f, 20, 20);
            }
#elif !defined(SINGLE_STEP_WORLD)
            this->b2->Step(.001f, 20, 20);
#endif

            this->step_count ++;
        }

        this->reload_modified_chunks();
    }

    return false;
}

void
world::add_action(uint32_t entity_id, uint32_t action_id, void *data/*=0*/)
{
    struct entity_action ea = {
        entity_id,
        action_id,
        data
    };

    this->actions.push_back(ea);
}

void
world::perform_actions()
{
    if (!this->actions.empty()) {
        for (std::deque<struct entity_action>::iterator it = this->actions.begin();
                it != this->actions.end(); ++it) {
            struct entity_action &ea = *it;

            entity *e = W->get_entity_by_id(ea.entity_id);

            if (!e) continue;

            switch (ea.action_id) {
                case ACTION_FINALIZE_GROUP:
                    if (e->gr) {
                        e->gr->finalize();
                    }
                    break;

                case ACTION_REBUILD_GROUP:
                    if (e->gr) {
                        e->gr->rebuild();
                    }
                    break;

                case ACTION_MOVE_ENTITY:
                    {
                        b2Vec2 *pos = static_cast<b2Vec2*>(ea.data);
                        e->set_position(pos->x, pos->y);
                        delete pos;
                    }
                    break;

                case ACTION_SET_ANIMAL_TYPE:
                    {
                        ((animal*)e)->set_animal_type(VOID_TO_UINT32(ea.data));
                        ((animal*)e)->do_recreate_shape = true;
                    }
                    break;

                case ACTION_CALL_ON_LOAD:
                    {
                        e->on_load(false, false);
                    }
                    break;
            }
        }

        this->actions.clear();
    }
}

void
world::apply_local_gravities()
{
    /* step local gravities separately */
    for (std::set<localgravity*>::iterator i = this->localgravities.begin();
            i != this->localgravities.end(); i++) {
        (*i)->step();
    }
}

#define SOLVE_OK 0
#define SOLVE_ERR 1
#define SOLVE_SKIP 2

int
world::solve_edevice(edevice *e)
{
    edevice *next, *last_next = 0;

    if (e->step_count == edev_step_count) {
        return SOLVE_SKIP;
    }

    e->step_count = edev_step_count;

    while ((next = e->solve_electronics())) {
        if (solve_edevice(next) == SOLVE_ERR) {
            e->step_count = ~edev_step_count;
            return SOLVE_ERR;
        }

        if (next == last_next) {
            e->step_count = ~edev_step_count;
            return SOLVE_ERR;
        }

        last_next = next;
    }

    e->step_count = edev_step_count;

    if (this->electronics[this->edevice_order] != e) {
        std::vector<edevice*>::iterator it = std::find(this->electronics.begin(), this->electronics.end(), e);
        if (it == this->electronics.end()) {
            /* the edevice was not found in the vector at all, we assume it has already been removed
             * and fixed somewhere! */
            return SOLVE_OK;
        }

        this->electronics.erase(it);
        if (this->edevice_order > this->electronics.size()) {
            this->electronics.push_back(e);
        } else {
            this->electronics.insert(this->electronics.begin()+this->edevice_order, e);
        }
    }

    this->edevice_order ++;

    return SOLVE_OK;
}

void
world::solve_electronics()
{
//#define PROFILING

#ifdef PROFILING
    Uint32 ss = SDL_GetTicks();
#endif

    this->electronics_accum += G->timemul(WORLD_STEP);

    if (this->electronics_accum >= 8000) {
        this->edevice_order = 0;
        edev_step_count = this->step_count;

        int retry_count = 0;

        while (this->edevice_order < this->electronics.size()) {
            //if ((*i)->step_count != this->step_count)
            edevice *e = this->electronics[this->edevice_order];

            int status = this->solve_edevice(e);
            if (status == SOLVE_ERR) {
                retry_count ++;
                if (retry_count == 2) {
                    G->add_error(e->get_entity(), ERROR_SOLVE);
                    //break;
                    this->edevice_order ++;
                    retry_count = 0;
                }
            } else if (status == SOLVE_SKIP) {
                this->edevice_order ++;
            } else if (status == SOLVE_OK) {
                retry_count = 0;
                continue;
            }
        }

        this->electronics_accum = 0;
        if (sm::gen_started) {
            sm::write_counter++;
        }
    }

#ifdef PROFILING
    tms_infof("solve electronics: %d", SDL_GetTicks() - ss);
#endif
}

int
world::get_layer_point(tms::camera *cam, int x, int y, float layer, tvec3 *out)
{
    float invcam[16];

    tvec2 v = {
        -1.f + (((float)x)/cam->width)*2.f,
        -1.f + (((float)y)/cam->height)*2.f,
    };

    //cam->calculate();
    tmat4_copy(invcam, cam->combined);

    if (tmat4_invert(invcam)) {
        tvec3 vp1 = {v.x, v.y, -1.f};
        tvec3 vp2 = {v.x, v.y, 1.f};

        if (cam->_flags & TMS_CAMERA_PERSPECTIVE) {
            tvec3_project_mat4(&vp1, invcam);
            tvec3_project_mat4(&vp2, invcam);

            vp1.x -= vp2.x;
            vp1.y -= vp2.y;
            vp1.z -= vp2.z;

            tvec4 plane = {0.f, 0.f, -1.f, layer * LAYER_DEPTH};
            tintersect_ray_plane(&vp2, &vp1, &plane, out);
        } else {
            vp2 = tms_camera_unproject(cam, x, y, 0.f);
            *out = vp2;
        }
    }

    return T_OK;
}

bool
world::ReportFixture(b2Fixture *f)
{
    entity *e = (entity*)f->GetUserData();

    if (this->is_playing() && e && this->level.type != LCAT_ADVENTURE) {
        if (!f->IsSensor() && e->is_control_panel()) {
            if (f->TestPoint(this->query_point)) {
                this->query_exact = true;
                this->query_nearest_b = f->GetBody();
                this->query_nearest = e;
                this->query_nearest_fx = f;
                return false;
            }
        }
    }

    if (f->GetFilterData().categoryBits & (15 << (this->query_layer*4))) {
        if (this->is_paused() && e && e->g_id == O_CHUNK) {
            return true;
        }

        if (!this->is_paused() && e && e->g_id == O_CURSOR_FIELD) {
            if (f->TestPoint(this->query_point)) {
                this->query_exact = true;
                this->query_nearest_b = f->GetBody();
                this->query_nearest = e;
                this->query_nearest_fx = f;
                return false;
            }
        }


        if (e && (!f->IsSensor() || this->is_paused() || IS_FACTORY(e->g_id))) {

            if (this->is_paused() && !G->state.sandbox && !e->get_property_entity()->is_moveable() && !this->query_force) {
                return true;
            }

            if (this->level.type == LCAT_ADVENTURE && !this->is_paused()) {
                if (adventure::player) {
                    if (world::fixture_in_layer(f, 2)) {
                        if ((adventure::player->get_position() - this->query_point).Length() < G->caveview_size) {
                            return true;
                        }
                    }

                    if (adventure::player->get_tool() && adventure::player->get_tool()->get_arm_type() == TOOL_BUILDER) {
                        if ((e->g_id == O_TPIXEL || e->g_id == O_CHUNK)) {
                            return true;
                        }
                    }

                }
            }

            /* test the point if it is exactly inside the shape */
            if (f->TestPoint(this->query_point)) {
                if ((e->flag_active(ENTITY_IS_LOW_PRIO) && (this->query_nearest && !this->query_nearest->flag_active(ENTITY_IS_LOW_PRIO)))
                    || (this->query_nearest && this->query_nearest->is_high_prio() && !e->is_high_prio())) {
                } else {
                    this->query_exact = true;
                    this->query_nearest_b = f->GetBody();
                    this->query_nearest = e;
                    this->query_nearest_fx = f;

                    if (e->flag_active(ENTITY_IS_COMPOSABLE) || this->query_nearest_b == this->ground) {
                        this->query_offs = this->query_point - e->get_position();//f->GetBody()->GetPosition();
                    } else {
                        this->query_offs = this->query_point - f->GetBody()->GetPosition();
                    }

                    //this->query_dist = this->query_offs.Length();
                    this->query_dist = 0.f;
                }
            } else {
                float dist = INFINITY;

                switch (f->GetType()) {
                    case b2Shape::e_circle:
                        {
                        b2Vec2 p = f->GetBody()->GetWorldPoint(((b2CircleShape*)f->GetShape())->m_p);
                        p = this->query_point - p;
                        dist = p.Length() - ((b2CircleShape*)f->GetShape())->m_radius;
                        }
                        break;

                    case b2Shape::e_polygon:
                        {
                        b2Vec2 *ov = (((b2PolygonShape*)f->GetShape())->m_vertices);
                        int num_verts = ((b2PolygonShape*)f->GetShape())->m_count;

                        tvec2 verts[num_verts];
                        for (int x=0; x<num_verts; x++) {
                            b2Vec2 tv = f->GetBody()->GetWorldPoint(ov[x]);
                            verts[x].x = tv.x;
                            verts[x].y = tv.y;
                        }

                        dist = tintersect_point_poly_distance((tvec2*)&(this->query_point), verts, num_verts);
                        }
                        break;

                    default: break;
                }

                if (dist < 0.f) {
                    dist = 0.f;
                }

                if ((dist < this->query_dist || (this->query_nearest && this->query_nearest->flag_active(ENTITY_IS_LOW_PRIO))) && dist < QUERY_EPS) {
                    this->query_nearest_b = f->GetBody();
                    this->query_nearest = e;
                    this->query_nearest_fx = f;
                    if (e->flag_active(ENTITY_IS_COMPOSABLE) || this->query_nearest_b == this->ground) {
                        this->query_offs = this->query_point - e->get_position();//f->GetBody()->GetPosition();
                    } else {
                        this->query_offs = this->query_point - f->GetBody()->GetPosition();
                    }
                    this->query_dist = dist;
                }
            }
        }
    }

    return true;
}

void
world::explode(entity *source, b2Vec2 pos, int layer,
               int num_rays, float force,
               float damage_multiplier, float dist_multiplier)
{
    class _cb : public b2RayCastCallback {
      public:
        entity    *source;
        int        layer;
        b2Fixture *result;
        b2Vec2     result_pt;
        _cb(entity *e, int layer){this->source = e; this->layer = layer;};
        float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
        {
            entity *e = static_cast<entity*>(f->GetUserData());

            if (f->IsSensor()) {
                return -1;
            }

            if (e) {
                if (e == this->source) {
                    return -1.f;
                }
                if (this->source) {
                    if (this->layer != this->source->get_layer()) {
                        return -1;
                    }

                    if (!world::fixture_in_layer(f, this->layer)) {
                        if (W->level.flag_active(LVL_SINGLE_LAYER_EXPLOSIONS)
                                || e->g_id == O_CHUNK
                                || e->g_id == O_TPIXEL) {
                            return -1;
                        }
                    }
                }

                if (e->g_id == O_TPIXEL) {
                    if ((e->interactive_hp < 0.f && e->properties[0].v.i8 == 0) || e->get_layer() != this->layer) {
                        return -1;
                    }
                }
            }

            this->result = f;
            this->result_pt = pt;

            return fraction;
        };
    } cb(source, layer);

    for (int x=0; x<num_rays; x++) {
        cb.result = 0;

        float a = ((M_PI*2.f)/(float)num_rays) * x;
        b2Vec2 r;
        tmath_sincos(a, &r.y, &r.x);

        b2Vec2 rr = r;
        rr.x *= RAY_LENGTH;
        rr.y *= RAY_LENGTH;

        b2Vec2 prr = pos+rr;

        this->raycast(&cb, pos, prr, 1.f, 1.f, 0.f, 1000.f);

        if (cb.result) {
            float dist = (cb.result_pt - pos).Length();

            dist *= dist_multiplier;

            b2Body *b = cb.result->GetBody();

            if (W->level.version >= LEVEL_VERSION_1_5) {
                if (dist < 1.0f) {
                    dist = 1.0f;
                }
            } else {
                if (dist < 0.001f) {
                    dist = 0.001f;
                }
            }

            r.x *= force * fminf(1.f, 1.f/dist) / G->get_time_mul();
            r.y *= force * fminf(1.f, 1.f/dist) / G->get_time_mul();

            float rlen = r.Length();

            if (cb.result->GetUserData()) {
                entity *e = (entity*)cb.result->GetUserData();
                float damage;

                if (e->is_creature()) {
                    creature *c = static_cast<creature*>(e);
                    damage = (rlen*.05f) * damage_multiplier;

                    if (W->level.version >= LEVEL_VERSION_1_5) {
                        c->shock_forces += rlen;
                    }

                    c->damage(damage, cb.result, DAMAGE_TYPE_FORCE, DAMAGE_SOURCE_WORLD, 0);
                } else if ((e->is_interactive()
                     || e->g_id == O_CHUNK
                     || e->g_id == O_TPIXEL
                     || e->g_id == O_PLANT
                     ) && this->level.flag_active(LVL_ENABLE_INTERACTIVE_DESTRUCTION)) {
                    damage = (2.f/dist) * damage_multiplier;
                    G->damage_interactive(e, cb.result, cb.result->GetUserData2(), damage, cb.result_pt, DAMAGE_TYPE_FORCE);
                } else if (e->g_id == O_PRESSURE_SENSOR || e->g_id == O_IMPACT_SENSOR) {
                    ((impact_sensor*)e)->impulse += r.Length() * 10.f/(WORLD_STEP/1000000.f);
                }
            }

            if (b->GetType() == b2_dynamicBody && (
                        !cb.result->GetUserData()
                        || ((entity*)cb.result->GetUserData())->get_layer() == source->get_layer()
                        )) {
                b->ApplyForce(r, cb.result_pt);
            }
        }
    }
}

int
world::query(tms::camera *cam, int x, int y,
        entity **out_ent, b2Body **out_body,
        tvec2 *offs, uint8_t *frame, int layer_mask,
        bool force_selection/*=false*/,
        b2Fixture **out_fx/*=0*/,
        bool is_exact/*=false*/)
{
    tvec3 p;

    this->query_exact = false;
    this->query_frame = 0;
    this->query_nearest = 0;
    this->query_nearest_b = 0;
    this->query_nearest_fx = 0;
    this->query_offs = b2Vec2(0.f,0.f);
    this->query_dist = INFINITY;
    this->query_force = force_selection;

    for (int z=2; z>=0; z--) {
        if ((1 << z) & layer_mask) {
            this->get_layer_point(cam, x, y, z, &p);

            this->query_point = b2Vec2(p.x, p.y);
            this->query_layer = z;

            //tms_infof("layer point %f %f %f", p.x, p.y, p.z);

            b2AABB aabb;
            aabb.lowerBound.Set(p.x-QUERY_EPS/2.f, p.y-QUERY_EPS/2.f);
            aabb.upperBound.Set(p.x+QUERY_EPS/2.f, p.y+QUERY_EPS/2.f);

            this->b2->QueryAABB(this, aabb);

            //tms_infof("query offs %f %f, layer %d, frame %u, body %p", this->query_offs.x, this->query_offs.y, z, (uint8_t)(this->query_nearest_b ? this->query_nearest_b->GetUserData() : 0), this->query_nearest_b);

            if (this->query_nearest && this->query_exact) {
                break;
            }
        }
    }

    if (is_exact) {
        this->query_nearest = (this->query_exact ? this->query_nearest : 0);
        this->query_nearest_b = (this->query_exact ? this->query_nearest_b : 0);
    }

    *offs = (tvec2){this->query_offs.x, this->query_offs.y};
    *out_body = this->query_nearest_b;
    *out_ent = this->query_nearest;
    *frame = VOID_TO_UINT8((this->query_nearest_b ? this->query_nearest_b->GetUserData() : 0));

    if (out_fx) *out_fx = this->query_nearest_fx;

    return 1;
}

void
world::init(bool paused)
{
    if (paused) {
        tms_infof("world init PAUSE");
        this->paused = true;

        this->electronics_accum = 0;
        this->step_count = 1000;
    } else {
        tms_infof("world init PLAY");
        this->paused = false;

        memset(this->events, 0, sizeof(this->events));
        this->electronics_accum = 0;

        _tms.time_accum = 0;

        edev_step_count = this->step_count = 0;

        this->last_gravity = b2Vec2(0.f, 0.f);
        this->gravity_x = 0.f;
        this->gravity_y = 0.f;
    }
}

void
world::init_simulation(void)
{
    bool INGAME_ALLOW_SLEEP = true;
    bool PAUSE_ALLOW_SLEEP = false;

    this->b2->SetContinuousPhysics(false);
    this->b2->SetAutoClearForces(true);

    if (this->is_paused()) {
        this->b2->SetContactListener(this->contacts_paused);
        this->b2->SetAllowSleeping(PAUSE_ALLOW_SLEEP);

        this->b2->SetGravity(b2Vec2(0.f, 0.f));
    } else {
        this->gravity_x = this->level.gravity_x;
        this->gravity_y = this->level.gravity_y;

        this->b2->SetContactListener(this->contacts_playing);
        this->b2->SetAllowSleeping(INGAME_ALLOW_SLEEP);

        this->b2->m_min_linear_damping = this->level.linear_damping;
        this->b2->m_min_angular_damping = this->level.angular_damping;

        this->b2->SetGravity(b2Vec2(this->gravity_x, this->gravity_y));

    }
}

void
world::absorb_all(void)
{
    if (!this->to_be_absorbed.empty()) {
        std::set<cable*> r_cables;
        std::set<group*> r_groups;
        std::set<connection*> r_conns;
        std::set<entity *> r_free;

        while (!this->to_be_absorbed.empty()) {
            fadeout_event *ev = new fadeout_event();
            ev->time = 1.f;

            for (std::set<pending_absorb>::iterator i = this->to_be_absorbed.begin();
                    i != this->to_be_absorbed.end();) {
                pending_absorb a = (*i);
                entity *e = a.e;

                if (a.absorber) {
                    if (!ev->entities.empty()) {
                        break;
                    }

                    ev->absorber = a.absorber;
                    ev->absorber_point = a.absorber_point;
                    ev->absorber_frame = a.absorber_frame;
                }

                if (e == G->follow_object) {
                    tms_debugf("follow_object was just absorbed, reset follow object.");
                    G->set_follow_object(0, false);
                }

                bool is_culled = false;

                if (e->scene) {
                    is_culled = tms_graph_is_entity_culled(G->graph, e);
                }

                if (G->current_panel == e) {
                    if (this->is_adventure() && adventure::player)  {
                        G->set_control_panel(adventure::player);
                    } else {
                        G->set_control_panel(0);
                    }
                }

                if (this->is_adventure() || this->is_custom()) {
                    if (e->g_id == O_OPEN_PIVOT) {
                        this->erase_connection(&((pivot_1*)e)->dconn);
                    } else if (e->g_id == O_DAMPER) {
                        this->erase_connection(&((damper_1*)e)->dconn);
                    } else if (e->g_id == O_RUBBERBAND) {
                        this->erase_connection(&((rubberband_1*)e)->dconn);
                    } else if (e->g_id == O_CRANE) {
                        this->erase_connection(&((crane*)e)->pc);
                        this->erase_connection(&((crane*)e)->rc);
                    }
                }

                e->on_absorb();

                //
                //
                //
                //
                if (e->type == ENTITY_CABLE) {
                    cable *c = ((cable*)e);
                    G->remove_entity(e);

                    c->freeze = true;
                    c->disconnect(c->p[0]);
                    c->disconnect(c->p[1]);
                    c->destroy_joint();
                    this->remove(e);
                    delete e;
                    this->to_be_absorbed.erase(i++);
                    continue;
                }

                fadeout_entity fe;
                fe.e = e;
                fe.velocity = (e->get_body(0) ? .45f * e->get_body(0)->GetLinearVelocity()  : b2Vec2(0.f, 0.f));

                if (e->flag_active(ENTITY_FADE_ON_ABSORB) && !is_culled) {
                    e->prepare_fadeout();
                }

                if (e->gr) r_groups.insert(e->gr);
                edevice *ed = e->get_edevice();
                if (ed) {
                    for (int x=0; x<ed->num_s_in; x++) {
                        if (ed->s_in[x].p && ed->s_in[x].p->c)
                            r_cables.insert(ed->s_in[x].p->c);
                    }
                    for (int x=0; x<ed->num_s_out; x++) {
                        if (ed->s_out[x].p && ed->s_out[x].p->c)
                            r_cables.insert(ed->s_out[x].p->c);
                    }
                }

                G->remove_entity(e);
                this->remove(e);

                connection *c = e->conn_ll;
                while (c) {
                    connection *next = c->get_next(e);

                    if (c->owned && c->e == e) {
                        this->erase_connection(c);
                        c->o->remove_connection(c);
                    } else if (!c->owned) {
                        r_conns.insert(c);
                    } else {
                        /* owned but not by us */
                        this->erase_connection(c);
                        c->e->remove_connection(c);
                    }
                    c = next;
                }

                if (!(e->flag_active(ENTITY_FADE_ON_ABSORB))) {
                    r_free.insert(e);
                }

                fe.do_free = true;

                if (e->flag_active(ENTITY_FADE_ON_ABSORB) && !is_culled) {
                    ev->entities.push_back(fe);
                }

                this->to_be_absorbed.erase(i++);

                if (a.absorber) {
                    break;
                }
            }

            G->fadeouts.insert(ev);
        }

        /* clean up leftover cables and stuff */
        for (std::set<cable*>::iterator i = r_cables.begin(); i != r_cables.end(); i++) {
            (*i)->joint = 0;
            (*i)->freeze = true;

            (*i)->disconnect((*i)->p[0]);
            (*i)->disconnect((*i)->p[1]);

            G->remove_entity(*i);
            this->remove(*i);
            delete (*i);
        }

        for (std::set<group*>::iterator i = r_groups.begin(); i != r_groups.end(); i++) {
            G->remove_entity(*i);
            this->remove(*i);
            delete (*i);
        }

        for (std::set<connection*>::iterator i = r_conns.begin(); i != r_conns.end(); i++) {
            (*i)->o->remove_connection((*i));
            (*i)->e->remove_connection((*i));
            this->erase_connection((*i));
            delete *i;
        }

        for (std::set<entity*>::iterator i = r_free.begin(); i != r_free.end(); i++) {
            delete *i;
        }

        if (G->force_static_update == 2) {
            G->force_static_update = 1;
        }
    }
}

void
world::emit_all()
{
    for (std::vector<pending_emit>::iterator i = this->to_be_emitted.begin();
            i != this->to_be_emitted.end(); i++) {

        pending_emit ee = (*i);

        if (ee.partial) {
            tms_infof("emitting partial from buffer");
            /* create a wrapper lvlbuf for the buf and len */
            lvlbuf dummy;
            dummy.buf = (uint8_t*)ee.data.multi.buf;
            dummy.size = ee.data.multi.buf_len;

            std::map<uint32_t, entity*> entities;
            std::map<uint32_t, group*> groups;
            std::set<connection*> connections;
            std::set<cable*> cables;

            b2Vec2 displacement = b2Vec2(ee.data.multi.displacement_x, ee.data.multi.displacement_y);
            this->load_partial_from_buffer(&dummy, displacement, &entities, &groups, &connections, &cables);
            this->init_level_entities(&entities);

            /* add the entities to game as well */
            G->add_entities(&entities, &groups, &connections, &cables);

            dummy.buf = 0; /* prevent destructor from freeing it */
            dummy.size = 0;
        } else {
            if (ee.data.single.emitter) {
                ee.data.single.e->emitted_by = ee.data.single.emitter->id;
            }

            ee.data.single.e->emit_step = this->step_count;

            ee.data.single.e->on_load(false, false);
            this->add(ee.data.single.e);
            ee.data.single.e->init();
            ee.data.single.e->setup();
            ee.data.single.e->on_entity_play();

            b2Body *b = ee.data.single.e->get_body(0);
            if (b) {
                b->SetLinearVelocity(b2Vec2(ee.data.single.velocity_x, ee.data.single.velocity_y));
            }

            G->add_entity(ee.data.single.e);
        }
    }

    this->to_be_emitted = this->post_to_be_emitted;

    this->post_to_be_emitted.clear();
}

void
world::destroy_connection_joint(connection *c)
{
    /* TODO: make sure we allow destroying the joint */
    if (c->j) {
        this->to_be_destroyed.insert(c->j);
    }
}

void
world::destroy_joints(void)
{
    if (this->to_be_destroyed.empty()) {
        return;
    }

    for (std::set<b2Joint*>::iterator i = this->to_be_destroyed.begin();
            i != this->to_be_destroyed.end(); i++) {
        b2Joint *j = *i;

        //G->remove_entity(*i);
        //this->remove(*i);

        tms_debugf("destroying joint %p", j);

        bool remove_joint = true;

        joint_info *ji = static_cast<joint_info*>(j->GetUserData());
        if (ji) {
            switch (ji->type) {
                case JOINT_TYPE_CONN:
                    {
                        connection *c = static_cast<connection*>(ji->data);

                        if (c) {
                            c->destroyed = true;
                            G->emit(new break_effect(c->e->local_to_world(c->p, c->f[0]), c->layer), 0);
                            c->e->destroy_connection(c);
                            this->destructable_joints.erase(j);

                            remove_joint = false;
                        }
                    }
                    break;

                case JOINT_TYPE_SCUP:
                    {
                        tms_debugf("Destroying suction cup joint");
                        scup *e = static_cast<scup*>(ji->data);
                        for (int n=0; n<SCUP_NUM_JOINTS; ++n) {
                            if (j == e->j[n]) {
                                e->j[n] = 0;
                            }
                        }

                        e->stuck = false;
                    }
                    break;

                case JOINT_TYPE_RAGDOLL:
                    {
                        ragdoll *r = static_cast<ragdoll*>(ji->data);
                        for (int x=0; x<9; x++) {
                            if (r->joints[x] == j) {
                                r->joints[x] = 0;
                            }
                        }
                    }
                    break;

                default:
                    tms_debugf("Destroyed unhandled joint %d", ji->type);
                    break;
            }
        }

        if (remove_joint) {
            // If the joint has not already been removed in the above code.
            this->destructable_joints.erase(j);
            this->b2->DestroyJoint(j);
        }
    }

    this->to_be_destroyed.clear();
}

void
world::reset()
{
    of::_id = 1;

    this->locked = false;

    this->gravity_forces.clear();
    this->gravity_x = 0;
    this->gravity_y = 0;
    this->last_gravity = b2Vec2(0.f, 0.f);

    this->cwindow->reset();
    this->cwindow->preloader.reset();
    this->timed_absorb.clear();

    this->to_be_absorbed.clear();

    this->level_variables.clear();
    this->b2->SetContactListener(0);

    for (std::set<connection *>::iterator i = this->connections.begin();
            i != this->connections.end(); ++i) {
        connection *c = *i;

        if (!c->owned) {
            delete c;
        }
    }

    for (std::set<cable*>::iterator i = this->cables.begin();
            i != this->cables.end(); i++) {
        (*i)->remove_from_world();
        delete *i;
    }

    for (std::map<uint32_t, entity*>::iterator i = this->all_entities.begin();
            i != this->all_entities.end(); i++) {
        i->second->remove_from_world();
        delete i->second;
    }

    for (std::map<uint32_t, group*>::iterator i = this->groups.begin();
            i != this->groups.end(); i++) {
        delete i->second;
    }

    /* force-clear the world of bodies
     * this will implicitly destroy all joints and shapes as well */
    {
        b2Body *next;
        for (b2Body *b = this->b2->GetBodyList(); b; b = next) {
            next = b->GetNext();
            this->b2->DestroyBody(b);
        }
    }

    {
        /* destroy all particles */
        b2ParticleGroup *next;
        for (b2ParticleGroup *g = this->b2->GetParticleGroupList(); g; g = next) {
            next = g->GetNext();
            this->b2->DestroyParticlesInGroup(g);
        }
    }

    this->b2->Step(1.f, 1, 1); /* step the world once, to make sure fluid particles are cleaned up */

    this->ground = 0;
    for (int x=0; x<4; x++) ground_fx[x] = 0;

    this->all_entities.clear();
    this->groups.clear();
    this->cam_markers.clear();
    this->eventlisteners.clear();
    this->key_listeners.clear();
    this->localgravities.clear();
    this->repair_stations.clear();
    this->escripts.clear();
    this->tickable.clear();
    this->stepable.clear();
    this->prestepable.clear();
    this->mstepable.clear();
    this->electronics.clear();
    this->activators.clear();
    this->cables.clear();
    this->connections.clear();
}

float
world::get_height(float x)
{
    if (this->level.seed) {
        return this->cwindow->get_height(x)+1.f;
    } else
        return -this->level.size_y[0];
}

void
world::create(int type, uint64_t seed, bool play)
{
    of::_id = 1;
    this->level_id_type = LEVEL_LOCAL;
    this->reset();
    this->init(!play);
    this->level.create(type, seed);
    G->init_background();
    this->init_level();

    if (this->level.type == LCAT_ADVENTURE) {
        entity *e = of::create_with_id(O_ROBOT, this->level.get_adventure_id());
        e->_pos.x = 0.f;
        e->_pos.y = this->get_height(0)+1.5f;
        e->_angle = 0.f;
        e->prio = 0;
        this->level.sandbox_cam_y = e->_pos.y;
        ((robot_base*)e)->set_faction(FACTION_FRIENDLY);
        e->on_load(true, false);
        this->add(e);
        adventure::player = static_cast<creature*>(e);

        if (seed && play) {
            /* start new adventure */
            creature *r = static_cast<creature*>(e);
            r->set_equipment(EQUIPMENT_BACK, BACK_EQUIPMENT_NULL);
            r->set_equipment(EQUIPMENT_FRONT, FRONT_EQUIPMENT_NULL);

            repair_station *s = static_cast<repair_station*>(of::create(O_REPAIR_STATION));
            s->_pos.x = -5.f;
            s->_pos.y = e->_pos.y + 2.f;
            s->on_load(true, false);
            this->add(s);

            factory *f = static_cast<factory*>(of::create(O_FACTORY));
            f->_pos.x = +5.f;
            f->_pos.y = e->_pos.y + 2.f;
            f->on_load(true, false);
            this->add(f);
        }
    }
}

void
world::set_level_type(int type)
{
    int previous_type = this->level.type;

    if (type != previous_type) {
        this->level.type = type;

        if (type == LCAT_ADVENTURE) {
            entity *e = this->get_entity_by_id(this->level.get_adventure_id());
            if (!e) {
                /* No entity had the ID `adventure id`, create him */
                e = of::create_with_id(O_ROBOT, this->level.get_adventure_id());
                e->_pos.x = 0.f;
                e->_pos.y = this->get_height(0)+1.5f;
                e->_angle = 0.f;
                e->prio = 0;
                ((robot_base*)e)->set_faction(FACTION_FRIENDLY);
                e->on_load(true, false);
                this->add(e);
                adventure::player = static_cast<robot_base*>(e);
                G->add_entity(e);
            }
        }
    }
}

void
world::init_level(bool soft)
{
    tms_debugf("init level (soft=%s)", soft?"true":"false");
    /* create the level borders */
    float w = (float)this->level.size_x[0]+(float)this->level.size_x[1];
    float h = (float)this->level.size_y[0]+(float)this->level.size_y[1];

    this->init_simulation();

    if (!soft || !this->ground) {
        if (this->ground) {
            this->b2->DestroyBody(this->ground);
            tms_infof("destroying ground");
        }
        this->ground_fx[0] = 0;
        this->ground_fx[1] = 0;
        this->ground_fx[2] = 0;
        this->ground_fx[3] = 0;

        b2BodyDef bd;
        bd.type = b2_staticBody;
        bd.position = b2Vec2(0.f, 0.f);
        bd.angle = 0.f;
        this->ground = this->b2->CreateBody(&bd);
    }

    if (w >= 5 && h >= 5) {
        b2PolygonShape e;
        b2FixtureDef fd;
        fd.filter.categoryBits = ~0;
        fd.filter.maskBits = ~0;
        fd.filter.groupIndex = -1;
        fd.shape = &e;
        fd.restitution = .7f;
        fd.friction = .7f;
        fd.density = 1.f;

        float px = (float)this->level.size_x[1] / 2.f - (float)this->level.size_x[0]/2.f;
        float py = (float)this->level.size_y[1] / 2.f - (float)this->level.size_y[0]/2.f;

        float ww = 1.f;

        if (this->level.version >= LEVEL_VERSION_1_4 || G->state.pkg != 0) {
            ww = 10.f;
        }

        float offs = ww == 1.f ? 0.f : ww/2.f-.5f;

        b2Vec2 pos[4] = {
            b2Vec2(this->level.size_x[1] + offs, py),
            b2Vec2(px, this->level.size_y[1] + offs),
            b2Vec2(-this->level.size_x[0] - offs, py),
            b2Vec2(px, -this->level.size_y[0] - offs)
        };

        b2Vec2 size[4] = {
            b2Vec2(ww, h),
            b2Vec2(w, ww),
            b2Vec2(ww, h),
            b2Vec2(w, ww),
        };

        for (int x=0; x<4; x++) {
            if ((material_factory::background_id != BG_SPACE && material_factory::background_id != BG_COLORED_SPACE && (!((simplebg*)G->bgent)->bottom_only || x == 3)) || this->level.version < LEVEL_VERSION_1_3_0_3) {
                e.SetAsBox(size[x].x/2.f, size[x].y/2.f, pos[x], 0.f);

                if (soft && ground_fx[x]) {
                    ((b2PolygonShape*)(ground_fx[x]->GetShape()))->Set(e.m_vertices, 4);
                } else {
                    tms_debugf("creating ground_fx[%d]", x);
                    ground_fx[x] = ground->CreateFixture(&fd);
                }
            } else {
                if (soft && ground_fx[x]) {
                    ground->DestroyFixture(ground_fx[x]);
                }
                ground_fx[x] = 0;
            }
        }
    } else {
        if (soft) {
            for (int x=0; x<4; x++) {
                if (ground_fx[x]) {
                    ground->DestroyFixture(ground_fx[x]);
                    ground_fx[x] = 0;
                }
            }
        }
    }

    this->cwindow->set_seed(this->level.seed);
}

/**
 * - Make dynamic bodies connected to static bodies static, if the max force is inf
 * - Remove joints between static entities
 **/
void
world::optimize_connections()
{
    for (std::set<connection*>::iterator i = this->connections.begin();
            i != this->connections.end(); i++) {
        connection *c = (*i);
        if (c->j) {
            b2Joint *j = c->j;

            /* let group::finalize() handle group cases */
            if (c->e->gr != 0 || c->o->gr != 0) continue;

            if ((c->type == CONN_GROUP || c->type == CONN_WELD || c->type == CONN_PLATE) && c->max_force == INFINITY
                    && c->e->get_body(c->f[0]) && c->o->get_body(c->f[1])) {
                if (!c->e->flag_active(ENTITY_MUST_BE_DYNAMIC) && c->e->get_body(c->f[0])->GetType() == b2_dynamicBody && c->o->get_body(c->f[1])->GetType() == b2_staticBody) {
                    c->e->get_body(c->f[0])->SetType(b2_staticBody);
                } else if (!c->o->flag_active(ENTITY_MUST_BE_DYNAMIC) && c->e->get_body(c->f[0])->GetType() == b2_staticBody && c->o->get_body(c->f[1])->GetType() == b2_dynamicBody) {
                    c->o->get_body(c->f[1])->SetType(b2_staticBody);
                }
            }
        }
    }

    for (std::set<connection*>::iterator i = this->connections.begin();
            i != this->connections.end(); i++) {
        connection *c = (*i);
        if (c->j) {
            b2Joint *j = c->j;
            if (c->e->get_body(c->f[0]) && c->o->get_body(c->f[1]) && c->e->get_body(c->f[0])->GetType() == b2_staticBody && c->o->get_body(c->f[1])->GetType() == b2_staticBody) {
                this->b2->DestroyJoint(j);
                c->j = 0;
            }
        }
    }
}

void
world::init_level_entities(std::map<uint32_t, entity*> *entities, std::map<uint32_t, group*> *groups)
{
    if (entities == 0) entities = &this->all_entities;

    /* loop through all entities and run setup or on_pause depending on state */
    if (this->is_paused()) {
        for (std::map<uint32_t, entity*>::iterator i = entities->begin();
                i != entities->end(); i++) {
            i->second->on_pause();
        }
    } else {
        for (std::map<uint32_t, entity*>::iterator i = entities->begin();
                i != entities->end(); i++) {

            i->second->init();

            if (i->second->get_edevice()) {
                i->second->get_edevice()->begin();
            }

            if (i->second->state_size) {
                i->second->restore();
            } else {
                i->second->setup();
            }

            /* XXX TODO do we need this one for state shit */
            i->second->on_entity_play();
        }

        if (groups) {
            for (std::map<uint32_t, group*>::iterator i = groups->begin();
                    i != groups->end(); i++) {
                if (i->second->state_size) {
                    i->second->restore();
                }
            }
        }
    }
}

bool
world::load_buffer(lvlinfo *lvl, lvlbuf *buf, uint32_t id_modifier, b2Vec2 displacement, std::map<uint32_t, entity*> *entities, std::map<uint32_t, group*> *groups, std::set<connection*> *connections, std::set<cable*> *cables)
{
    /* XXX keep in sync with chunk_preloader::preload() */
    uint32_t num_entities, num_groups, num_connections, num_cables, n_read = 0, num_chunks = 0, num_gentypes = 0;

    num_groups = lvl->num_groups;
    num_entities = lvl->num_entities;
    num_connections = lvl->num_connections;
    num_cables = lvl->num_cables;

    tms_infof("load buffer[%p]: (id mod: %u, displ: %f %f)", buf, id_modifier, displacement.x, displacement.y);
    tms_infof("num groups %d, num entities %d, num connections %d, num_cables %d", num_groups, num_entities, num_connections, num_cables);

    for (n_read = 0; (!buf->eof() && n_read < num_groups); n_read++) {
        this->load_group(buf, lvl->version, id_modifier, displacement, groups);
    }

    for (n_read = 0; (!buf->eof() && n_read < num_entities); n_read++) {
        entity *e = this->load_entity(buf, lvl->version, id_modifier, displacement, entities);

        if (!e) {
            tms_errorf("error while reading entities from level file");
            this->reset();
            return false;
        }
    }

    for (n_read = 0; (!buf->eof() && n_read < num_cables); n_read++) {
        this->load_cable(buf, lvl->version, lvl->flags, id_modifier, displacement, cables);
    }

    for (n_read = 0; (!buf->eof() && n_read < num_connections); n_read++) {
        this->load_connection(buf, lvl->version, lvl->flags, id_modifier, displacement, connections);
    }

    return true;
}

entity *
world::load_entity(lvlbuf *buf, int version, uint32_t id_modifier, b2Vec2 displacement, std::map<uint32_t, entity*> *entities, std::vector<chunk_pos> *affected_chunks)
{
    entity *e = of::read(buf, version, id_modifier, displacement, affected_chunks);

    if (e) {
        e->on_load(false, e->state_size != 0);

        if (e->state_size) {
            size_t rp = buf->rp;
            buf->rp = e->state_ptr;
            e->read_state(0, buf);
            buf->rp = rp;
        }

        if (e->gr && e->flag_active(ENTITY_IS_COMPOSABLE)) {
            uint32_t group_id = VOID_TO_UINT32(e->gr);
            e->gr = 0;

            std::map<uint32_t, group*>::iterator r = this->groups.find((uint32_t)group_id);
            group *g = r->second;

            if (r == this->groups.end()) {
                /* group has not been loaded, try to fetch it from the preloader */
                tms_debugf("group id unknown, attempting to find in preloader ");
                g = this->cwindow->preloader.load_group(group_id);
            } else {
                g = r->second;
            }

            if (g) {
                g->push_entity((composable*)e, e->_pos, e->_angle);
            } else {
                tms_errorf("invalid group id %u", group_id);
            }
        }

        this->add(e);

        if (entities) entities->insert(std::make_pair(e->id, e));

    }

    return e;
}

group *
world::load_group(lvlbuf *buf, int version, uint32_t id_modifier, b2Vec2 displacement, std::map<uint32_t, group*> *groups)
{
    group *g = of::read_group(buf, version, id_modifier, displacement);
    if (g) {
        if (/*this->initial_add && */ /* XXX */this->level.type == LCAT_PUZZLE) {
            g->set_moveable(false);
        }

        g->on_load(false, g->state_size != 0);

        if (g->state_size) {
            size_t rp = buf->rp;
            buf->rp = g->state_ptr;
            g->read_state(0, buf);
            buf->rp = rp;
        }

        this->add(g);

        if (groups) groups->insert(std::make_pair(g->id, g));
    }
    return g;
}

cable*
world::load_cable(lvlbuf *buf, int version, uint64_t flags, uint32_t id_modifier, b2Vec2 displacement, std::set<cable*> *cables)
{
    uint8_t ctype = buf->r_uint8();
    uint32_t id = buf->r_uint32()+id_modifier;

    cable *c = new cable((int)ctype);
    c->id = id;

    if (version >= 11) {
        c->extra_length = buf->r_float();

        if (std::isnan(c->extra_length) || c->extra_length < 0.f || c->extra_length > CABLE_MAX_EXTRA_LENGTH) {
            c->extra_length = 0.f;
        }
    }

    if (version >= LEVEL_VERSION_1_5) {
        c->saved_length = buf->r_float();
    }

    if (version >= LEVEL_VERSION_1_0) {
        c->set_moveable(buf->r_uint8());
    }

    if (c->id >= of::_id) of::_id = c->id+1;

    c->ready = false;

    for (int x=0; x<2; x++) {
        uint32_t e_id = buf->r_uint32() + id_modifier;
        entity *e = this->get_entity_by_id(e_id);

        uint8_t s_index = buf->r_uint8();
        float px = buf->r_float() + displacement.x;
        float py = buf->r_float() + displacement.y;

        if (!e && (flags & LVL_CHUNKED_LEVEL_LOADING)) {
            tms_debugf("cable entity %u not found, checking in preloader", e_id);
            e = this->cwindow->preloader.load_entity(e_id);
            tms_debugf("found? %p", e);
        }

        if (e != 0) {
            //tms_infof("%p, %p(%s)(%u), %d", c->p[x], e->get_edevice(), e->get_name(), e->id, s_index);
            bool ret = c->connect(c->p[x], e->get_edevice(), s_index);

            if (!ret) {
                tms_errorf("could not connect plug to entity");
            }
        } else {
            c->p[x]->_pos.x = px;
            c->p[x]->_pos.y = py;
            c->p[x]->_angle = 0.f;
        }
    }

    this->add(c);

    if (cables) {
        cables->insert(c);
    }

    return c;
}

connection*
world::load_connection(lvlbuf *buf, int version, uint64_t flags, uint32_t id_modifier, b2Vec2 displacement, std::set<connection*> *connections)
{
    /* XXX keep in sync with chunk_preloader::preload_connection */

    connection c;
    connection *cc;
    c.pending = false;

    c.type = buf->r_uint8();

    uint32_t _e_id = buf->r_uint32() + id_modifier;
    uint32_t _o_id = buf->r_uint32();
    uint32_t chunk_pos_x = 0;
    uint32_t chunk_pos_y = 0;

    c.e = this->get_entity_by_id(_e_id);

    if (version >= LEVEL_VERSION_1_5) {
        /* XXX: should this be r_int32? */
        chunk_pos_x = buf->r_uint32();
        chunk_pos_y = buf->r_uint32();
        c.e_data = buf->r_uint32();
        c.o_data = buf->r_uint32();
    } else {
        c.o = this->get_entity_by_id(_o_id + id_modifier);
    }


    c.owned = (int)buf->r_uint8() == 1;
    c.fixed = (int)buf->r_uint8() == 1;
    c.o_index = buf->r_uint8();
    c.p.x = buf->r_float(); /* local coordinates, no need to add displacement */
    c.p.y = buf->r_float();
    c.p_s.x = buf->r_float();
    c.p_s.y = buf->r_float();

    c.f[0] = buf->r_uint8();
    c.f[1] = buf->r_uint8();

    c.max_force = (version >= 4 ? buf->r_float() : INFINITY);
    c.option = (version >= 5 ? buf->r_uint8() : 0);
    c.damping = (version >= 8 ? buf->r_float() : 0.f);

    c.angle = (version >= 14 ? buf->r_float() : 0.f);
    c.render_type = (version >= 14 ? buf->r_uint8() : 0);

    c.relative_angle = (version >= 22 ? buf->r_float() : -(c.o->get_angle(c.f[1]) - c.e->get_angle(c.f[0])));

    if (version >= LEVEL_VERSION_1_5) {
        if (_o_id == 0) { /* this is a chunk */
            c.o = this->cwindow->get_chunk(chunk_pos_x, chunk_pos_y);
            c.o->add_to_world();
        } else {
            c.o = this->get_entity_by_id(_o_id + id_modifier);
        }
    }

    if (version >= LEVEL_VERSION_1_5 && (flags & (LVL_CHUNKED_LEVEL_LOADING))) {
        if (!c.e) {
            tms_debugf("e_id invalid, checking in preloader");
            c.e = this->cwindow->preloader.load_entity(_e_id);
        }

        if (!c.o) {
            if (_o_id != 0) {
                tms_debugf("o_id invalid, checking in preloader");
                c.o = this->cwindow->preloader.load_entity(_o_id);
            } else {
                tms_errorf("entity connected to chunk, but chunk could not be found");
            }
        }
    }

    if (!c.e || !c.o) {
        tms_errorf("invalid entity ids <%u,%u>", _e_id, _o_id);
        return 0;
    }

    if (c.type == CONN_BR) {
        tms_infof("warning: removing deprecated CONN_BR connection");
        return 0;
    }

    c.update();

    if (c.owned) {
        /* the connection is owned by an object */
        cc = c.e->load_connection(c);
    } else
        cc = c.clone();

    if (cc) {
        this->insert_connection(cc);

        if (cc->e->gr != 0 && (cc->e->gr == cc->o->gr)
            && cc->type == CONN_GROUP) {
            cc->e->gr->push_connection(cc);
        }

        if (!cc->fixed) {
            cc->e->add_connection(cc);
            cc->o->add_connection(cc);
        }

        cc->j = 0;
        cc->create_joint(0);

        if (connections)
            connections->insert(cc);
    } else {
        tms_errorf("could not load connection");
    }

    return cc;
}

void
world::calculate_bounds(std::set<entity*> *entities, float *min_x, float *max_x,
                        float *min_y, float *max_y)
{
    entity *first = *entities->begin();

    *min_x = 0;
    *max_x = 0;
    *min_y = 0;
    *max_y = 0;

    if (entities->size()<1)
        return;

    *min_x = first->get_position().x;
    *max_x = first->get_position().x;
    *min_y = first->get_position().y;
    *max_y = first->get_position().y;

    b2Vec2 mask[4] = {
        b2Vec2(.5f, 0.5f),
        b2Vec2(.5f, -.5f),
        b2Vec2(-.5f, -.5f),
        b2Vec2(-.5f, .5f)
    };

    for (std::set<entity*>::iterator i = entities->begin();
            i != entities->end(); i++) {
        entity *e = *i;

        for (int x=0; x<4; x++) {
            b2Vec2 lp = e->local_to_world(b2Vec2(e->get_width()*mask[x].x, e->get_width()*mask[x].y), 0);

            if (lp.x < *min_x) *min_x = lp.x;
            if (lp.x > *max_x) *max_x = lp.x;
            if (lp.y < *min_y) *min_y = lp.y;
            if (lp.y > *max_y) *max_y = lp.y;
        }
    }

    tms_infof("bounds calculated to: %f %f %f %f", *min_x, *max_x, *min_y, *max_y);
}

void
world::fill_buffer(lvlinfo *lvl, lvlbuf *buf,
                   std::map<uint32_t, group*>  *groups,
                   std::map<uint32_t, entity*> *entities,
                   std::set<connection*>       *connections,
                   std::set<cable*>            *cables,
                   uint32_t id_modifier, b2Vec2 displacement,
                   bool fill_unloaded,
                   bool fill_states
                   )
{
    for (std::map<uint32_t, group*>::iterator i = groups->begin();
            i != groups->end(); i++) {
        i->second->pre_write();
        of::write_group(buf, lvl->version, i->second, id_modifier, displacement, fill_states);
        i->second->post_write();
    }

    if (fill_unloaded) this->cwindow->preloader.write_groups(lvl, buf);

    for (std::map<uint32_t, entity*>::iterator i = entities->begin();
            i != entities->end(); i++) {
        i->second->pre_write();
        of::write(buf, lvl->version, i->second, id_modifier, displacement, fill_states);
        i->second->post_write();
    }
    if (fill_unloaded) this->cwindow->preloader.write_entities(lvl, buf);

    for (std::set<cable*>::iterator i = cables->begin();
            i != cables->end(); i++) {
        cable *c = *i;
        b2Vec2 p0, p1;

        c->write_ptr = buf->size;

        p0 = c->p[0]->get_position() + displacement;
        p1 = c->p[1]->get_position() + displacement;

        buf->ensure(1+4+4+4+1+4+1+4+4+4+1+4+4);

        buf->w_uint8(c->ctype);
        buf->w_uint32(c->id + id_modifier);

        if (lvl->version >= 11) {
            buf->w_float(c->extra_length);
        }
        if (lvl->version >= LEVEL_VERSION_1_5) {
            buf->w_float(c->length);
        }
        if (lvl->version >= 15) {
            buf->w_uint8(c->is_moveable());
        }

        tms_debugf("cable plug 0 is connected? %d %u", c->p[0]->is_connected(), c->p[0]->is_connected() ? c->p[0]->plugged_edev->get_entity()->id : 0);
        buf->w_uint32((c->p[0]->is_connected() ? c->p[0]->plugged_edev->get_entity()->id + id_modifier: 0));
        buf->w_uint8(c->p[0]->get_socket_index());
        buf->w_float(p0.x);
        buf->w_float(p0.y);

        tms_debugf("cable plug 1 is connected? %d %u", c->p[1]->is_connected(), c->p[1]->is_connected() ? c->p[1]->plugged_edev->get_entity()->id : 0);
        buf->w_uint32((c->p[1]->is_connected() ? c->p[1]->plugged_edev->get_entity()->id + id_modifier: 0));
        buf->w_uint8(c->p[1]->get_socket_index());
        buf->w_float(p1.x);
        buf->w_float(p1.y);

        c->write_size = buf->size - c->write_ptr;
    }
    if (fill_unloaded) this->cwindow->preloader.write_cables(lvl, buf);

    for (std::set<connection*>::iterator i = connections->begin();
            i != connections->end(); i++) {
        buf->ensure(
                 sizeof(uint8_t) /* type */
                +sizeof(uint32_t) /* first id */
                +sizeof(uint32_t) /* second id */
                +sizeof(uint32_t) /* chunk x */
                +sizeof(uint32_t) /* chunk y */
                +sizeof(uint32_t) /* first data */
                +sizeof(uint32_t) /* second data */
                +sizeof(uint8_t) /* owned */
                +sizeof(uint8_t) /* fixed */
                +sizeof(uint8_t) /* o_index */
                +sizeof(float) /* local coordinates */
                +sizeof(float) /* local coordinates */
                +sizeof(float) /* local coordinates */
                +sizeof(float) /* local coordinates */
                +sizeof(uint8_t) /* f[0] */
                +sizeof(uint8_t) /* f[1] */
                +sizeof(float) /* max_force */
                +sizeof(uint8_t) /* option */
                +sizeof(float) /* damping */
                +sizeof(float) /* angle */
                +sizeof(uint8_t) /* render_type */
                +sizeof(float) /* relative_angle */
                );
        (*i)->write_ptr = buf->size;
        buf->w_uint8((*i)->type);
        buf->w_uint32((*i)->e->id + id_modifier);
        buf->w_uint32((!(*i)->o || (*i)->o->g_id == O_CHUNK) ? 0 : ((*i)->o->id + id_modifier));

        if (lvl->version >= LEVEL_VERSION_1_5) {
            if ((*i)->o->g_id == O_CHUNK) {
                level_chunk *c = static_cast<level_chunk*>((*i)->o);
                buf->w_int32(c->pos_x);
                buf->w_int32(c->pos_y);
            } else {
                buf->w_int32(0);
                buf->w_int32(0);
            }
            buf->w_uint32((*i)->e_data);
            buf->w_uint32((*i)->o_data);
        }

        buf->w_uint8((*i)->owned ? 1:0);
        buf->w_uint8((*i)->fixed ? 1:0);
        buf->w_uint8((*i)->o_index);
        buf->w_float((*i)->p.x); /* these are local coordinates, no need to displace */
        buf->w_float((*i)->p.y);
        buf->w_float((*i)->p_s.x);
        buf->w_float((*i)->p_s.y);
        buf->w_uint8((*i)->f[0]);
        buf->w_uint8((*i)->f[1]);
        if (lvl->version >= 4) {
            buf->w_float((*i)->max_force);
        }
        if (lvl->version >= 5) {
            buf->w_uint8((*i)->option);
        }
        if (lvl->version >= 8) {
            buf->w_float((*i)->damping);
        }
        if (lvl->version >= 14) {
            buf->w_float((*i)->angle);
        }
        if (lvl->version >= 14) {
            buf->w_uint8((*i)->render_type);
        }

        if (this->is_paused()) {
            /* if we're paused and saving, we recalculate the relative angles of
             * some connection types */
            (*i)->update_relative_angle(false);
        }

        if (lvl->version >= LEVEL_VERSION_1_2_4) {
            buf->w_float((*i)->relative_angle);
        }

        (*i)->write_size = buf->size - (*i)->write_ptr;
    }

    if (fill_unloaded) this->cwindow->preloader.write_connections(lvl, buf);
}

/**
 * return true if the world has minimum 'count' number of entities with the given gid
 *
 * very slow function
 **/
bool
world::has_num_entities_with_gid(uint32_t gid, int count)
{
    /* XXX TODO this is not compatible with chunked level loading! check preloader */
    for (std::map<uint32_t, entity*>::iterator i = this->all_entities.begin();
            i != this->all_entities.end(); i++) {
        if (i->second->g_id == gid) {
            count --;
            if (count <= 0) return true;
        }
    }

    return false;
}

/**
 * Save a partial set of entities from the world, including all related connections, groups and cables.
 * Used by game to save a multiselect.
 **/
void
world::save_partial(std::set<entity*> *entity_list, const char *name, uint32_t partial_id)
{
    uint32_t min_id = 0xffffffff;

    lvlinfo tmp;
    this->lb.clear();

    tmp.create(LCAT_PARTIAL, 0, W->level.version);
    int len = strlen(name);
    if (len > 255) len = 255;
    strncpy(tmp.name, name, len);
    tmp.name_len = len;

    this->calculate_bounds(entity_list, &tmp.min_x, &tmp.max_x, &tmp.min_y, &tmp.max_y);

    b2Vec2 displace = b2Vec2((tmp.min_x + tmp.max_x) / 2.f, (tmp.min_y + tmp.max_y) / 2.f);

    std::map<uint32_t, group*>  groups;
    std::map<uint32_t, entity*> entities;
    std::set<connection*>       connections;
    std::set<cable*>            cables;

    for (std::set<entity*>::iterator i = entity_list->begin();
            i != entity_list->end();
            i ++) {
        entity *en = (*i);
        edevice *e;

        if (en->gr) {
            if (groups.find(en->gr->id) == groups.end()) {
                groups.insert(std::pair<uint32_t, group*>(en->gr->id, en->gr));
                if (en->gr->id < min_id) min_id = en->gr->id;
            }
        }

        connection *c = en->conn_ll;

        if (c) {
            do {
                connection *next = c->next[(c->e == en) ? 0 : 1];
                entity *other = ((en == c->e) ? c->o : c->e);

                if (entity_list->find(other) != entity_list->end())
                    connections.insert(c);

                c = next;
            } while (c);
        }

        /* special case for pivots, dampers, etc, YUCK! */
        connection *extra_conn = 0;
        if (en->g_id == O_OPEN_PIVOT) extra_conn = &((pivot_1*)en)->dconn;
        else if (en->g_id == O_DAMPER) extra_conn = &((damper_1*)en)->dconn;
        else if (en->g_id == O_RUBBERBAND) extra_conn = &((rubberband_1*)en)->dconn;

        if (extra_conn) {
            connections.insert(extra_conn);
        }

        if ((e = en->get_edevice())) {
            entity *f;
            for (int x=0; x<e->num_s_in; x++) {
                if (e->s_in[x].p) {
                    if (e->s_in[x].p->get_other() && e->s_in[x].p->get_other()->plugged_edev) {
                        f = e->s_in[x].p->get_other()->plugged_edev->get_entity();
                        if (f && entity_list->find(f) != entity_list->end()) {
                            cables.insert(e->s_in[x].p->c);
                            if (e->s_in[x].p->c->id < min_id) min_id = e->s_in[x].p->c->id;
                        }
                    }
                }
            }
            for (int x=0; x<e->num_s_out; x++) {
                if (e->s_out[x].p) {
                    if (e->s_out[x].p->get_other() && e->s_out[x].p->get_other()->plugged_edev) {
                        f = e->s_out[x].p->get_other()->plugged_edev->get_entity();
                        if (f && entity_list->find(f) != entity_list->end()) {
                            cables.insert(e->s_out[x].p->c);
                            if (e->s_out[x].p->c->id < min_id) min_id = e->s_out[x].p->c->id;
                        }
                    }
                }

            }
        }

        entities.insert(std::pair<uint32_t,entity*>(en->id, en));
        if (en->id < min_id) min_id = en->id;
    }

    tmp.num_groups = groups.size();
    tmp.num_entities = entities.size();
    tmp.num_connections = connections.size();
    tmp.num_cables = cables.size();
    tmp.num_chunks = 0;
    tmp.num_gentypes = 0;
    tmp.write(&this->lb);

    /* TODO: make sure we're writing more than 1 entity, and that we're NOT writing the adventure robot */
    {
        lvlinfo *lvl = &tmp;
        tms_infof("fill buffer (v.%d, id mod:%u, displ: %f %f): groups:%d, entities:%d, conns:%d, cables:%d",
                lvl->version, -min_id+1, -displace.x, -displace.y, lvl->num_groups, lvl->num_entities, lvl->num_connections, lvl->num_cables);
    }
    this->fill_buffer(&tmp, &this->lb, &groups, &entities, &connections, &cables, -min_id+1, -displace);

    if (partial_id == 0) {
        /* TODO: get next partial id */
    }

    char filename[1024];
    snprintf(filename, 1023, "%s/%d.pobj", pkgman::get_level_path(LEVEL_LOCAL), partial_id);

    FILE *fp = fopen(filename, "wb");

    tms_infof("saving partial: %s", filename);

    if (fp) {
        fwrite(this->lb.buf, 1, this->lb.size, fp);
        fclose(fp);
    } else {
        tms_errorf("could not open file '%s' for writing", filename);
        /* TODO: report to user */
    }
}

void
world::open_autosave()
{
    this->open(LEVEL_LOCAL, 0, true, false);
}

bool
world::save(int save_type)
{
    tms_infof("Saving (%d)", save_type);
    this->lb.clear();

    if (save_type == SAVE_TYPE_AUTOSAVE) {
        this->level.autosave_id = this->level.local_id;
    }

    this->cwindow->preloader.prepare_write();

    this->level.num_groups = this->groups.size() + this->cwindow->preloader.groups.size();
    this->level.num_entities = this->all_entities.size() + this->cwindow->preloader.entities.size();
    this->level.num_connections = this->connections.size() + this->cwindow->preloader.connections.size();
    this->level.num_cables = this->cables.size() + this->cwindow->preloader.cables.size();
    this->level.num_chunks = this->cwindow->preloader.active_chunks.size()
                           + this->cwindow->preloader.wastebin.size()
                           + this->cwindow->preloader.chunks.size();
    this->level.num_gentypes = this->cwindow->preloader.gentypes.size();
    this->level.state_size = (save_type == SAVE_TYPE_STATE ? G->get_state_size() : 0);
    this->level.write(&this->lb);

    if (save_type == SAVE_TYPE_STATE) {
        G->write_state(&this->level, &this->lb);
    }

    {
        lvlinfo *lvl = &this->level;
        tms_infof("fill buffer (v.%d, id mod:%u, displ: %f %f): groups:%d, entities:%d, conns:%d, cables:%d",
                lvl->version, 0, 0.f, 0.f, lvl->num_groups, lvl->num_entities, lvl->num_connections, lvl->num_cables);
    }
    this->fill_buffer(&this->level, &this->lb, &this->groups, &this->all_entities, &this->connections, &this->cables, 0, b2Vec2(0.f, 0.f), true, (save_type == SAVE_TYPE_STATE));

    this->cwindow->preloader.write_gentypes(&this->level, &this->lb);
    this->cwindow->preloader.write_chunks(&this->level, &this->lb);

    char filename[1024];

    switch (save_type) {
        case SAVE_TYPE_DEFAULT:
            if (this->level.local_id == 0) {
                /* this level does not have a local id,
                 * we need to retrieve a new id for it */
                this->level.local_id = pkgman::get_next_level_id();
                tms_infof("Assigned level ID: %d", this->level.local_id);
            }

            if (G->state.is_main_puzzle) {
                snprintf(filename, 1023, "%s/7.%d.psol", pkgman::get_level_path(LEVEL_LOCAL), this->level.local_id);
            } else {
                snprintf(filename, 1023, "%s/%d.plvl", pkgman::get_level_path(LEVEL_LOCAL), this->level.local_id);
            }
            break;

        case SAVE_TYPE_AUTOSAVE:
            snprintf(filename, 1023, "%s/.autosave", pkgman::get_level_path(LEVEL_LOCAL));
            break;

        case SAVE_TYPE_STATE:
            if (this->level.local_id == 0) {
                //tms_errorf("can not save state for autosave shit");
                //return false;
            }

            if (this->level.save_id == 0) {
                this->level.save_id = (uint32_t)time(0);
            }

            uint8_t level_type = this->level_id_type;

            if (level_type < LEVEL_LOCAL_STATE) {
                level_type += LEVEL_LOCAL_STATE;
            }

            pkgman::get_level_full_path(level_type, this->level.local_id, this->level.save_id, filename);
            this->save_cache(level_type, this->level.local_id, this->level.save_id);
            break;
    }

    bool ret = true;

    if (this->level.version >= LEVEL_VERSION_1_5) {
        this->level.compression_length = this->lb.size - this->level.get_size();

        tms_infof("Size before: %" PRIu64, this->lb.size);
        unsigned char *dest = 0;
        uint64_t dest_len = 0;
        this->lb.zcompress(this->level, &dest, &dest_len);

        lvlbuf zlb;
        zlb.reset();
        zlb.size = 0;

        /* Write level header with the newly modified compression length */
        this->level.write(&zlb);

        zlb.ensure(dest_len);

        /* Write compressed blob */
        zlb.w_buf((char*)dest, dest_len);

        zlb.size = this->level.get_size() + dest_len;

        ret = this->write_level(filename, &zlb);
    } else {
        ret = this->write_level(filename, &this->lb);
    }

    return ret;
}

bool
world::write_level(const char *filename, lvlbuf *out_lb)
{
    FILE *fp = fopen(filename, "wb");

    tms_infof("saving level: %s", filename);
    tms_infof("with name: '%s'", this->level.name);
    tms_infof("size: %" PRIu64, out_lb->size);

    if (fp) {
        fwrite(out_lb->buf, 1, out_lb->size, fp);
        fclose(fp);
    } else {
        tms_errorf("could not open file '%s' for writing", filename);
        return false;
    }

    return true;
}

bool
world::load_partial_from_buffer(lvlbuf *lb, b2Vec2 position,
                                std::map<uint32_t, entity*> *entities,
                                std::map<uint32_t, group*> *groups, std::set<connection*> *connections, std::set<cable*> *cables) {
    lvlinfo tmp;
    if (!tmp.read(lb)) {
        ui::message("You need to update Principia to load this object.");
        return false;
    }

    /* XXX: There is probably a more simple way to increment the ID, this increases the ID by a bit too much for some reason. */
    uint32_t id_modifier = of::get_next_id();

    tms_infof("loading buffer with id modifier %u, version %u", id_modifier, tmp.version);

    uint8_t old_version = W->level.version;
    W->level.version = tmp.version;

    bool status = this->load_buffer(&tmp, lb, id_modifier, position, entities, groups, connections, cables);

    W->level.version = old_version;

    if (!status) {
        ui::message("An error occured while reading the object.");
    }

    return status;
}

bool
world::load_partial(uint32_t id, b2Vec2 position,
                    std::map<uint32_t, entity*> *entities,
                    std::map<uint32_t, group*> *groups,
                    std::set<connection*> *connections,
                    std::set<cable*> *cables)
{
    char filename[1024];
    snprintf(filename, 1023, "%s/%d.pobj", pkgman::get_level_path(LEVEL_LOCAL), id);

    if (!this->is_paused()) {
        tms_errorf("can not load partials in PLAY mode!");
        return false;
    }

    tms_infof("Opening partial: %s", filename);

    FILE *fp = fopen(filename, "rb");

    if (fp) {
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (size > 8*1024*1024)
            tms_fatalf("Partial too big");

        this->lb.reset();
        this->lb.size = 0;
        this->lb.ensure((int)size);

        fread(this->lb.buf, 1, size, fp);

        fclose(fp);

        this->lb.size = size;
        tms_infof("read file of size: %lu", size);

        if (this->load_partial_from_buffer(&this->lb, position, entities, groups, connections, cables)) {
            this->init_level_entities(entities);
            return true;
        } else
            return false;
    }

    tms_errorf("could not open file '%s' for reading", filename);
    return false;
}

bool
world::open(int id_type, uint32_t id, bool paused, bool sandbox, uint32_t save_id/*=0*/)
{
    bool is_autosave = false;

    this->reset();
    this->init(paused);

    char filename[1024];

    if (id == 0 && id_type == LEVEL_LOCAL) {
        /* opening an autosave file */
        tms_debugf("Opening autosave file");
        snprintf(filename, 1023, "%s/.autosave", pkgman::get_level_path(id_type));
        is_autosave = true;
    } else if (G->state.is_main_puzzle) {
        snprintf(filename, 1023, "%s/7.%d.psol", pkgman::get_level_path(id_type), id);
    } else {
        pkgman::get_level_full_path(id_type, id, save_id, filename);
    }

    tms_infof("Opening level: %s", filename);

    FILE_IN_ASSET(id_type == LEVEL_MAIN);
    _FILE *fp = _fopen(filename, "rb");

    if (fp) {
        _fseek(fp, 0, SEEK_END);
        long size = _ftell(fp);
        _fseek(fp, 0, SEEK_SET);

        if (size > 8*1024*1024) {
            // XXX: Is this necessary? can we support larger levels
            tms_fatalf("Level file too big");
        }

        this->lb.reset();
        this->lb.size = 0;
        this->lb.ensure((int)size);

        _fread(this->lb.buf, 1, size, fp);

        _fclose(fp);

        this->lb.size = size;
        tms_infof("read file of size: %lu", size);

        if (!this->level.read(&this->lb)) {
            ui::message("You need to update Principia to play this level.", true);
            return false;
        } else {
            tms_debugf("Successfully read level");
            tms_debugf("Version: %u", this->level.version);
        }

        if (!this->read_cache(id_type, id, save_id)) {

        }

        if (!sandbox && this->level.visibility == LEVEL_LOCKED && G->state.pkg == 0) {
            ui::message("This level is locked and can only be played from inside a package.");
            tms_errorf("locked level");
            return false;
        }

        this->level_id_type = id_type;
        if (is_autosave) {
            this->level.local_id = this->level.autosave_id;
        } else {
            this->level.local_id = id;
        }

        G->init_background();

        this->init_level();

        if (this->level.version >= LEVEL_VERSION_1_5) {
            this->lb.zuncompress(this->level);
        }

        /* save the location of the state buffer so game can load it later */
        this->state_ptr = this->lb.rp;
        this->lb.rp += this->level.state_size;

        bool result;

        if (this->level.flag_active(LVL_CHUNKED_LEVEL_LOADING)) {
            result = this->cwindow->preloader.preload(&this->level, &this->lb);
        } else {
            result = this->load_buffer(&this->level, &this->lb);
            /* preloader is always used to load and write chunks, disregarding chunked level loading flag */
            this->cwindow->preloader.read_gentypes(&this->level, &this->lb);
            this->cwindow->preloader.read_chunks(&this->level, &this->lb);
        }

        if (!result) {
            ui::message("Could not load level. You may need to update Principia to the latest version.", true);
            this->reset();
            return false;
        }


        uint8_t extra = this->lb.r_uint8();

        if (extra == (uint8_t)1 && sandbox) {
            this->reset();
            return false;
        }

        if (!sandbox && this->level.type == LCAT_PUZZLE) this->apply_puzzle_constraints();
        if (!paused) this->optimize_connections();

        return true;
    }

    tms_errorf("could not open file '%s' for reading", filename);
    return false;
}

void
world::begin()
{
    this->init_level_entities(&this->all_entities, &this->groups);

    tms_assertf(this->to_be_absorbed.empty(),  "Pending absorbs not empty in world::begin");
    tms_assertf(this->to_be_destroyed.empty(), "Pending joint destructions not empty in world::begin");
    tms_assertf(this->to_be_reloaded.empty(),  "Pending reloads not empty in world::begin");
    tms_assertf(this->post_interact.empty(),   "Post interact not empty in world::begin");

    if (!paused && this->level_id_type < LEVEL_LOCAL_STATE) {
        /* step everything once and solve electronics once, this makes screenshots
         * look better, and things that modify game states like timemul or gravity
         * have a shot before the player begins */
        for (std::set<entity*>::iterator i = this->prestepable.begin();
                i != this->prestepable.end(); i++) {
            (*i)->pre_step();
        }

        for (std::set<entity*>::iterator i = this->mstepable.begin();
                i != this->mstepable.end(); i++) {
            (*i)->mstep();
        }

        for (std::set<entity*>::iterator i = this->stepable.begin();
                i != this->stepable.end(); i++) {
            (*i)->step();
        }

        this->first_solve = true;
        this->solve_electronics();
        this->first_solve = false;
    }
}


/**
 **/
void
world::apply_puzzle_constraints()
{
    if (this->level.type == LCAT_PUZZLE) {
        for (std::set<connection*>::iterator i = this->connections.begin();
                i != this->connections.end(); i++) {
            connection *c = *i;
            if (!c->e->is_moveable() && !c->o->is_moveable()) {
                (*i)->fixed = true;
            }
        }
    }
}

bool
world::read_cache(int level_type, uint32_t id, uint32_t save_id/*=0*/)
{
    char cache_path[1024];

    pkgman::get_cache_full_path(level_type, id, save_id, cache_path);

    tms_debugf("Reading cache (%s)", cache_path);

    FILE *fh = fopen(cache_path, "r");

    if (!fh) {
        tms_debugf("No cache file available.");
        return false;
    }

    char buf[256];

    while (fgets(buf, 256, fh) != NULL) {
        if (strchr(buf, '=') == NULL) continue;
        size_t sz = strlen(buf);
        char key[64];
        char val[64];
        int k = 0;

        bool on_key = true;

        for (size_t i = 0; i < sz; ++i) {
            if (buf[i] == '\n' || buf[i] == ' ') continue;

            if (k == 62) {
                if (on_key) {
                    key[k++] = '\0';
                    k = 0;
                    on_key = false;
                }
                else {
                    break;
                }
            }

            if (buf[i] == '=') {
                if (on_key) key[k++] = '\0';
                k = 0;
                on_key = false;
                continue;
            }

            if (on_key) {
                key[k++] = buf[i];
            } else {
                val[k++] = buf[i];
            }
        }

        val[k] = '\0';

        tms_debugf("Val: %.25f", atof(val));
        this->level_variables.insert(std::pair<std::string, double>(key, atof(val)));
    }

    fclose(fh);

    return true;
}

bool
world::save_cache(int level_type, uint32_t id, uint32_t save_id/*=0*/)
{
    char cache_path[1024];

    pkgman::get_cache_full_path(level_type, id, save_id, cache_path);

    tms_debugf("Saving cache (%s)", cache_path);

    FILE *fh = fopen(cache_path, "w+");

    if (!fh) {
        tms_errorf("no file to open!");
        return false;
    }

    for (std::map<std::string, double>::iterator i = this->level_variables.begin();
            i != this->level_variables.end(); ++i) {
        fprintf(fh, "%s=%.15f\n", i->first.c_str(), i->second);
    }

    fclose(fh);

    return true;
}

entity *
world::get_entity_by_id(uint32_t id)
{
    if (id) {
        std::map<uint32_t, entity*>::const_iterator i = this->all_entities.find(id);

        if (i != this->all_entities.end()) {
            return i->second;
        }
    }

    return 0;
}

void
world::b2_sleep_listener::OnSleep(b2Body *b)
{
    /*tms_debugf("body %p went to sleep", b);
    tms_debugf("%p", b->GetFixtureList()->GetUserData());*/
    /*if (b->GetFixtureList()->GetUserData()) {
        tms_debugf("%s", static_cast<entity*>(b->GetFixtureList()->GetUserData())->get_name());
    }*/

    if (!(W->level.flags & LVL_CHUNKED_LEVEL_LOADING)) {
        return;
    }

    /* loop through all fixtures and decrement the num fixtures in the chunks their
     * contained in */
    b2Fixture *f, *my;
    entity *e;
    entity *ee;
    b2ContactEdge *c;

    for (c = b->GetContactList(); c; c = c->next) {
        if (c->other->GetType() == b2_staticBody && c->contact->IsTouching()) {
            if (c->contact->GetFixtureA()->GetBody() == b) {
                f = c->contact->GetFixtureB();
                my = c->contact->GetFixtureA();
            } else {
                f = c->contact->GetFixtureA();
                my = c->contact->GetFixtureB();
            }

            if (!(ee = static_cast<entity*>(my->GetUserData()))) {
                continue;
            }

            /*
            if (ee->flag_active(ENTITY_DYNAMIC_UNLOADING)) {
                continue;
            }
            */

            if ((e = static_cast<entity*>(f->GetUserData()))) {
                if (e->g_id == O_CHUNK && f->IsSensor()) {
                    static_cast<level_chunk*>(e)->remove_fixture(my, ee);
                }
            }
        }
    }
}

void
world::b2_sleep_listener::OnWakeup(b2Body *b)
{
    if (!(W->level.flags & LVL_CHUNKED_LEVEL_LOADING)) {
        return;
    }

    b2Fixture *f, *my;
    entity *e;
    entity *ee;
    b2ContactEdge *c;

    for (c = b->GetContactList(); c; c = c->next) {
        if (c->other->GetType() == b2_staticBody && c->contact->IsTouching()) {
            if (c->contact->GetFixtureA()->GetBody() == b) {
                f = c->contact->GetFixtureB();
                my = c->contact->GetFixtureA();
            } else {
                f = c->contact->GetFixtureA();
                my = c->contact->GetFixtureB();
            }

            if (!(ee = static_cast<entity*>(my->GetUserData()))) {
                continue;
            }

            /*if (ee->flag_active(ENTITY_DYNAMIC_UNLOADING)) {
                continue;
            }*/

            if ((b->m_flags & b2Body::e_forceSleepFlag3) && ee->flag_active(ENTITY_STATE_SLEEPING)) {
                ee->set_flag(ENTITY_STATE_SLEEPING, false);
                ee->num_chunk_intersections = 0;
            }

            if ((e = static_cast<entity*>(f->GetUserData()))) {
                if (e->g_id == O_CHUNK && f->IsSensor()) {
                    static_cast<level_chunk*>(e)->add_fixture(my, ee);
                }
            }
        }
    }

    b->m_flags &= ~b2Body::e_forceSleepFlag3;
}

void
world::b2_destruction_listener::SayGoodbye(b2Joint *j)
{
    G->say_goodbye(j);
}

void
world::b2_destruction_listener::SayGoodbye(b2Fixture *f)
{
    //G->say_goodbye(f);
}

void
world::raycast(b2RayCastCallback *callback,
        const b2Vec2 &point1, const b2Vec2 &point2,
        float r/*=.3f*/, float g/*=.9f*/, float b/*=.3f*/,
        int64_t life/*=1.7f*/)
{
    this->b2->RayCast(callback, point1, point2);

#ifdef DEBUG
    // if debug is activated, draw some funny lines
    struct game_debug_line *gdl = static_cast<struct game_debug_line*>(malloc(sizeof(struct game_debug_line)));
    gdl->x1 = point1.x;
    gdl->y1 = point1.y;
    gdl->x2 = point2.x;
    gdl->y2 = point2.y;

    gdl->r = r;
    gdl->g = g;
    gdl->b = b;
    gdl->life = life;
    G->lock();
    G->debug_lines.insert(gdl);
    G->unlock();
#endif
}

#ifdef DEBUG

static struct game_debug_line*
create_gdl(float x1, float y1, float x2, float y2, float r, float g, float b, int64_t life)
{
    struct game_debug_line *gdl = static_cast<struct game_debug_line*>(malloc(sizeof(struct game_debug_line)));

    gdl->x1 = x1;
    gdl->y1 = y1;
    gdl->x2 = x2;
    gdl->y2 = y2;

    gdl->r = r;
    gdl->g = g;
    gdl->b = b;
    gdl->life = life;

    return gdl;
}

#endif

void
world::query_aabb(b2QueryCallback *callback,
        const b2AABB &aabb,
        float r/*=.3f*/, float g/*=.9f*/, float b/*=.3f*/,
        int64_t life/*=1.7f*/)
{
    this->b2->QueryAABB(callback, aabb);

#ifdef DEBUG
    G->lock();

    G->debug_lines.insert(
            create_gdl(
                aabb.lowerBound.x, aabb.lowerBound.y,
                aabb.lowerBound.x, aabb.upperBound.y,
                r, g, b, life
                ));

    G->debug_lines.insert(
            create_gdl(
                aabb.lowerBound.x, aabb.upperBound.y,
                aabb.upperBound.x, aabb.upperBound.y,
                r, g, b, life
                ));

    G->debug_lines.insert(
            create_gdl(
                aabb.upperBound.x, aabb.upperBound.y,
                aabb.upperBound.x, aabb.lowerBound.y,
                r, g, b, life
                ));

    G->debug_lines.insert(
            create_gdl(
                aabb.upperBound.x, aabb.lowerBound.y,
                aabb.lowerBound.x, aabb.lowerBound.y,
                r, g, b, life
                ));

    G->unlock();
#endif
}
