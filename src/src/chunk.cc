#include "chunk.hh"
#include "game.hh"
#include "model.hh"
#include "group.hh"
#include "terrain.hh"
#include "gentype.hh"

/* chunked loading/unloading
 *
 * physics grid, fixed AxB num chunks
 * render grid, selected from physics grid using cam borders
 *
 * CHUNK LOADING
 * - generate if the chunk hasn't been generated
 * - add entities to world
 *
 * CHUNK UNLOADING
 * - can the chunk be unloaded?
 *   - chunk is outside of physics grid
 *   - no active bodies except entities that are marked for free unloading
 *   - unload timer reached, timer should be longer if the chunk is unloaded/loaded often
 *
 * DYNAMIC RELOADING
 * If we shoot a rocket that goes outside the physics grid, how to handle it?
 *
 * Solution #1: Deactive bodies that go outside the physics grid
 *
 * Solution #2: Track bodies outside the physics grid and load surrounding chunks
 *
 * Solution #3: Track chunks with active bodies inside, load surrounding chunks
 *
 * #3 is probably the fastest. But how do we track active bodies inside every chunk?
 * One option is to let every chunk have a sensor covering its entire area, and let it count
 * all fixtures entering and leaving.
 *
 * Using sensors can get very slow if we have very many fixtures inside it though,
 * because box2d will perform collision testing of all shapes with the sensor
 * shape every tick, every shape in the world will always collide with something.
 *
 * A slight optimization would be to remove the sensor shape if all surrounding chunks
 * are active as well, because then we don't have to track what's inside, but only what
 * leaves the chunk.
 *
 * We could also put 4 sensors in each chunk, one for every dir. As long as the sensor
 * is wider than b2_maxTranslation it should not be a problem.
 *
 *
 *
 **/

bool operator <(const chunk_pos& lhs, const chunk_pos &rhs)
{
    if (lhs.x != rhs.x)
        return lhs.x < rhs.x;
    else
        return lhs.y < rhs.y;
}

bool operator <(const genslot& lhs, const genslot &rhs)
{
    if (lhs.chunk_x != rhs.chunk_x) {
        return lhs.chunk_x < rhs.chunk_x;
    } else {
        if (lhs.chunk_y != rhs.chunk_y) {
            return lhs.chunk_y < rhs.chunk_y;
        } else {
            if (lhs.slot_x != rhs.slot_x) {
                return lhs.slot_x < rhs.slot_x;
            } else if (lhs.slot_y != rhs.slot_y) {
                return lhs.slot_y < rhs.slot_y;
            }
        }
    }

    return lhs.sorting < rhs.sorting;
}

bool operator ==(const genslot& lhs, const genslot &rhs)
{
    return lhs.chunk_x == rhs.chunk_x
           && lhs.chunk_y == rhs.chunk_y
           && lhs.slot_x == rhs.slot_x
           && lhs.slot_y == rhs.slot_y
           && lhs.sorting == rhs.sorting;
}

level_chunk::~level_chunk()
{
    this->clear_chunk_neighbours();

    for (int x=0; x<3; ++x) {
        tms_entity_uninit(&this->layer_entities[x]);
    }

    for (int x=0; x<2; ++x) {
        tms_entity_uninit(&this->grass_entity[x]);
    }
#if 0
    for (int x=0; x<GENSLOT_SIZE_X; ++x) {
        for (int y=0; y<GENSLOT_SIZE_Y; ++y) {
            for (int z=0; z<2; z++) {
                gentype *gt = this->genslots[x][y][z];

                if (!gt) {
                    continue;
                }

#ifdef DEBUG
                tms_fatalf("ERROR deleting chunk with an unapplied gentype");
                fflush(stdout);
                fflush(stderr);
                //SDL_Delay(5000000);
#endif
                delete gt;
            }
        }
    }
#endif
}

/**
 * Read merged[] and convert it to a pixel buffer,
 * this is used when a chunk has been loaded from a file
 **/
void
level_chunk::update_pixel_buffer()
{
    memset(this->pixels, 0, sizeof(this->pixels));

    for (int z=0; z<3; z++) {
        this->min_merged[z] = this->num_merged[z];

        for (int m=0; m<this->num_merged[z]; m++) {
            struct tpixel_desc desc = this->merged[z][m];

            if (desc.hp <= 0.f) {
                if (m < this->min_merged[z]) {
                    this->min_merged[z] = m;
                }
                continue;
            }

            int x = (desc.pos & 15);
            int y = desc.pos >> 4;
            int size = (1<<desc.size);
            int mat = tpixel_mat_to_chunk_mat(desc.material);

#ifdef DEBUG
            if (size > (1<<3)) size = (1<<3);
#endif

            for (int py=y; py<y+size; py++) {
                for (int px=x; px<x+size; px++) {
                    this->pixels[z][py][px] = mat;
                }
            }
        }
    }
}

/**
 * Update the heights buffer
 **/
void
level_chunk::update_heights()
{
    float *heights = W->cwindow->get_heights(this->pos_x);

    for (int x=0; x<16; x++) {
        for (int y=15; y>=0; y--) {
            if (this->pixels[0][y][x]) {
                terrain_coord c;
                c.set_from_global(this->pos_x*16+x, this->pos_y*16+y);

                float h = c.get_world_y();
                if (h > heights[x]) {
                    heights[x] = h;
                    break;
                }
            }
        }
    }
}

void
level_chunk::clear_chunk_neighbours()
{
    for (int x=0; x<8; x++) {
        if (this->neighbours[x]) {
            this->neighbours[x]->neighbours[7-x] = 0;
            this->neighbours[x] = 0;
        }
    }
}

void
level_chunk::init_chunk_neighbours()
{
    int xx[8] = {-1,0,1,
                 -1,  1,
                 -1,0,1};
    int yy[8] = { 1,1,1,
                  0,  0,
                 -1,-1,-1};

    for (int x=0; x<8; x++) {
        if (!this->neighbours[x]) {
            this->neighbours[x] = W->cwindow->preloader.get_chunk(this->pos_x+xx[x], this->pos_y+yy[x], true);

            if (this->neighbours[x]) {
                this->neighbours[x]->neighbours[7-x] = this;
            } else {
                //tms_fatalf("DAAAMN CUTIE");
            }
        }
    }
}

void
level_chunk::load_neighbours()
{
    if (this->loaded_neighbours) {
        //return;
    }

    if (this->num_fixtures > 0) {
        W->cwindow->preloader.require_chunk_neighbours(this);

        for (int x=0; x<8; x++) {
            W->cwindow->preloader.load(this->neighbours[x], 2);
        }

        this->loaded_neighbours = true;
    } else if (this->num_dyn_fixtures > 0) {
        W->cwindow->preloader.require_chunk_neighbours(this);

        W->cwindow->preloader.load(this, 2);

        for (int x=0; x<8; x++) {
            W->cwindow->preloader.load(this->neighbours[x], 1);
        }

        this->loaded_neighbours = true;
    }
}

void
level_chunk::add_fixture(b2Fixture *fx, entity *owner)
{
    if (owner->flag_active(ENTITY_DYNAMIC_UNLOADING)) {
        if (owner->g_id != O_PLANT) {
            this->num_dyn_fixtures ++;
        }
    } else {
        this->num_fixtures ++;

        if (this->garbage) {
            this->garbage = 0;
        }
    }
}

void
level_chunk::remove_fixture(b2Fixture *fx, entity *owner)
{
    if (owner->flag_active(ENTITY_DYNAMIC_UNLOADING)) {
        if (owner->g_id != O_PLANT) {
            this->num_dyn_fixtures --;
        }
    } else {
        this->num_fixtures --;
    }
}

bool
level_chunk::is_focused()
{
    chunk_window *win = W->cwindow;
    if (this->pos_y < win->y || this->pos_y > win->y+win->h
        || this->pos_x < win->x || this->pos_x > win->x+win->w) {
        return false;
    }

    return true;
}

bool
level_chunk::occupy_pixel(int local_x, int local_y, gentype *gt)
{
    int cisx = std::abs(local_x % GENSLOT_SIZE_X);
    int cisy = std::abs(local_y % GENSLOT_SIZE_Y);
    gentype *previous = 0;

    if (gt->lock) {
        return false;
    }

    tms_assertf(this->generate_phase < 5, "can't occupy a chunk of gen phase %d", this->generate_phase);

    if (this->genslots[cisx][cisy][gt->sorting] != 0) {
        if (this->genslots[cisx][cisy][gt->sorting] == gt) {
            return true;
        }

        previous = this->genslots[cisx][cisy][gt->sorting];

        //tms_debugf("found previous %p", previous);

        if (previous->transaction.state != TERRAIN_TRANSACTION_INVALIDATED) {
            //tms_debugf("Slot at %d/%d[%d/%d] is occupied by %p", local_x, local_y, cisx, cisy, this->genslots[cisx][cisy][gt->sorting]);
            if (previous->priority > gt->priority) {
                return false;
            } else if (previous->priority == gt->priority) {
                if (std::abs(previous->coord.get_world_x()) < std::abs(gt->coord.get_world_x())) {
                    return false;
                } else if (std::abs(previous->coord.get_world_x()) == std::abs(gt->coord.get_world_x())) {
                    if (std::abs(previous->coord.get_world_y()) < std::abs(gt->coord.get_world_y())) {
                        return false;
                    }
                }
            }

            previous->transaction.state = TERRAIN_TRANSACTION_INVALIDATED;
        }

        /* the following should not be necessary */
#if 0
        std::vector<genslot>::iterator i = std::find(previous->genslots.begin(),
                previous->genslots.end(), genslot(this->pos_x, this->pos_y, cisx, cisy, gt->sorting));
        if (i != previous->genslots.end()) {
            previous->genslots.erase(i);
        } else {
            tms_warnf("what? previous gentype did not have a genslot for this");
        }
#endif
    }

#ifdef DEBUG_SPECIFIC_CHUNK
    if (this->pos_x == DEBUG_CHUNK_X && this->pos_y == DEBUG_CHUNK_Y) {
        tms_debugf("(chunk %d,%d) occupied slot %d %d, sorting %d",
                DEBUG_CHUNK_X, DEBUG_CHUNK_Y,
                cisx, cisy, gt->sorting);
    }
#endif

    this->genslots[cisx][cisy][gt->sorting] = gt;
    gt->genslots.push_back(genslot(this->pos_x, this->pos_y, cisx, cisy, gt->sorting));
    return true;
}

void
level_chunk::add_to_world()
{
    if (!this->body) {
        b2BodyDef bd;
        bd.type = b2_staticBody;
        bd.position = b2Vec2(this->pos_x*8.f, this->pos_y*8.f);
        bd.angle = 0.f;
        this->body = W->b2->CreateBody(&bd);

        if ((W->level.flags & LVL_CHUNKED_LEVEL_LOADING) || W->is_paused()) {
            b2FixtureDef fd;
            fd.restitution = 0.f;
            fd.friction = FLT_EPSILON;
            fd.density = 1.f;
            fd.isSensor = true;
            fd.filter = world::get_filter_for_multilayer(15, 15, 15);
            b2PolygonShape p;
            p.SetAsBox(4.f, 4.f, b2Vec2(4.f, 4.f), 0.f);
            fd.shape = &p;

            b2Fixture *sensor = this->body->CreateFixture(&fd);
            sensor->SetUserData(this);
            sensor->SetUserData2(0);
        }
    }

    if (this->load_phase >= 1) {
        this->generate(W->cwindow, 5);
        this->recreate_fixtures(true);
    }
    if (this->load_phase >= 2) {
        this->generate(W->cwindow, 6);
    }
}

void
level_chunk::recreate_fixtures(bool initial)
{
    if (!this->body) {
        return;
    }

    for (b2Fixture *f = this->body->GetFixtureList(); f;) {
        b2Fixture *n = f->GetNext();
        if (!f->IsSensor())
            this->body->DestroyFixture(f);
        f = n;
    }

    for (int z=0; z<3; z++) {
        for (int px=0; px<this->num_merged[z]; px++) {
            struct tpixel_desc desc = this->merged[z][px];

            if (desc.hp <= 0.f) continue;

            int x = (desc.pos & 15);
            int y = desc.pos >> 4;
            int size = desc.size;

            float offs = 0.f;
            if (size>0)
                offs = .25f*(1<<(size)) - .25f;

            /* create shape */
            {
                b2PolygonShape e;

                if (desc.desc_1_5_1.half == 0) {
                    e.SetAsBox((float)(1 << size)*.25f, (float)(1 << size)*.25f, b2Vec2(
                                (x/*+c->pos_x*16*/)*.5 + offs,
                                (y/*+c->pos_y*16*/)*.5 + offs
                                ), 0);
                } else {
                    int vert_count = 3;
                    b2Vec2 verts[4];
                    float sz = .25f;
                    switch (desc.desc_1_5_1.half-1) {
                        case 0: verts[0]=b2Vec2(sz, sz); verts[1]=b2Vec2(-sz, sz); verts[2]=b2Vec2(-sz, -sz); break;
                        case 1: verts[0]=b2Vec2(-sz, sz); verts[1]=b2Vec2(-sz, -sz); verts[2]=b2Vec2(sz, -sz); break;
                        case 2: verts[0]=b2Vec2(-sz, -sz); verts[1]=b2Vec2(sz, -sz); verts[2]=b2Vec2(sz, sz); break;
                        case 3: verts[0]=b2Vec2(sz, -sz); verts[1]=b2Vec2(sz, sz); verts[2]=b2Vec2(-sz, sz); break;

                        default:
                            /* Something went wrong! */
                            tms_errorf("Something when wrong when creating shape for a pixel! desc.desc_1_5_1.half-1: %d", desc.desc_1_5_1.half-1);
                            desc.desc_1_5_1.half = 0;
                            verts[0].Set(-sz, -sz);
                            verts[1].Set( sz, -sz);
                            verts[2].Set( sz,  sz);
                            verts[3].Set(-sz,  sz);
                            vert_count = 4;
                            break;
                    }
                    for (int v=0; v<vert_count; ++v) {
                        verts[v].x += x*.5f+offs;
                        verts[v].y += y*.5f+offs;
                    }
                    e.Set(verts, vert_count);
                }

                b2FixtureDef fd;
                fd.filter = world::get_filter_for_layer(z, 15);
                fd.filter.groupIndex = -1;
                fd.shape = &e;
                /* TODO: set material properties */
                fd.restitution = .7f;
                fd.friction = .7f;
                fd.density = 1.f;
                b2Fixture *f = this->body->CreateFixture(&fd);
                f->SetUserData(this);
                f->SetUserData2(&this->merged[z][px]);
            }
        }
    }

    /* destroy connections */
    if (!initial) {
        connection *cc = this->conn_ll;
        if (cc) {
            do {
                tms_debugf("checking conn %p", cc);
                connection *next = cc->next[1];//cc->e == this ? 0 : 1];

                if (!this->get_pixel((cc->o_data & 15u), (cc->o_data >> 4u) & 15u, ((cc->o_data >> 8u) & 3u))) {
                    tms_debugf("destroyed conn %p", cc);
                    cc->e->destroy_connection(cc);
                }
                cc = next;
            } while (cc);
        }
    }
}

uint32_t
level_chunk::get_fixture_connection_data(b2Fixture *f)
{
    tpixel_desc *info = static_cast<tpixel_desc*>(f->GetUserData2());

    if (info) {
        return (uint32_t)info->pos | (world::fixture_get_layer(f) << 8u);
    }

    return 0;
}

void
level_chunk::remove_from_world()
{
    if (this->body) {
        //tms_debugf("chunk at [%d,%d] REMOVED", this->pos_x, this->pos_y);
        W->b2->DestroyBody(this->body);
        this->body = 0;
    }
}

void
level_chunk::on_paused_touch(b2Fixture *my, b2Fixture *other)
{
    this->on_touch(my,other);
}

void
level_chunk::on_touch(b2Fixture *my, b2Fixture *other)
{
    entity *o = (entity*)other->GetUserData();

    if (o) {
        G->lock();
        for (int x=0; x<o->num_chunk_intersections+1; x++) {
            if (x == o->num_chunk_intersections) {
                if (x == ENTITY_MAX_CHUNK_INTERSECTIONS) {
                    tms_warnf("entity gid %u id %u (%s) reached max chunk intersections", o->g_id, o->id, o->get_name());

                    o->num_chunk_intersections = 0;
                    other->Refilter();
                } else {
                    o->chunk_intersections[x].x = this->pos_x;
                    o->chunk_intersections[x].y = this->pos_y;
                    o->chunk_intersections[x].num_fixtures = 1;
                    o->num_chunk_intersections ++;
                }

                break;
            }
            if (o->chunk_intersections[x].x == this->pos_x && o->chunk_intersections[x].y == this->pos_y) {
                o->chunk_intersections[x].num_fixtures ++;
                break;
            }
        }

        /* awake/sleep also increments/decrements the num_fixtures status, so
         * we don't touch it if the body is sleeping */
        if (other->GetBody()->IsAwake()) {
            this->add_fixture(other, o);

            if (o->flag_active(ENTITY_DISABLE_UNLOADING)) {
                this->num_non_unloadable ++;
            }
        }

        G->unlock();
    }
}

void
level_chunk::on_paused_untouch(b2Fixture *my, b2Fixture *other)
{
    this->on_untouch(my,other);
}

void
level_chunk::on_untouch(b2Fixture *my, b2Fixture *other)
{
    entity *o = (entity*)other->GetUserData();

    if (o) {
        G->lock();
        for (int x=0; x<o->num_chunk_intersections; x++) {
            if (o->chunk_intersections[x].x == this->pos_x && o->chunk_intersections[x].y == this->pos_y) {
                o->chunk_intersections[x].num_fixtures --;

#ifdef DEBUG
                tms_assertf(o->chunk_intersections[x].num_fixtures >= 0, "num fixtures in chunk intersection was < 0");
#endif

                if (o->chunk_intersections[x].num_fixtures <= 0) {
                    o->num_chunk_intersections --;
                    if (o->num_chunk_intersections && x != o->num_chunk_intersections) {
                        o->chunk_intersections[x] = o->chunk_intersections[o->num_chunk_intersections];
                    }
                }
                break;
            }
        }

        if (other->GetBody()->IsAwake()) {
            this->remove_fixture(other, o);

            if (o->flag_active(ENTITY_DISABLE_UNLOADING)) {
                this->num_non_unloadable --;
            }
        }
        G->unlock();
    }
}
