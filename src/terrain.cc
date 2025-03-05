#include "world.hh"
#include "terrain.hh"
#include "chunk.hh"
#include "object_factory.hh"
#include "game.hh"
#include "noise.h"
#include "plant.hh"
#include "gentype.hh"
#include "decorations.hh"

bool operator <(const terrain_coord& lhs, const terrain_coord &rhs)
{
    if (lhs.chunk_x != rhs.chunk_x) {
        return lhs.chunk_x < rhs.chunk_x;
    } else {
        return lhs.chunk_y < rhs.chunk_y;
    }
}

/**
 * Terrain generation overview
 *
 * Phase 1:
 * heightmap
 *
 * Phase 2:
 * set material based on height
 *
 * Phase 3:
 * Dig out caves
 *
 * Phase 4:
 * Merge pixels
 **/

static double
t_height(double x)
{
    x -= ((W->level.seed & 0xff0) >> 4)*13.33f;
    double height = 0;

    static const float max_height = 500.f;

    for (int n=0; n<12; n++) {
        float amp = powf(2.f, n);
        float freq = amp;
        //height += 100. * _noise1(x*.0001*pow(2., n)) / (pow(2.,n)*.1);
        //height += 300. * _noise1(x*.0005*pow(2., n)) / (n+1);
        height += _noise1(x*.0004*freq) / amp;

        if (n == 5) {n = 10;}
    }

    float hh = fabsf(_noise1(x*.001f)) + .05f;

    return std::abs(height) * 500.f*hh;
}

/**
 * Occupy all chunk slots
 **/
void
terrain_transaction::occupy(gentype *gt)
{
    typedef std::multimap<terrain_coord, terrain_edit>::iterator iterator;
    iterator i, ii;

    this->state = TERRAIN_TRANSACTION_OCCUPYING;

    for (i = this->modifications.begin(); i != this->modifications.end(); ) {
        terrain_coord c = (*i).first;
        std::pair<iterator, iterator> range = this->modifications.equal_range(c);

        level_chunk *chunk = W->cwindow->get_chunk(c.chunk_x, c.chunk_y);

        tms_debugf("occupy gt %p %d %d", gt, chunk->pos_x, chunk->pos_y);

        if (this->state != TERRAIN_TRANSACTION_OCCUPYING) {
            /* another transaction invalidate this one */
            tms_debugf("INVALIDATED!!!");
            return;
        }

        for (ii = range.first; ii != range.second; ++ii) {
            terrain_coord cc = (*ii).first;

            if (!chunk->occupy_pixel(cc.get_local_x(), cc.get_local_y(), gt)) {
                tms_debugf("terrain transaction failed");
                this->state = TERRAIN_TRANSACTION_INVALIDATED;
                return;
            }
        }

        /* make sure the chunk is generated to phase 3 */
        chunk->generate(W->cwindow, 3);

        if (this->state != TERRAIN_TRANSACTION_OCCUPYING) {
            return;
        }

        i = range.second;
    }

    tms_assertf(this->state == TERRAIN_TRANSACTION_OCCUPYING, "state was not occupying");
    this->state = TERRAIN_TRANSACTION_OCCUPIED;
}

void
terrain_transaction::apply()
{
#ifdef DEBUG
    tms_assertf(this->state != TERRAIN_TRANSACTION_APPLIED, "wtf?");
#endif

    tms_debugf("applying terrain transaction");

    typedef std::multimap<terrain_coord, terrain_edit>::iterator iterator;
    iterator i, ii;

    /* make sure nearby chunks are generated to phase 3 */
    for (i = this->modifications.begin(); i != this->modifications.end(); ) {
        terrain_coord c = (*i).first;
        std::pair<iterator, iterator> range = this->modifications.equal_range(c);

        level_chunk *chunk = W->cwindow->get_chunk(c.chunk_x, c.chunk_y);

        for (int y=GENTYPE_MAX_REACH_Y; y>=-GENTYPE_MAX_REACH_Y; y--) {
            for (int x=-GENTYPE_MAX_REACH_X; x<=GENTYPE_MAX_REACH_X; x++) {
                //tms_debugf("make sure %d %d is gen 3", chunk->pos_x+x, chunk->pos_y+y);
                level_chunk *c = W->cwindow->get_chunk(chunk->pos_x+x, chunk->pos_y+y, false);
                c->generate(W->cwindow, 3);
            }
        }

        i = range.second;
    }

    if (this->state != TERRAIN_TRANSACTION_OCCUPIED) {
        return;
    }

    tms_debugf("transaction apply, state is %d", this->state );
    this->state = TERRAIN_TRANSACTION_APPLIED;

    for (i = this->modifications.begin(); i != this->modifications.end(); ) {
        terrain_coord c = (*i).first;
        std::pair<iterator, iterator> range = this->modifications.equal_range(c);

        level_chunk *chunk = W->cwindow->get_chunk(c.chunk_x, c.chunk_y);

#ifdef DEBUG_SPECIFIC_CHUNK
        if (chunk->pos_x == DEBUG_CHUNK_X && chunk->pos_y == DEBUG_CHUNK_Y) {
            tms_debugf("applying gentype in chunk %d %d", DEBUG_CHUNK_X, DEBUG_CHUNK_Y);
        }
#endif

        for (ii = range.first; ii != range.second; ++ii) {
            terrain_coord cc = (*ii).first;
            terrain_edit e = (*ii).second;

            if (e.flags & TERRAIN_EDIT_TOUCH) {

            } else {
                int local_x = cc.get_local_x();
                int local_y = cc.get_local_y();
                int new_mat = e.data;

                for (int z=0; z<3; z++) {
                    if (!(e.flags & (TERRAIN_EDIT_LAYER0 << z))) {
                        continue;
                    }

                    int prev = chunk->get_pixel(local_x, local_y, z);
                    bool apply = true;

                    if (e.flags & TERRAIN_EDIT_INC) {
                        apply = (prev < new_mat);
                    } else if (e.flags & TERRAIN_EDIT_DEC) {
                        apply = (prev > new_mat);
                    }

                    if (e.flags & TERRAIN_EDIT_SOFTEN) {
                        new_mat = prev - 1;
                    } else if (e.flags & TERRAIN_EDIT_HARDEN) {
                        new_mat = prev + 1;
                    }

                    if (e.flags & TERRAIN_EDIT_SOFT) {
                        apply = (apply && !prev);
                    } else if (e.flags & TERRAIN_EDIT_NONEMPTY) {
                        apply = (apply && prev);
                    }

                    if (apply) {
                        chunk->set_pixel(local_x, local_y, z, new_mat);
                    }
                }
            }
        }

        if (chunk->generate_phase >= 5) {
            /* XXX XXX XXX */
            tms_fatalf("This should never happen, chunk xy: %d %d, dist: %d %d", chunk->pos_x, chunk->pos_y, this->start_x-chunk->pos_x, this->start_y-chunk->pos_y);
#if 1
            chunk->remerge();

            int s = chunk->slot;
            if (s != -1) {
                W->cwindow->unload_slot(s);
                W->cwindow->load_slot(s, chunk);
            }

            if (chunk->body) {
                chunk->recreate_fixtures(true);
            }
        }
#endif

        i = range.second;
    }
}

/**
 * 3 = very stony
 * 0 = very grassy
 **/
double t_tundra(double x)
{
    double r = 0.;

    x -= 453.346f;

    for (int n=0; n<3; n++) {
        double freq = pow(2, (double)n);
        r += _noise1(x*.005*freq) / freq;
    }

    //r *= (/*_n5ise1(x*.3)*.05 + */_noise1(x*.0042)*.2 + _noise1(x*.0042*.25)*.1);

    r = 3.f - tclampf(std::abs(r)*6.f, 0.f, 3.f);

    return r;
}

double roughness(double x, double y)
{
    double r = 0.;

    float f = (/*_n5ise1(x*.3)*.05 + */_noise1(x*.002)*.2 + _noise1(x*.002*.25)*.1);

    if (f > .001) {
        for (int n=0; n<3; n++) {
            double freq = pow(2, (double)n);
            double v[2] = {x*.04*freq, y*.03*freq};
            r += 80. * _noise2(v[0], v[1]) / freq;
        }
    }

    r *= f;
    return r;
}

double cave(double x, double y, double depth)
{
    double r = 0;

    for (int n=0; n<3; n++) {
        float freq = powf(2, n);
        float amp = powf(.7f, n);
        float nn = powf(n, 2);
        double v[2] = {x*.035*freq, y*.035*freq};
        r += _noise2(v[0], v[1])*amp;
    }

    r *= _noise2(x*.003f, y*.003f)*.5f+.5f;

    //r = fabs(r);
    //r = pow(r, .125/2.);

    /*
    {
    double v[2] = {x*.01, y*.01};
    r -= tclampf(_noise2(v[0], v[1]), 0.f, 1.f)*.25;
    }
    */

    return r;
}

unsigned flp2(unsigned x)
{
   x = x | (x >> 1);
   x = x | (x >> 2);
   x = x | (x >> 4);
   x = x | (x >> 8);
   x = x | (x >>16);
   return x - (x >> 1);
}

void
chunk_window::set_seed(uint64_t seed)
{
    this->seed = seed;
    tms_infof("chunk window set seed to %" PRIu64, seed);
    _noise_init_perm(seed & 0xffffffff);
}

float *
chunk_window::generate_heightmap(int chunk_x, bool search)
{
    if (search) {
        level_chunk *c = this->get_chunk(chunk_x, 0);
        c->generate(this, 2);

        return this->get_heights(chunk_x);
    } else {
        float *heights = (float*)malloc(16*sizeof(float));

#if 0
        tms_debugf("generate heightmap for chunk x %d", chunk_x);
#endif

        for (int x=0; x<16; x++) {
            heights[x] = -100000.f;
        }
        this->heightmap.insert(std::make_pair(chunk_x, heights));
        return heights;
    }
}

static inline bool is_surface_level(int pos_y, float *heights)
{
    return ((pos_y+1)*8.f > heights[7] && (pos_y)*8.f < heights[7]);
}

void
level_chunk::generate_phase6(chunk_window *win)
{
    this->generate_phase = 6;
    float *heights = win->get_heights(this->pos_x);

    if (is_surface_level(this->pos_y, heights)) {
        this->generate_vegetation(win->get_heights(this->pos_x));
    } else {
    }
}

void
level_chunk::generate(chunk_window *win, int up_to_phase/*=5*/)
{
    if (win->seed == 0) {
        return;
    }

#ifdef DEBUG_SPECIFIC_CHUNK
    if (up_to_phase > this->generate_phase && this->pos_x == DEBUG_CHUNK_X && this->pos_y == DEBUG_CHUNK_Y) {
        tms_debugf("(chunk %d,%d) generate phase %d->%d %p", DEBUG_CHUNK_X, DEBUG_CHUNK_Y,
                this->generate_phase, up_to_phase, this);
    }
#endif

    switch (this->generate_phase) {
        /* always passthrough */
        case 0:
            if (up_to_phase < 1) return;
            this->garbage = 0;
            generate_phase1(win);
        case 1:
            if (up_to_phase < 2) return;
            this->garbage = 0;
            generate_phase2(win);
        case 2:
            if (up_to_phase < 3) return;
            this->garbage = 0;
            generate_phase3(win);
        case 3:
            if (up_to_phase < 4) return;
            this->garbage = 0;
            generate_phase4(win);
        case 4:
            if (up_to_phase < 5) return;
            this->garbage = 0;
            generate_phase5(win);

            /* phase 6 is generated just after the chunk was added to the world for the first time */
        case 5:
            if (up_to_phase < 6) return;
            this->garbage = 0;
            generate_phase6(win);
    }
}

void
level_chunk::make_smooth(chunk_window *win)
{
    /* XXX */
    double h[16];
    double t[16];
    double mat_depths[16][3];
    for (int x=0; x<16; x++) {
        float wx = ((double)x)*.5+(double)this->pos_x*8.;
        h[x] = t_height(wx);
        t[x] = t_tundra(wx);
        mat_depths[x][0] = -1.f * t[x];
        mat_depths[x][1] = -2.f * t[x];
        mat_depths[x][2] = -50.f * (t[x]+.5f);
    }

    float *heights = win->get_heights(this->pos_x);

    float depth_tbl[16][16];

    for (int z=0; z<3; z++) {
        for (int y=0; y<16; y++) {
            double wy = (y*.5 + this->pos_y*8.);
            for (int x=0; x<16; x++) {
                if (this->pixels[z][y][x]) {
                    double wx = (x*.5 + this->pos_x*8.);
                    int mat = 1;

                    float depth = wy - heights[x];
                    float depth2 = wy - h[x];

                    if (z == 0) {
                        //depth_tbl[y][x] = _noise2(wx*.03, wy*.03)*4.;
                        //depth_tbl[y][x] += _noise2(wx*.1, wy*.1)*2.;
                        //depth_tbl[y][x] += _noise2(wx*.2, wy*.2);
                        depth_tbl[y][x] = _noise1(wx*.03);
                        depth_tbl[y][x] += _noise1(wx*.1);
                        depth_tbl[y][x] += _noise1(wx*.2);
                    }

                    depth2 -= depth_tbl[y][x];

                    if (depth < mat_depths[x][0]) {
                        mat = 2;
                    }
                    if (depth2 < mat_depths[x][1]) {
                        mat ++;
                    }
                    if (depth2 < mat_depths[x][2]) {
                        mat = 4;
                    }

                    if (mat < 1) mat = 1;
                    if (mat > 4) mat = 4;

                    this->pixels[z][y][x] = mat;
                }
            }
        }
    }
}

/**
 * Phase 1: base outline of the terrain, heights
 **/
void
level_chunk::generate_phase1(chunk_window *win)
{
    if (this->generate_phase == 0) {
        this->generate_phase = 1;
    }

    double h[16];
    double rough[16][16];

    float *heights = win->get_heights(this->pos_x);

    for (int z=0; z<3; z++) {
        for (int y=0; y<16; y++) {

            double wy = (y*.5 + this->pos_y*8.);

            for (int x=0; x<16; x++) {
                double wx = (x*.5 + this->pos_x*8.);

                /* setup heights */
                if (z == 0 && y == 0) {
                    h[x] = t_height(((double)x)*.5+(double)this->pos_x*8.);
                }

                /* setup roughness */
                if (z == 0) {
                    rough[y][x] = roughness(wx, wy);
                }

                double r = 0.f;

                r = rough[y][x];

                float height = (h[x]+r);

                double depth = wy - height + z*.5;

                if (z > 0) {
                    depth += z*.5* _noise1(wx*.2)*1.25;
                }

                if (depth < 0.) {
                    if (z == 0 && heights[x] < wy) {
                        heights[x] = wy;
                    }
                    this->pixels[z][y][x] = 1;
                } else {
                    this->pixels[z][y][x] = 0;
                }
            }
        }
    }
}

/**
 * Phase 2: set materials
 **/
void
level_chunk::generate_phase2(chunk_window *win)
{
    if (this->generate_phase == 1) this->generate_phase = 2;

    /* make sure some chunks up are generated to phase 1
     * so the height is properly initialized */

    float height_bkp[16];

    int curr_y = -1;
    float *heights = win->get_heights(this->pos_x);

    terrain_coord htest(this->pos_x*8.f, heights[0]-17.f);

    curr_y = std::max(curr_y, htest.chunk_y);

    //tms_debugf("starting at %d for %d, heights[0]=%f", curr_y, this->pos_x, heights[0]);

    do {
        memcpy(height_bkp, heights, sizeof(float)*16);

        level_chunk *c = win->get_chunk(this->pos_x, curr_y);
        c->generate(win, 1);

        /* fetch the heights again and see if they changed */
        float *new_heights = win->get_heights(this->pos_x);

#ifdef DEBUG
        tms_assertf(new_heights == heights, "that pointer should NOT change!");
#endif

        if (memcmp(height_bkp, heights, sizeof(float)*16) == 0) {
            /* heights didn't change, the chunk we checked
             * is above ground or already generated */

            if ((curr_y)*8.f > heights[0]) {
                break;
            }
        }

        if (curr_y > 500) {
            tms_fatalf("This shouldn't happen! (terrain generation) %f %f", (curr_y)*8.f, heights[0]);
            break;
        }

        curr_y ++;
    } while (true);

    this->make_smooth(win);
}

void
level_chunk::generate_caves(float *heights)
{
    for (int y=0; y<16; y++) {
        double wy = (y*.5 + this->pos_y*8.);
        for (int x=0; x<16; x++) {
            double wx = (x*.5 + this->pos_x*8.);
            float depth = wy - heights[x];
            double c = cave(wx, wy, depth);

            if (c > .75f && depth < -1.f) {
                this->pixels[1][y][x] = 0;
            }
        }
    }
}

bool
level_chunk::find_ground(terrain_coord *in, int layer, terrain_coord *out, float *heights, int tolerance)
{
    if (!heights) {
        heights = W->cwindow->get_heights(this->pos_x, true);
    }

#ifdef DEBUG_PRELOADER_SANITY
    tms_assertf(heights[0] > -1000.f, "suspicious height %f as base for find_ground", heights[0]);
#endif

    bool found = false;

    terrain_coord c = *in;

    for (int s=0; s<8; s++, c.step(0, -1)) {
        if (c.chunk_y < this->pos_y) {
            break;
        }
        if (c.chunk_y > this->pos_y || c.get_local_y() == 15) {
            continue;
        }

        if (this->pixels[layer][c.get_local_y()][c.get_local_x()]) {
            c.step(0, 1);
            /* XXX use tolerance here */
            if (!this->pixels[layer][c.get_local_y()][c.get_local_x()]) {
                c.step(0, -1);
                found = true;
            }
            break;
        }
    }

    if (found) {
        *out = c;
        return true;
    }

    return false;
}

void
level_chunk::generate_vegetation(float *heights)
{
    float last_noise;
    for (int x=0; x<16; x+=2) {
        double wx = (x*.5 + this->pos_x*8.);
        double wy = heights[x];

        float tundra = t_tundra(wx);

        float noise = _noise1(wx*0.171522f);// * ((3.f-t_tundra(wx))*1.f);

        terrain_coord c(wx,wy);
        terrain_coord ground;
        int layer = rand()%2 * 2;

        if (layer == 0 && (this->pos_x == 0 || this->pos_x == -1 || (this->pos_x == -1 && x > 8))) {
            last_noise = noise;
            /* do not generate vegetation where the repair station and factory is placed in start new adventure */
            continue;
        }

        bool found = this->find_ground(&c, layer, &ground, heights, 1);

        if (!found) {
            last_noise = noise;
            continue;
        }

        if (x > 0) {
            plant *r=0;

            if (true || !W->is_paused()) {
                if (noise < .15f && last_noise >= .15f) {
                    r = static_cast<plant*>(of::create(O_PLANT));
                }

                if (r) {
                    int rnd = rand()%200;
                    if (tundra < .5f) {
                        if (rnd < 30) {
                            r->set_from_predef(PLANT_COLORFUL_BUSH);
                        } else if (rnd < 100) {
                            r->set_from_predef(PLANT_SAND_TREE);
                        } else {
                            r->set_from_predef(PLANT_BUSH);
                        }
                    } else {
                        if (rnd < 2) {
                            r->set_from_predef(PLANT_COLORFUL_BUSH);
                        } else if (rnd < 5) {
                            r->set_from_predef(PLANT_BIG_TREE);
                        } else if (rnd < 100) {
                            r->set_from_predef(PLANT_TREE);
                        } else if (rnd < 120) {
                            r->set_from_predef(PLANT_ROUGH_TREE);
                        } else {
                            r->set_from_predef(PLANT_BUSH);
                        }
                    }
                    r->set_layer(layer);

                    if (rand()%5==0) {
                        decoration *i = static_cast<decoration*>(of::create(O_DECORATION));
                        i->set_decoration_type(DECORATION_STONE1+rand()%3);
                        i->set_layer(layer);
                        i->_pos = b2Vec2(ground.get_world_x()+1*(rand()%2?-1:1), ground.get_world_y());
                        i->_angle = rand()%100/100*M_PI*2.f;
                        i->on_load(true, false);
                        W->add(i);
                        G->add_entity(i);
                    }

                    r->_pos = b2Vec2(ground.get_world_x(), ground.get_world_y()+.25f);
                    r->_angle = M_PI/2.f;

                    r->on_load(true, false);

                    W->add(r);
                    G->add_entity(r);

                    r->init();

                    tms_assertf(this->body != 0, "damn that body!!!!!!!");
                    r->c.p = r->get_position();
                    r->c.o = this;
                    r->c.o_data = (uint32_t)(ground._xy | (layer << 8u));
                    G->apply_connection(&r->c, -1);
                }
            }
        }

        last_noise = noise;
    }
}

/**
 * Phase 3: caves and merging into bigger pixels
 *
 * Occupy gentypes
 **/
void
level_chunk::generate_phase3(chunk_window *win)
{
    if (this->generate_phase == 2) this->generate_phase = 3;

    float *heights = win->get_heights(this->pos_x, true);

    this->generate_caves(heights);

    gentype::generate(this);
}

/**
 * Phase 4: Apply any gentypes allocated in this chunk
 **/
void
level_chunk::generate_phase4(chunk_window *win)
{
    if (this->generate_phase == 3) {
        this->generate_phase = 4;
    }

    /* before we can generate phase 4, we must make sure that a certain amount of
     * chunks nearby is generated to phase 3, incase they have gentypes of higher priority */
    for (int y=GENTYPE_MAX_REACH_Y; y>=-GENTYPE_MAX_REACH_Y; y--) {
        for (int x=-GENTYPE_MAX_REACH_X; x<=GENTYPE_MAX_REACH_X; x++) {
            if (y != 0 || x != 0) {
                level_chunk *c = win->get_chunk(this->pos_x+x, this->pos_y+y, false);
                c->generate(win, 3);
            }
        }
    }

#ifdef DEBUG_SPECIFIC_CHUNK
    if (this->pos_x == DEBUG_CHUNK_X && this->pos_y == DEBUG_CHUNK_Y) {
        tms_debugf("(chunk %d,%d) apply gentypes", this->pos_x, this->pos_y);
    }
#endif

    this->apply_gentypes(0);
}

/**
 * Phase 5: Apply gentypes that modify other gentypes (minerals etc)
 **/
void
level_chunk::generate_phase5(chunk_window *win)
{
#if 0
    if (this->generate_phase < 5) {
        tms_debugf("GENERATE PHASE 5 for %d %d", this->pos_x, this->pos_y);
    }
#endif

    if (this->generate_phase == 4) {
        this->generate_phase = 5;
    }

    this->apply_gentypes(1);

    this->remerge();
}

void
level_chunk::apply_gentypes(int sorting)
{
    /* TODO: to handle delete of gentypes, we first must make sure that
     * all relevant chunks are done generating, we can use reference counting in the
     * gentypes, and when a chunk generates it decrements the gentypes reference count,
     * and deletes the gentype when the reference count is 0 */

    std::set<gentype*> fixed;

    tms_debugf("chunk %p applying gentypes with sorting %d", this, sorting);

    for (int x=0; x<GENSLOT_SIZE_X; ++x) {
        for (int y=0; y<GENSLOT_SIZE_Y; ++y) {
            gentype *gt = this->genslots[x][y][sorting];

            if (!gt) {
                continue;
            }


            tms_debugf("applying %p %u", gt, gt->id);

            //tms_debugf("found %p in a genslot", gt);

            /*
            std::set<gentype*>::iterator it = fixed.find(gt);
            if (it != fixed.end()) {
                continue;
            }
            */

            gt->apply();

            /* make sure the gentype isnt already deleted after apply */
            if (this->genslots[x][y][sorting] == gt) {
                tms_debugf("deleting gentype %p %u", gt, gt->id);
                delete gt;
            }

            //fixed.insert(gt);
            //delete gt;
        }
    }

    /*for (std::set<gentype*>::iterator it = fixed.begin();
            it != fixed.end(); ++it) {
        //delete *it;
    }*/
}

void
level_chunk::remerge()
{
    for (int z=0; z<3; ++z) {
        this->num_merged[z] = 0;
        this->min_merged[z] = 0;
    }

    this->merge(0, 0, 0, 16, 16, 3);

#ifdef DEBUG_SPECIFIC_CHUNK
    if (this->pos_x == DEBUG_CHUNK_X && this->pos_y == DEBUG_CHUNK_Y) {
        tms_debugf("(chunk %d,%d) REMERGE created [%p] %d %d %d",
                this->pos_x, this->pos_y,
                this, this->num_merged[0], this->num_merged[1],this->num_merged[2]);
    }
#endif
}

void
level_chunk::merge(int _x, int _y, int _z, int _w, int _h, int _d)
{
    uint8_t pixels[16][16];

    for (int z=_z; z<_d; z++) {
        memcpy(pixels, this->pixels[z], sizeof(pixels));
        for (int y=_y; y<_h; y++) {
            for (int x=_x; x<_w; x++) {
                uint8_t px = pixels[y][x];
                if (px) {
                    int sy=0,sx=0,min_sx = 8;
                    for (sy=0; ; sy++) {
                        if (y+sy >= _h) {break;};
                        for (sx=0; ; sx++) {
                            if (sx>=min_sx) break;
                            if (x+sx >= _w) {break;};
                            uint8_t ppx = pixels[y+sy][x+sx];
                            if (!(ppx)) {break;};
                            if (ppx != px) {break;};

                        }
                        min_sx = sx;
                        if (sy>=min_sx) break;
                    }

                    if (sy < 1) sy = 1;
                    if (min_sx < 1) min_sx = 1;

                    int ssy = flp2(sy);
                    int ssx = flp2(min_sx);

                    int ss = ssy < ssx ? ssy : ssx;
                    int size = (int)floorf(log2(ss));

                    /* mark pixels as taken */
                    for (sy=0; sy<ss; sy++) {
                        for (sx=0; sx<ss; sx++) {
                            pixels[y+sy][x+sx] = 0;
                        }
                    }

                    struct tpixel_desc desc;
                    desc.size = size;
                    desc.pos = (y << 4) | (x & 15);
                    desc.material = chunk_mat_to_tpixel_mat(px);
                    desc.r =
                        (((x + this->pos_x*16) ^ 0xfa367aef)
                        * (((y + this->pos_y*16)) ^ 0x73f976ab))
                        & 0xff;
                    desc.reset();
                    if (size == 0) {
                        if (x > 0 && y > 0 && x < 15 && y < 15) {
                            int bits = (this->pixels[z][y+1][x] ? 1 : 0)
                                 | (this->pixels[z][y][x-1] ? 2 : 0)
                                 | (this->pixels[z][y-1][x] ? 4 : 0)
                                 | (this->pixels[z][y][x+1] ? 8 : 0)
                                 ;

                            switch (bits) {
                                case 1+2: desc.desc_1_5_1.half = 1; break;
                                case 2+4: desc.desc_1_5_1.half = 2; break;
                                case 4+8: desc.desc_1_5_1.half = 3; break;
                                case 8+1: desc.desc_1_5_1.half = 4; break;
                            }
                        }
                    }
                    if (_w != 16 || _h != 16) {
                        /* force oil to be 0 if we're recreating a part of this chunk,
                         * damaging pixels will effectively remove all oil from that place */
                        desc.oil = 0.f;
                    }
                    if (this->generate_phase < 6) {
                        if (y < 15 && px == 1 && size == 0 && !(this->pixels[z][y+1][x])) {
                            desc.grass = (uint8_t)(_noise1((x+this->pos_x)*.1)*128 + 127);
                        }
                    }
                    /* TODO: check out of bands and loop through and look for destroyed pixels to take their place */

                    /* find a new slot in the merged buffer */
                    int m;
                    int limit = std::min(16*16, (int)this->num_merged[z]);
                    for (m=this->min_merged[z]; m<limit; m++) {
                        if (this->merged[z][m].hp <= 0.f) {
                            this->merged[z][m] = desc;
                            this->min_merged[z] = m+1;
                            break;
                        }
                    }

                    if (m == this->num_merged[z]) {
                        this->merged[z][m] = desc;
                        this->num_merged[z] ++;
                        this->min_merged[z] = this->num_merged[z];
                    }

                    x += ss-1;
                }
            }
        }
    }
}
