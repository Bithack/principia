#include "chunk.hh"
#include "world.hh"
#include "game.hh"
#include "group.hh"
#include "terrain.hh"
#include "gentype.hh"

#include "pivot.hh"
#include "damper.hh"
#include "rubberband.hh"

static size_t _conn_id = 0;

chunk_preloader::chunk_preloader()
{
    this->w_lb.cap = 8388608; /* 2^23 */
    this->w_lb.min_cap = this->w_lb.cap;
    this->w_lb.sparse_resize = true;
    this->w_lb.size = 0;
    this->w_lb.buf = (unsigned char*)malloc(this->w_lb.cap);

    this->heap.cap = 8388608; /* 2^23 */
    this->heap.min_cap = this->heap.cap;
    this->heap.sparse_resize = true;
    this->heap.size = 0;
    this->heap.buf = (unsigned char*)malloc(this->heap.cap);

    this->reset();
}

chunk_preloader::~chunk_preloader()
{
    this->reset();
}

void
chunk_preloader::reset()
{
    /* TODO: free everything */
    _conn_id = 0;
    _gentype_id = 1;

    this->w_lb.reset();
    this->heap.reset();
    this->entities_by_chunk.clear();
    this->groups.clear();
    this->entities.clear();
    this->cables.clear();
    this->chunks.clear();
    this->gentypes.clear();
    this->connections.clear();
    this->connection_rels.clear();

    this->loaded_entities.clear();
    this->loaded_groups.clear();
    this->loaded_connections.clear();
    this->loaded_cables.clear();
    this->affected_chunks.clear();

    this->clear_chunks();
}

void
chunk_preloader::clear_chunks()
{
    for (chunk_iterator it = this->active_chunks.begin();
            it != this->active_chunks.end(); ++it) {
        it->second->remove_from_world();
        delete it->second;
    }

    for (chunk_iterator it = this->wastebin.begin();
            it != this->wastebin.end(); ++it) {
        it->second->remove_from_world();
        delete it->second;
    }
    this->active_chunks.clear();
    this->wastebin.clear();
}

bool
chunk_preloader::preload(lvlinfo *lvl, lvlbuf *lb)
{
    /* XXX keep in sync with world::load_buffer */
    uint32_t num_entities, num_groups, num_connections, num_cables, num_gentypes, n_read = 0;

    this->w_lb.reset();
    this->w_lb.ensure(lb->size);
    memcpy(this->w_lb.buf, lb->buf, lb->size);
    this->w_lb.size = lb->size;
    this->w_lb.rp = lb->rp;

    num_groups = lvl->num_groups;
    num_entities = lvl->num_entities;
    num_connections = lvl->num_connections;
    num_cables = lvl->num_cables;
    num_gentypes = lvl->num_gentypes;

    tms_infof("PRELOAD buffer");
    tms_infof("num groups %d, num entities %d, num connections %d, num_cables %d", num_groups, num_entities, num_connections, num_cables);

    while (!this->w_lb.eof() && n_read < num_groups) {
        this->preload_group();
        n_read ++;
    }

    n_read = 0;
    while (!this->w_lb.eof() && n_read < num_entities) {
        this->preload_entity();
        n_read ++;
    }

    n_read = 0;
    while (!this->w_lb.eof() && n_read < num_cables) {
        this->preload_cable();
        n_read ++;
    }

    n_read = 0;
    while (!this->w_lb.eof() && n_read < num_connections) {
        this->preload_connection();
        n_read ++;
    }

    this->read_gentypes(lvl, &this->w_lb);
    this->read_chunks(lvl, &this->w_lb);

    /* auto-load the adventure robot */
    if (W->level.type == LCAT_ADVENTURE) {
        this->load_entity(G->state.adventure_id);
    }

    this->loaded_entities.clear();

    return true;
}

void
chunk_preloader::preload_group()
{
    /* XXX keep in sync with of::preload_group() */
    uint32_t id;
    b2Vec2 pos;
    float angle;

    size_t ptr = w_lb.rp, size;
    id = w_lb.r_uint32();

    if (id >= of::_id) {
        of::_id = id+1;
    }

    w_lb.rp += sizeof(float) + sizeof(float) + sizeof(float);
    size_t state_size = w_lb.r_uint32();
    w_lb.rp += state_size;

    size = w_lb.rp - ptr;
    this->groups.insert(std::make_pair(id, preload_info(ptr,size)));
}

void
chunk_preloader::preload_entity()
{
    /* XXX keep in sync with of::read() */
    size_t ptr = this->w_lb.rp;

    uint32_t g_id = w_lb.r_uint8();
    uint32_t id = w_lb.r_uint32();

    w_lb.rp += sizeof(uint16_t)+sizeof(uint16_t); /* skip group id */

    uint8_t num_props = w_lb.r_uint8();
    uint8_t num_chunks = w_lb.r_uint8();
    uint32_t state_size = w_lb.r_uint32();

    if (id >= of::_id && id != 0xffffffff) of::_id = id+1;

    float px = w_lb.r_float();
    float py = w_lb.r_float();

    /* angle, layer */
    w_lb.rp += sizeof(float)+sizeof(uint8_t);

    w_lb.rp += sizeof(uint64_t); /* flags */

    if (num_chunks) {
        for (int x=0; x<num_chunks; x++) {
            int xx = (int)w_lb.r_uint32();
            int yy = (int)w_lb.r_uint32();
            chunk_pos cp(xx, yy);
            this->entities_by_chunk.insert(std::make_pair(cp, id));
            tms_debugf("preloaded entity %u (%u) in chunk [%d,%d]", id, g_id, cp.x, cp.y);
        }
    } else {
        tms_warnf("entity %u is not in a chunk, guessing", id);
        int xx = (int)floorf(px / 8.f);
        int yy = (int)floorf(py / 8.f);
        chunk_pos cp(xx, yy);
#ifdef DEBUG_PRELOADER_SANITY
        tms_assertf(abs(cp.x) < 1000 && abs(cp.y) < 1000, "guessed chunk with suspicious position %d %d for entity %u with pos %f,%f", cp.x, cp.y, id, px, py);
#endif
        this->entities_by_chunk.insert(std::make_pair(cp, id));
        tms_debugf("preloaded entity %u in chunk [%d,%d]", id, cp.x, cp.y);
    }

    /* skip state */
    w_lb.rp += state_size;

    for (int x=0; x<num_props; x++) {
        uint8_t type = w_lb.r_uint8();
        switch (type) {
            default:
                tms_fatalf("Invalid property type");
            case P_INT8: w_lb.rp += sizeof(uint8_t); break;
            case P_ID: case P_FLT: case P_INT:
                 w_lb.rp+=sizeof(uint32_t);
                 break;

            case P_STR:
                 {
                     if (W->level.version >= LEVEL_VERSION_1_5) {
                         uint32_t len = w_lb.r_uint32();
                         w_lb.rp += len;
                     } else {
                         uint16_t len = w_lb.r_uint16();
                         w_lb.rp += len;
                     }
                 }
            break;
        }
    }

    size_t size = w_lb.rp - ptr;
    this->entities.insert(std::make_pair(id, preload_info(ptr, size)));
}

void
chunk_preloader::preload_cable()
{
    /* XXX: keep in sync with world::laod_buffer */
    size_t ptr = w_lb.rp;
    w_lb.rp += sizeof(uint8_t);
    uint32_t id = w_lb.r_uint32();

    if (id >= of::_id) of::_id = id+1;

    w_lb.rp += sizeof(float); /* extra length */
    w_lb.rp += sizeof(float); /* saved length */
    w_lb.rp += sizeof(uint8_t); /* moveable */

    for (int x=0; x<2; x++) {
        uint32_t e_id = w_lb.r_uint32(); /* entity id */
        w_lb.rp += sizeof(uint8_t); /* socket index */
        w_lb.rp += sizeof(float)*2; /* pos x,y */

        if (e_id) this->cable_rels.insert(std::make_pair(e_id, id));
    }

    size_t size = w_lb.rp - ptr;
    this->cables.insert(std::make_pair(id, preload_info(ptr, size)));
}

void
chunk_preloader::preload_connection()
{
    /* XXX: keep in sync with world::load_connection */

    size_t ptr = w_lb.rp;

    uint8_t type = w_lb.r_uint8();

    uint32_t e_id = w_lb.r_uint32();
    uint32_t o_id = w_lb.r_uint32();

    w_lb.rp += sizeof(uint32_t); /* chunk pos x */
    w_lb.rp += sizeof(uint32_t); /* chunk pos y */
    w_lb.rp += sizeof(uint32_t); /* e data */
    w_lb.rp += sizeof(uint32_t); /* o data */

    w_lb.rp += sizeof(uint8_t); /* owned */
    w_lb.rp += sizeof(uint8_t); /* fixed */
    w_lb.rp += sizeof(uint8_t); /* o_index */
    w_lb.rp += sizeof(float); /* p.x */
    w_lb.rp += sizeof(float); /* p.y */
    w_lb.rp += sizeof(float); /* p_s.x */
    w_lb.rp += sizeof(float); /* p_s.y */
    w_lb.rp += sizeof(uint8_t); /* f[0] */
    w_lb.rp += sizeof(uint8_t); /* f[1] */
    w_lb.rp += sizeof(float); /* force_max */
    w_lb.rp += sizeof(uint8_t); /* option */
    w_lb.rp += sizeof(float); /* damping */
    w_lb.rp += sizeof(float); /* angle */
    w_lb.rp += sizeof(uint8_t); /* render_type */
    w_lb.rp += sizeof(float); /* relative_angle */

    size_t size = w_lb.rp - ptr;

    size_t conn_id = _conn_id ++;

    this->connections.insert(std::make_pair(conn_id, preload_info(ptr, size)));
    this->connection_rels.insert(std::make_pair(e_id, conn_id));
    this->connection_rels.insert(std::make_pair(o_id, conn_id));

    tms_debugf("preloaded connection %u <-> %u @ %u", e_id, o_id, (uint32_t)ptr);
}

void
chunk_preloader::unwaste_chunk(level_chunk *l)
{
    std::map<chunk_pos, level_chunk*>::iterator i;
    if ((i = this->wastebin.find(chunk_pos(l->pos_x, l->pos_y))) != this->wastebin.end()) {

        this->active_chunks.insert(std::make_pair(i->first, i->second));
        this->wastebin.erase(i);
        l->garbage = 0;
        l->init_chunk_neighbours();

        if (l->load_phase == 2) {
            l->load_phase = l->body ? 1 : 0;
            this->load(l, 2);
        }

        l->loaded_neighbours = false;
        l->load_neighbours();
    }
}

/**
 * Unload a chunk in the world to the heap
 *
 * If an entity has a cable that extends from one chunk to another, we make
 * sure both chunks are marked as garbage
 **/
bool
chunk_preloader::unload(level_chunk *chunk)
{
    if (chunk->body) {
        std::set<entity*> entity_list;
        std::set<entity*> gathered_entities;

        b2Fixture *f, *my;
        entity *e;
        b2ContactEdge *c;
        b2Body *b = chunk->body;

        for (c = b->GetContactList(); c; c = c->next) {
            if (c->contact->GetFixtureA()->GetBody() == b) {
                f = c->contact->GetFixtureB();
                my = c->contact->GetFixtureA();
            } else {
                f = c->contact->GetFixtureA();
                my = c->contact->GetFixtureB();
            }

            if (my->GetUserData2() == 0) {
                if ((e = static_cast<entity*>(f->GetUserData()))) {
                    entity_list.insert(e);
                }
            }
        }

        {
            connection *c = chunk->conn_ll;
            while (c) {
                entity *other = c->get_other(chunk);

                entity_list.insert(other);

                c = c->get_next(chunk);
            }
        }

        for (std::set<entity*>::iterator i = entity_list.begin(); i != entity_list.end(); i++) {
            e = (*i);
            if (gathered_entities.find(e) == gathered_entities.end()) {
                tms_debugf("gathering %s", e->get_name());
                e->gather_connected_entities(&gathered_entities, true, true, false, true);
            }
        }

        std::map<uint32_t, group*>  groups;
        std::map<uint32_t, entity*> entities;
        std::set<connection*>       connections;
        std::set<cable*>            cables;

        for (std::set<entity*>::iterator i = gathered_entities.begin();
                i != gathered_entities.end();
                i ++) {
            entity *en = (*i);
            edevice *e;

            level_chunk *chunk;

            if (en->id == G->state.adventure_id) {
                return false;
            }

            terrain_coord coord(en->get_position().x, en->get_position().y);

            if ((chunk = W->cwindow->get_chunk(coord.chunk_x, coord.chunk_y, true))) {
                if (!chunk->garbage)
                    return false;
            }

            if (en->gr) {
                if (groups.find(en->gr->id) == groups.end()) {
                    groups.insert(std::pair<uint32_t, group*>(en->gr->id, en->gr));
                }
            }

            connection *c = en->conn_ll;

            if (c) {
                do {
                    connection *next = c->next[(c->e == en) ? 0 : 1];
                    entity *other = ((en == c->e) ? c->o : c->e);

                    connections.insert(c);

                    c = next;
                } while (c);
            }

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
                            if (f && gathered_entities.find(f) != gathered_entities.end()) {
                                cables.insert(e->s_in[x].p->c);
                            }
                        }
                    }
                }
                for (int x=0; x<e->num_s_out; x++) {
                    if (e->s_out[x].p) {
                        if (e->s_out[x].p->get_other() && e->s_out[x].p->get_other()->plugged_edev) {
                            f = e->s_out[x].p->get_other()->plugged_edev->get_entity();
                            if (f && gathered_entities.find(f) != gathered_entities.end()) {
                                cables.insert(e->s_out[x].p->c);
                            }
                        }
                    }

                }
            }

            entities.insert(std::pair<uint32_t,entity*>(en->id, en));
        }

        if (!entities.empty()) {
            tms_debugf("writing %d entities to heap", (int)entities.size());
        }
        W->fill_buffer(&W->level, &this->heap, &groups, &entities, &connections, &cables, 0, b2Vec2(0.f, 0.f), false, true);

        /* now everything is written to the heap, insert preload_info into our maps */
        {
            for (std::map<uint32_t, entity*>::iterator i = entities.begin();
                    i != entities.end(); i++) {
                entity *e = i->second;
                preload_info info(e->write_ptr, e->write_size, true);

                this->entities.insert(std::make_pair(e->id, info));

                for (int x=0; x<e->num_chunk_intersections; x++) {
                    tms_debugf("chunk intersection %d %d",e->chunk_intersections[x].x, e->chunk_intersections[x].y);
                    this->entities_by_chunk.insert(std::make_pair(chunk_pos(e->chunk_intersections[x].x, e->chunk_intersections[x].y), e->id));
                }
            }
            for (std::set<connection*>::iterator i = connections.begin(); i != connections.end(); i++) {
                connection *e = (*i);
                size_t conn_id = _conn_id++;
                preload_info info(e->write_ptr, e->write_size, true);
                this->connections.insert(std::make_pair(conn_id, info));

                this->connection_rels.insert(std::make_pair(e->e->id, conn_id));
                this->connection_rels.insert(std::make_pair(e->o?e->o->id:0, conn_id));
            }
            for (std::set<cable*>::iterator i = cables.begin(); i != cables.end(); i++) {
                cable *e = (*i);
                preload_info info(e->write_ptr, e->write_size, true);
                this->cables.insert(std::make_pair(e->id, info));
                if (e->p[0]->is_connected()) {
                    this->cable_rels.insert(std::make_pair(e->p[0]->plugged_edev->get_entity()->id, e->id));
                }
                if (e->p[1]->is_connected()) {
                    this->cable_rels.insert(std::make_pair(e->p[1]->plugged_edev->get_entity()->id, e->id));
                }
            }
            for (std::map<uint32_t, group*>::iterator i = groups.begin(); i != groups.end(); i++) {
                group *e = i->second;
                preload_info info(e->write_ptr, e->write_size, true);
                this->groups.insert(std::make_pair(e->id, info));
            }
        }

        b2World::no_wakeups = true;

        for (std::map<uint32_t, entity*>::iterator i = entities.begin();
                i != entities.end(); i++) {
            entity *e = i->second;

            if (e->type == ENTITY_CABLE) {
                tms_fatalf("This should never happen!");
            }

            /* this will destroy all joints created by connections and cables */
            G->remove_entity(e);
            W->remove(e);
        }

        /* clean up leftover cables and stuff */
        for (std::set<cable*>::iterator i = cables.begin(); i != cables.end(); i++) {
            (*i)->joint = 0;
            (*i)->freeze = true;

            //(*i)->disconnect((*i)->p[0]);
            //(*i)->disconnect((*i)->p[1]);

            G->remove_entity(*i);
            W->remove(*i);
            delete (*i);
        }

        for (std::map<uint32_t, group*>::iterator i = groups.begin(); i != groups.end(); i++) {
            G->remove_entity(i->second);
            W->remove(i->second);
            delete (i->second);
        }

        for (std::set<connection*>::iterator i = connections.begin(); i != connections.end(); i++) {
            connection *c = (*i);
            c->e->remove_connection(c);
            c->o->remove_connection(c);
            W->erase_connection(*i);
            c->j = 0;

            if (!c->owned) {
                delete *i;
            }
        }

        for (std::map<uint32_t, entity*>::iterator i = entities.begin();
                i != entities.end(); i++) {
            entity *e = i->second;
            delete e;
        }

        chunk->remove_from_world();
    }
    //tms_debugf("unloading %d,%d", chunk->pos_x, chunk->pos_y);

    size_t write_ptr = this->heap.size;
    this->write_chunk(&W->level, &this->heap, chunk);
    size_t write_size = this->heap.size - write_ptr;
    preload_info info(write_ptr, write_size, true);
    this->chunks.insert(std::make_pair(chunk_pos(chunk->pos_x, chunk->pos_y), info));
    delete chunk;

    b2World::no_wakeups = false;

    return true;
}

/**
 * Load all entities
 **/
void
chunk_preloader::load(level_chunk *c, int phase)
{
    //phase = 1;
    chunk_window *win = W->cwindow;
    chunk_pos cp(c->pos_x, c->pos_y);

    if (phase < c->load_phase) {

    }

    if (phase == 1) {
        c->waste_retry_wait = 10;
    }

    switch (c->load_phase+1) {
        case 1:
            {
                c->load_phase = 1;
                c->loaded_neighbours = false;
                c->garbage = false;

                if (phase == 1) {
                    c->add_to_world();
                }
            }

        case 2:
            {
                if (phase == 1) {
                    break;
                }

                c->load_phase = 2;
                c->loaded_neighbours = false;
                c->garbage = false;

                typedef std::multimap<chunk_pos, uint32_t>::iterator iterator;
                std::pair<iterator, iterator> ip = this->entities_by_chunk.equal_range(cp);

                iterator it = ip.first;
                for (; it != ip.second; it++) {
                    entity *e = this->load_entity(it->second);

                    if (e) {
                        //tms_debugf("loaded in chunk[%d,%d]: %s (%d)", c->pos_x, c->pos_y, e->get_name(),e->id);
                    }
                }

                this->entities_by_chunk.erase(cp);

                c->add_to_world();

                if (phase == 2) {
                    break;
                }
            }
    }
}

void
chunk_preloader::push_loaded()
{
    /* make sure all affected chunks and their noughbours are loaded */
    while (!this->affected_chunks.empty()) {
        chunk_pos cp = this->affected_chunks.back();
        this->affected_chunks.pop_back();

        level_chunk *c = this->get_chunk(cp.x, cp.y, false);
        this->load(c, 2);
    }

    if (this->loaded_entities.size() > 0) {
        W->init_level_entities(&this->loaded_entities);

        /*tms_debugf("pushing %d entities, %d groups, %d connections, %d cables",
                (int)this->loaded_entities.size(),
                (int)this->loaded_groups.size(),
                (int)this->loaded_connections.size(),
                (int)this->loaded_cables.size());*/

        G->add_entities(
                &this->loaded_entities,
                &this->loaded_groups,
                &this->loaded_connections,
                &this->loaded_cables
                );

        this->loaded_entities.clear();
        this->loaded_groups.clear();
        this->loaded_connections.clear();
        this->loaded_cables.clear();
    }
}

cable*
chunk_preloader::load_cable(uint32_t id)
{
    std::map<uint32_t, preload_info>::iterator i = this->cables.find(id);
    cable *c;

    if (i != this->cables.end()) {
        preload_info info = i->second;
        this->cables.erase(id);
        c = this->read_cable(info);
    } else
        c = 0;

    return c;
}

cable*
chunk_preloader::read_cable(preload_info i)
{
    cable *c;

    lvlbuf b = i.heap ? this->heap : this->w_lb;
    b.rp = i.ptr;

    c = W->load_cable(&b, W->level.version, W->level.flags, 0, b2Vec2(0,0), 0);

    if (c) {
        this->loaded_cables.insert(c);
    }

    return c;
}

/**
 * Read and load a connection
 * The world will automatically make sure both relevant entities are loaded by calling back
 * to the preloader
 **/
connection*
chunk_preloader::read_connection(size_t conn_id)
{
    connection *c;

    tms_debugf("reading connection %p", (void*)conn_id);

    /* make sure a connection is only loaded once */
    std::map<size_t, preload_info>::iterator i = this->connections.find(conn_id);
    if (i == this->connections.end()) {
        tms_debugf("connection is already loaded or does not exist");
        return 0;
    }

    preload_info info = i->second;
    lvlbuf b = info.heap ? this->heap : this->w_lb;
    b.rp = info.ptr;

    this->connections.erase(conn_id);

    c = W->load_connection(&b, W->level.version, W->level.flags, 0, b2Vec2(0,0), 0);

    if (c) {
        tms_debugf("world created connection from %p", (void*)info.ptr);
        this->loaded_connections.insert(c);
    }

    return c;
}

entity*
chunk_preloader::read_entity(preload_info info)
{
    lvlbuf b = info.heap ? this->heap : this->w_lb;
    b.rp = info.ptr;

    entity *e = W->load_entity(&b, W->level.version, 0, b2Vec2(0,0), 0, &this->affected_chunks);

    //tms_debugf("read entity at %p, got %p with id %u", (void*)info.ptr, e, e?e->id:0);

    if (e) {
        this->loaded_entities.insert(std::make_pair(e->id, e));
    }

    return e;
}

entity*
chunk_preloader::load_entity(uint32_t id)
{
    entity *e = 0;
    std::map<uint32_t, preload_info>::iterator i = this->entities.find(id);

    if (i != this->entities.end()) {
        preload_info info = i->second;
        this->entities.erase(i);

        e = this->read_entity(info);

        if (e) {
            /* load all connections this entity has */

            bool retry = true; /* see XXX comment below */
#ifdef DEBUG
            int retries = 0;
#endif

            do {
                typedef std::multimap<uint32_t, size_t>::iterator iterator;
                std::pair<iterator, iterator> ip = this->connection_rels.equal_range(e->id);

                iterator it = ip.first;
                retry = false;
                for (; it != ip.second; it++) {
                    //tms_debugf("entity %u, force load of connection at %p", id, (void*)it->second);
                    connection *e = this->read_connection(it->second);

                    /* XXX IMPORTANT: when this entity loads a connection, that connection
                     * might load other entities. This might cause the connection_rels
                     * map to change, invalidating the iterator we have. That is why we
                     * need to restart everything every time a connection is loaded.
                     */

                    if (e) {
#ifdef DEBUG
                        retries ++;
#endif
                        retry = true;
                        break;
                    }
                }

                if (!retry)
                    this->connection_rels.erase(e->id);

            } while (retry);

#ifdef DEBUG
            //tms_debugf("loaded all entity connections for %u with %d retries", id, retries);
            retries = 0;
#endif

            /* now do exactly the same thing as above but for cables */
            do {
                typedef std::multimap<uint32_t, uint32_t>::iterator iterator;
                std::pair<iterator, iterator> ip = this->cable_rels.equal_range(e->id);

                iterator it = ip.first;
                retry = false;
                for (; it != ip.second; it++) {
                    tms_debugf("entity %u, force load of cable %u", id, it->second);
                    cable *e = this->load_cable(it->second);

                    if (e) {
#ifdef DEBUG
                        retries ++;
#endif
                        retry = true;
                        break;
                    }
                }

                if (!retry)
                    this->cable_rels.erase(e->id);

            } while (retry);

#ifdef DEBUG
            //tms_debugf("loaded all cables for %u with %d retries", id, retries);
#endif

        }
    }

    return e;
}

group*
chunk_preloader::read_group(preload_info i)
{
    lvlbuf b = i.heap ? this->heap : this->w_lb;
    b.rp = i.ptr;

    group *g = W->load_group(&b, W->level.version, 0, b2Vec2(0.f, 0.f), 0);

    if (g) {
        this->loaded_groups.insert(std::make_pair(g->id, g));
    }

    return g;
}

group*
chunk_preloader::load_group(uint32_t id)
{
    group *g = 0;
    std::map<uint32_t, preload_info>::iterator i = this->groups.find(id);

    if (i != this->groups.end()) {
        g = this->read_group(i->second);

        this->groups.erase(i);
    }

    //tms_debugf("preloader:loaded group %d (%p)", id, g);

    return g;
}

level_chunk *
chunk_preloader::read_chunk(preload_info i)
{
    lvlbuf bb = i.heap ? this->heap : this->w_lb;
    lvlbuf *b = &bb;
    b->rp = i.ptr;

    int pos_x = b->r_int32();
    int pos_y = b->r_int32();
    int generate_phase = b->r_uint8();
    level_chunk *c = new level_chunk(pos_x,pos_y);
    c->generate_phase = generate_phase;

#ifdef DEBUG_SPECIFIC_CHUNK
    if (c->pos_x == DEBUG_CHUNK_X && c->pos_y == DEBUG_CHUNK_Y) {
        tms_debugf("(chunk %d,%d) reading chunk, phase: %d",
                c->pos_x, c->pos_y,
                c->generate_phase);
    }
#endif

    uint8_t load_method = b->r_uint8();

    if (c->generate_phase < 5) {
        for (int x=0; x<GENSLOT_SIZE_X; x++) {
            for (int y=0; y<GENSLOT_SIZE_Y; y++) {
                for (int z=0; z<2; z++) {
                    uint32_t gt_id = b->r_uint32();

                    if (gt_id == 0) {
                        continue;
                    }

                    std::map<uint32_t, gentype*>::iterator i = this->gentypes.find(gt_id);

                    if (i != this->gentypes.end() && !i->second->lock) {
                        int _z = z;
                        if (i->second->sorting != z) {
                            tms_warnf("This should NEVER happen!!! ErRorIrOrkldjklfaf noooooooooooo!");
                            _z = i->second->sorting;
                        }
                        c->genslots[x][y][_z] = i->second;
                    } else {
                        //tms_errorf("invalid gentype id %u", gt_id);
                        //c->genslots[x][y][z] = 0;
                    }
                }
            }
        }
    }

    switch (load_method) {
        case CHUNK_LOAD_MERGED:
            {
                for (int m=0; m<3; m++) {
                    c->num_merged[m] = b->r_uint8();

                    if (W->level.version < LEVEL_VERSION_1_5_1) {
                        for (int x=0; x<c->num_merged[m]; x++) {
                            c->merged[m][x] = tpixel_desc();
                            b->r_buf((char*)&c->merged[m][x], sizeof(struct tpixel_desc_1_5));
                        }
                    } else {
                        b->r_buf((char*)c->merged[m], c->num_merged[m]*sizeof(struct tpixel_desc));
                    }
                }

                c->update_pixel_buffer();
                c->update_heights();
            }
            break;

        case CHUNK_LOAD_PIXELS:
            {
                /* load by pixel values */
                for (int m=0; m<3; m++) {
                    c->num_merged[m] = 0;
                }
                b->r_buf((char*)c->pixels, sizeof(uint8_t)*3*16*16);
                c->update_heights();
            }
            break;

        case CHUNK_LOAD_EMPTY:
            break;
    }

    return c;
}

void
chunk_preloader::read_chunks(lvlinfo *lvl, lvlbuf *lb)
{
    int num_chunks = lvl->num_chunks, n_read;
    //tms_debugf("reading %d chunks", num_chunks);

    //tms_debugf("read chunks at pos %u", lb->rp);

    for (n_read = 0; (!lb->eof() && n_read < num_chunks); n_read++) {
        for (int x=0; x<num_chunks; x++) {
            size_t ptr = lb->rp;

            int pos_x = lb->r_int32();
            int pos_y = lb->r_int32();
            int generate_phase = lb->r_uint8();

            uint8_t load_method = lb->r_uint8();

            bool soft = (lvl->flags & LVL_CHUNKED_LEVEL_LOADING);

            level_chunk *c = this->get_chunk(pos_x, pos_y, soft);

            //tms_debugf("read chunk at %d %d, already found? %p", pos_x, pos_y, c)

            if (c) {
#ifdef DEBUG_SPECIFIC_CHUNK
                if (c->pos_x == DEBUG_CHUNK_X && c->pos_y == DEBUG_CHUNK_Y) {
                    tms_debugf("(chunk %d, %d) reading chunk, phase: %d, read phase: %d",
                            c->pos_x, c->pos_y,
                            c->generate_phase, generate_phase);
                }
#endif

                c->generate_phase = generate_phase;

                if (c->generate_phase < 5) {
                    for (int x=0; x<GENSLOT_SIZE_X; x++) {
                        for (int y=0; y<GENSLOT_SIZE_Y; y++) {
                            for (int z=0; z<2; z++) {
                                uint32_t gt_id = lb->r_uint32();
                                std::map<uint32_t, gentype*>::iterator i = this->gentypes.find(gt_id);

                                if (i != this->gentypes.end()) {
                                    c->genslots[x][y][z] = i->second;
                                } else {
                                    //tms_errorf("invalid gentype id %u", gt_id);
                                }
                            }
                        }
                    }
                }

                switch (load_method) {
                    case CHUNK_LOAD_MERGED:
                        for (int m=0; m<3; m++) {
                            c->num_merged[m] = lb->r_uint8();

                            if (W->level.version < LEVEL_VERSION_1_5_1) {
                                for (int x=0; x<c->num_merged[m]; x++) {
                                    c->merged[m][x] = tpixel_desc();
                                    lb->r_buf((char*)&c->merged[m][x], sizeof(struct tpixel_desc_1_5));
                                }
                            } else {
                                lb->r_buf((char*)c->merged[m], c->num_merged[m]*sizeof(struct tpixel_desc));
                            }
                        }
                        c->update_pixel_buffer();
                        break;

                    case CHUNK_LOAD_PIXELS:
                        lb->r_buf((char*)c->pixels, sizeof(uint8_t)*3*16*16);
                        break;
                }

            } else {
                chunk_pos cp(pos_x, pos_y);

#ifdef DEBUG_SPECIFIC_CHUNK
                if (cp.x == DEBUG_CHUNK_X && cp.y == DEBUG_CHUNK_Y) {
                    tms_debugf("(chunk %d,%d) preloading chunk, phase: %d", cp.x, cp.y, generate_phase);
                }
#endif

                if (generate_phase < 5) {
                    lb->rp += sizeof(uint32_t)*GENSLOT_SIZE_X*GENSLOT_SIZE_Y*2;
                }

                switch (load_method) {
                    case CHUNK_LOAD_MERGED:
                        /* skip num merged */
                        for (int m=0; m<3; m++) {
                            uint32_t skip = lb->r_uint8();

                            if (W->level.version < LEVEL_VERSION_1_5_1) {
                                lb->rp += skip*sizeof(struct tpixel_desc_1_5);
                            } else {
                                lb->rp += skip*sizeof(struct tpixel_desc);
                            }
                        }
                        break;

                    case CHUNK_LOAD_PIXELS:
                        lb->rp += sizeof(uint8_t)*3*16*16;
                        break;
                }

                size_t size = lb->rp - ptr;
                //tms_debugf("reading unloaded chunk %d %d size %u", pos_x, pos_y, (int)size);
                this->chunks.insert(std::make_pair(cp, preload_info(ptr, size)));
            }
        }
    }
}

void
chunk_preloader::write_chunk(lvlinfo *lvl, lvlbuf *lb, level_chunk *c)
{
    lb->w_s_int32(c->pos_x);
    lb->w_s_int32(c->pos_y);
    lb->w_s_uint8(c->generate_phase);

    uint8_t load_method = (c->num_merged[0] || c->num_merged[1] || c->num_merged[2])
                           ? CHUNK_LOAD_MERGED
                           : CHUNK_LOAD_PIXELS;

    if (load_method == CHUNK_LOAD_PIXELS) {
        bool has_pixel = false;
        for (int z=0; z<3 && !has_pixel; z++) {
            for (int y=0; y<16 && !has_pixel; y++) {
                for (int x=0; x<16; x++) {
                    if (c->pixels[z][y][x] != 0) {
                        has_pixel = true;
                        break;
                    }
                }
            }
        }

        if (!has_pixel) {
            load_method = CHUNK_LOAD_EMPTY;
        }
    }

    lb->w_s_uint8(load_method);

    if (c->generate_phase < 5) {
        for (int x=0; x<GENSLOT_SIZE_X; x++) {
            for (int y=0; y<GENSLOT_SIZE_Y; y++) {
                for (int z=0; z<2; z++) {
                    if (c->genslots[x][y][z]) {
                        gentype *gt = c->genslots[x][y][z];
                        lb->w_s_uint32(gt->id);
                    } else {
                        lb->w_s_uint32(0);
                    }
                }
            }
        }
    }

    switch (load_method) {
        case CHUNK_LOAD_MERGED:
            for (int x=0; x<3; x++) {
                lb->w_s_uint8(c->num_merged[x]);
                if (W->level.version < LEVEL_VERSION_1_5_1) {
                    for (int y=0; y<c->num_merged[x]; y++) {
                        lb->w_s_buf((const char*)&c->merged[x][y], sizeof(struct tpixel_desc_1_5));
                    }
                } else {
                    lb->w_s_buf((const char *)c->merged[x], c->num_merged[x]*sizeof(struct tpixel_desc));
                }
            }
            break;

        case CHUNK_LOAD_PIXELS:
            lb->w_s_buf((const char*)c->pixels, sizeof(uint8_t)*3*16*16);
            break;
    }
}

void
chunk_preloader::prepare_write()
{
    /* clear all invalidated gentypes */
    for (std::map<uint32_t, gentype*>::iterator i = this->gentypes.begin();
            i != this->gentypes.end(); ) {
        gentype *gt = i->second;

        if (gt->transaction.state != TERRAIN_TRANSACTION_OCCUPIED) {
            tms_assertf(gt->transaction.state == TERRAIN_TRANSACTION_APPLIED
                    || gt->transaction.state == TERRAIN_TRANSACTION_INVALIDATED,
                    "terrain transaction state was %d during prepare_write()", gt->transaction.state);

            this->gentypes.erase(i++);

            gt->lock = true; /* prevent the gentype from touching our gentypes map */
            delete gt;
        } else {
            i++;
        }
    }

}

/**
 * Write all pending gentypes
 **/
void
chunk_preloader::write_gentypes(lvlinfo *lvl, lvlbuf *lb)
{
    tms_debugf("writing %d gentypes", (int)this->gentypes.size());
    for (std::map<uint32_t, gentype*>::iterator i = this->gentypes.begin();
            i != this->gentypes.end(); i ++) {
        gentype *gt = i->second;

        /* write gentype type and ID */
        tms_debugf("writing gentype %u %u", gt->type, gt->id);
        lb->w_s_uint32(gt->type);
        lb->w_s_uint32(gt->id);
        lb->w_s_uint8(gt->sorting);
        lb->w_s_float(gt->priority);
        lb->w_s_int32(gt->coord.chunk_x);
        lb->w_s_int32(gt->coord.chunk_y);
        lb->w_s_uint8(gt->coord._xy);
        lb->w_s_int32(gt->transaction.start_x);
        lb->w_s_int32(gt->transaction.start_y);

        tms_assertf(gt->transaction.state == TERRAIN_TRANSACTION_OCCUPIED, "not occupied! %d", gt->transaction.state);

        uint32_t num_mods = gt->transaction.modifications.size();
        lb->w_s_uint32(num_mods);

        for (std::multimap<terrain_coord, terrain_edit>::iterator i = gt->transaction.modifications.begin();
               i != gt->transaction.modifications.end();
               i ++) {
            terrain_coord c = i->first;
            terrain_edit e = i->second;
            lb->w_s_int32(c.chunk_x);
            lb->w_s_int32(c.chunk_y);
            lb->w_s_uint8(c._xy);

            lb->w_s_uint32(e.flags);
            lb->w_s_int32(e.data);
        }

        lb->w_s_uint32(gt->genslots.size());
        for (std::vector<genslot>::iterator i = gt->genslots.begin(); i != gt->genslots.end(); i ++) {
            genslot g = (*i);
            lb->w_s_int32(g.chunk_x);
            lb->w_s_int32(g.chunk_y);
            lb->w_s_uint8(g.slot_x);
            lb->w_s_uint8(g.slot_y);
            lb->w_s_uint8(g.sorting);
        }

        gt->write_state(lvl, lb);
    }
}

/**
 * Read all gentypes
 **/
void
chunk_preloader::read_gentypes(lvlinfo *lvl, lvlbuf *lb)
{
    int n_read = 0, num_gentypes = lvl->num_gentypes;

    while (!lb->eof() && n_read < num_gentypes) {
        uint32_t type;

        type = lb->r_uint32();
        tms_debugf("reading gentype %u", type);

        gentype *gt = gentype::gentypes[type].allocate();

        gt->id = lb->r_uint32();

        if (gt->id >= _gentype_id) {
            _gentype_id = gt->id+1;
        }

        gt->type = type;
        tms_debugf("reading gentype id %u", gt->id);

        gt->sorting = lb->r_uint8();
        gt->priority = lb->r_float();
        gt->coord.chunk_x = lb->r_int32();
        gt->coord.chunk_y = lb->r_int32();
        gt->coord._xy = lb->r_uint8();
        gt->transaction.start_x = lb->r_int32();
        gt->transaction.start_y = lb->r_int32();
        gt->transaction.state = TERRAIN_TRANSACTION_OCCUPIED;

        int num_modifications = lb->r_uint32();
        for (int x=0; x<num_modifications; x++) {
            terrain_coord c;
            terrain_edit e;

            c.chunk_x = lb->r_int32();
            c.chunk_y = lb->r_int32();
            c._xy = lb->r_uint8();

            e.flags = lb->r_uint32();
            e.data = lb->r_int32();

            gt->transaction.modifications.insert(std::make_pair(c,e));
        }

        int num_genslots = lb->r_uint32();
        for (int x=0; x<num_genslots; x++) {
            genslot g;
            g.chunk_x = lb->r_int32();
            g.chunk_y = lb->r_int32();
            g.slot_x = lb->r_uint8();
            g.slot_y = lb->r_uint8();
            g.sorting = lb->r_uint8();
            gt->genslots.push_back(g);
        }

        gt->read_state(lvl, lb);

        this->gentypes.insert(std::make_pair(gt->id, gt));

        n_read ++;
    }
}

/**
 * Write all loaded and unloaded chunks
 **/
void
chunk_preloader::write_chunks(lvlinfo *lvl, lvlbuf *lb)
{
    tms_debugf("WRITE chunks at pos %u", (int)lb->size);

    for (std::map<chunk_pos, level_chunk*>::iterator i = this->active_chunks.begin();
            i != this->active_chunks.end(); i++) {
        level_chunk *c = i->second;

        tms_debugf("writing active chunk %d %d",c->pos_x, c->pos_y);
        this->write_chunk(lvl, lb, c);
    }

    for (std::map<chunk_pos, level_chunk*>::iterator i = this->wastebin.begin();
            i != this->wastebin.end(); i++) {
        level_chunk *c = i->second;

        tms_debugf("writing wastebin chunk %d %d",c->pos_x, c->pos_y);
        this->write_chunk(lvl, lb, c);
    }

    /* write unloaded chunks */
    for (std::map<chunk_pos, preload_info>::iterator i = this->chunks.begin(); i != this->chunks.end(); i++) {
        preload_info info = i->second;
        lvlbuf *r_lb = info.heap ? &this->heap : &this->w_lb;
        //lvlbuf *r_lb = &this->w_lb;

        lb->ensure(info.size);
        tms_debugf("writing unloaded chunk %d %d of size %u", i->first.x, i->first.y, (int)info.size);
        lb->w_s_buf((const char*)r_lb->buf+info.ptr, info.size);
    }
}

void
chunk_preloader::write_groups(lvlinfo *lvl, lvlbuf *lb)
{
    for (std::map<uint32_t, preload_info>::iterator i = this->groups.begin(); i != this->groups.end(); i++) {
        preload_info info = i->second;
        lvlbuf *r_lb = info.heap ? &this->heap : &this->w_lb;

        lb->ensure(info.size);
        lb->w_buf((const char*)r_lb->buf+info.ptr, info.size);
    }
    tms_debugf("preloader wrote %d groups", (int)this->groups.size());
}

void
chunk_preloader::write_entities(lvlinfo *lvl, lvlbuf *lb)
{
    for (std::map<uint32_t, preload_info>::iterator i = this->entities.begin(); i != this->entities.end(); i++) {
        preload_info info = i->second;
        lvlbuf *r_lb = info.heap ? &this->heap : &this->w_lb;
        lb->ensure(info.size);
        lb->w_buf((const char*)r_lb->buf+info.ptr, info.size);
    }
    tms_debugf("preloader wrote %d entities", (int)this->entities.size());
}

void
chunk_preloader::write_cables(lvlinfo *lvl, lvlbuf *lb)
{
    for (std::map<uint32_t, preload_info>::iterator i = this->cables.begin(); i != this->cables.end(); i++) {
        preload_info info = i->second;
        lvlbuf *r_lb = info.heap ? &this->heap : &this->w_lb;
        lb->ensure(info.size);
        lb->w_buf((const char*)r_lb->buf+info.ptr, info.size);
    }
    tms_debugf("preloader wrote %d cables", (int)this->cables.size());
}

void
chunk_preloader::write_connections(lvlinfo *lvl, lvlbuf *lb)
{
    for (std::map<size_t, preload_info>::iterator i = this->connections.begin(); i != this->connections.end(); i++) {
        preload_info info = i->second;
        lvlbuf *r_lb = info.heap ? &this->heap : &this->w_lb;
        lb->ensure(info.size);
        lb->w_buf((const char*)r_lb->buf+info.ptr, info.size);
    }
    tms_debugf("preloader wrote %d connections", (int)this->connections.size());
}

void
chunk_preloader::require_chunk_neighbours(level_chunk *c)
{
    int xx[8] = {-1,0,1,
                 -1,  1,
                 -1,0,1};
    int yy[8] = { 1,1,1,
                  0,  0,
                 -1,-1,-1};

    for (int x=0; x<8; x++) {
        if (!c->neighbours[x]) {
            c->neighbours[x] = this->get_chunk(c->pos_x+xx[x], c->pos_y+yy[x], false);
            c->neighbours[x]->neighbours[7-x] = c;
        }
    }
}

/**
 * Always returns a valid chunk for the given chunk coordinates.
 * Will create an empty chunk and return it if the chunk hasn't been previously loaded
 *
 * if soft is set to true, a null chunk can be returned
 **/
level_chunk*
chunk_preloader::get_chunk(int x, int y, bool soft/*=false*/, bool load/*=true*/)
{
    chunk_pos p(x,y);
    level_chunk *c;

    std::map<chunk_pos, level_chunk*>::iterator i;
    std::map<chunk_pos, preload_info>::iterator pre_i;

    if ((i = this->active_chunks.find(p)) != this->active_chunks.end()) {
        return i->second;
    }

    if (soft) {
        return 0;
    }

    if ((i = this->wastebin.find(p)) != this->wastebin.end()) {
        i->second->garbage = false;
        //this->wastebin.erase(i);
        //this->active_chunks.insert(std::make_pair(p, c));
        return i->second;
    }

    if (!load) {
        return 0;
    }

    if ((pre_i = this->chunks.find(p)) != this->chunks.end()) {
        /* chunk was found in the preloader, we load it */
        c = this->read_chunk(pre_i->second);
        this->chunks.erase(pre_i);

        c->init_chunk_neighbours();
        this->active_chunks.insert(std::make_pair(p, c));
        return c;
    }

#ifdef DEBUG_PRELOADER_SANITY
    tms_assertf(abs(x) < 1000 && abs(y) < 1000, "get_chunk() suspicious position %d %d", x, y);
#endif

    c = new level_chunk(x,y);
    c->init_chunk_neighbours();
    this->active_chunks.insert(std::make_pair(p, c));

    return c;
}

const std::map<chunk_pos, level_chunk*>&
chunk_preloader::get_active_chunks()
{
    return this->active_chunks;
}
