#include "chunk.hh"
#include "world.hh"
#include "model.hh"
#include "object_factory.hh"
#include "gentype.hh"
#include "terrain.hh"
#include "settings.hh"
#include "noise.h"
#include "misc.hh"

static struct tms_mesh    *mesh_pool[MAX_CHUNKS];
static tms::gbuffer *vbuf[MAX_CHUNKS];
static struct tms_varray  *va[MAX_CHUNKS];
static struct tms_gbuffer *ibuf;
static bool   initialized;

static struct tms_mesh    *grass_pool[MAX_CHUNKS];
static struct tms_gbuffer *grass_vbuf[MAX_CHUNKS];
static struct tms_varray  *grass_va[MAX_CHUNKS];
static struct tms_gbuffer *grass_ibuf;

struct vertex {
    tvec3 pos;
    tvec3 nor;
    tvec2 uv;
} __attribute__ ((packed));

struct grass_vertex {
    tvec4 pos;
    tvec2 uv;
}  __attribute__ ((packed));

struct cvert {
    tvec3 p;
    tvec3 n;
    tvec2 u;
} __attribute__((packed));

static int vertices_per_tpixel;
static int indices_per_tpixel;

static void
_init()
{
    initialized = true;

    struct tms_mesh *mm = mesh_factory::get_mesh(MODEL_BOX_TEX);
    vertices_per_tpixel = mm->v_count;
    indices_per_tpixel = mm->i_count;

    {
        ibuf = new tms::gbuffer((3 * 16 * 16 * indices_per_tpixel * sizeof(uint16_t))/* / 4 * 3*/);
        ibuf->usage = GL_STATIC_DRAW;

        uint16_t *i = (uint16_t*)tms_gbuffer_get_buffer(ibuf);
        uint16_t *ri = (uint16_t*)((char*)tms_gbuffer_get_buffer(mm->indices)+mm->i_start*2);
        uint16_t ibase = mm->v_start / sizeof(struct cvert);

        for (int x=0; x<(16*16*3)/*/4*3*/; x++) {
            for (int y=0; y<indices_per_tpixel; y++) {
                i[x*indices_per_tpixel + y] = ri[y] - ibase + x*vertices_per_tpixel;
            }
        }

        tms_gbuffer_upload(ibuf);
    }

    {
        grass_ibuf = new tms::gbuffer(MAX_GRASS_PER_CHUNK*6*sizeof(short));
        uint16_t *i = (uint16_t*)tms_gbuffer_get_buffer(grass_ibuf);
        for (int x=0; x<MAX_GRASS_PER_CHUNK; x++) {
            i[x*6+0] = x*4;
            i[x*6+1] = x*4+1;
            i[x*6+2] = x*4+2;
            i[x*6+3] = x*4;
            i[x*6+4] = x*4+2;
            i[x*6+5] = x*4+3;
        }
        tms_gbuffer_upload(grass_ibuf);
    }

    for (int x=0; x<MAX_CHUNKS; x++) {
        vbuf[x] = new tms::gbuffer((3 * 16 * 16 * vertices_per_tpixel * sizeof(struct vertex))/* / 4 * 3*/);
        vbuf[x]->usage = GL_STREAM_DRAW;

        va[x] = tms_varray_alloc(3);
        tms_varray_map_attribute(va[x], "position", 3, GL_FLOAT, vbuf[x]);
        tms_varray_map_attribute(va[x], "normal", 3, GL_FLOAT, vbuf[x]);
        tms_varray_map_attribute(va[x], "texcoord", 2, GL_FLOAT, vbuf[x]);
        mesh_pool[x] = tms_mesh_alloc(va[x], ibuf);

        grass_vbuf[x] = new tms::gbuffer(MAX_GRASS_VERTS_PER_CHUNK*sizeof(struct grass_vertex));
        grass_vbuf[x]->usage = GL_STREAM_DRAW;

        grass_va[x] = tms_varray_alloc(2);
        tms_varray_map_attribute(grass_va[x], "position", 4, GL_FLOAT, grass_vbuf[x]);
        tms_varray_map_attribute(grass_va[x], "texcoord", 2, GL_FLOAT, grass_vbuf[x]);
        grass_pool[x] = tms_mesh_alloc(grass_va[x], grass_ibuf);
    }

    initialized = true;
}

chunk_window::chunk_window()
{
    this->set_mesh((struct tms_mesh*)0);
    this->set_material(&m_tpixel);
    this->reset();

    this->caveview = tms_texture_alloc();
    tms_texture_set_filtering(this->caveview, GL_LINEAR);
    this->caveview->wrap = GL_CLAMP_TO_EDGE;
    tms_texture_alloc_buffer(this->caveview, CAVEVIEW_SIZE, CAVEVIEW_SIZE, 1);
    tms_texture_clear_buffer(this->caveview, 255);
}

void
chunk_window::reset()
{
    if (!initialized) {
        _init();
    }

    memset(this->slots, 0, sizeof(this->slots));

    for (std::map<int, float*>::iterator i = this->heightmap.begin(); i != this->heightmap.end(); i++) {
        free(i->second);
    }
    this->heightmap.clear();

    if (this->children) {free(this->children);this->children=0;this->num_children=0;}

    this->x = 0;
    this->y = 0;
    this->w = 0;
    this->h = 0;
    this->isset = false;
}

/** 
 * Sets a pixel at global gx and gy
 * returns the affected chunk
 **/
level_chunk *
chunk_window::set_pixel(int gx, int gy, int z, int material)
{
    int cx = (int)floor(gx/16.f);
    int cy = (int)floor(gy/16.f);

    level_chunk *c = this->get_chunk(cx, cy);

    c->generate(this, 2);

    c->set_pixel(gx-cx*16, gy-cy*16, z, material);

    return c;
}

/** 
 * Gets the pixel at global gx and gy
 * returns the material of the given pixel
 **/
int
chunk_window::get_pixel(int gx, int gy, int z)
{
    int cx = (int)floor(gx/16.f);
    int cy = (int)floor(gy/16.f);

    level_chunk *c = this->get_chunk(cx, cy);

    return c->get_pixel(gx-cx*16, gy-cy*16, z);
}

float*
chunk_window::get_heights(int chunk_x, bool must_be_valid/*=false*/)
{
    std::map<int, float*>::iterator i = this->heightmap.find(chunk_x);

    float *heights;

    if (i == this->heightmap.end()) {
        heights = this->generate_heightmap(chunk_x, must_be_valid);
    } else {
        heights = i->second;
    }

    return heights;
}

float
chunk_window::get_height(float x)
{
    terrain_coord c(x, 0.f);
    int cx = c.chunk_x;
    int sx = c.get_local_x();

    std::map<int, float*>::iterator i = this->heightmap.find(cx);

    float *heights;

    if (i == this->heightmap.end()) {
        tms_infof("get height at %f, %d %d", x, cx, sx);
        heights = this->generate_heightmap(cx, true); /* see terrain.cc */
    } else
        heights = i->second;

    return heights[sx];
}

void
chunk_window::unload_slot(int s)
{
    if (this->slots[s]) {
        this->slots[s]->slot = -1;
        for (int x=0; x<3; x++) {
            tms_entity_remove_child(static_cast<struct tms_entity*>(this), &this->slots[s]->layer_entities[x]);
            if (this->scene) tms_scene_remove_entity(this->scene, &this->slots[s]->layer_entities[x]);
        }

        this->slots[s] = 0;
    }
}

void
chunk_window::load_slot(int s, level_chunk *c)
{
    this->preloader.load(c, 2);

#ifdef DEBUG_SPECIFIC_CHUNK
    if (c->pos_x == DEBUG_CHUNK_X && c->pos_y == DEBUG_CHUNK_Y) {
        tms_debugf("(chunk %d,%d) ADDING TO WORLD: %d %p", c->pos_x, c->pos_y, c->generate_phase, c);
    }
#endif

    this->slots[s] = c;
    c->slot = s;

    tmat4_load_identity(c->M);
    tmat3_load_identity(c->N);

    if (c->layer_entities[0].parent) {
        for (int x=0; x<3; x++) {
            tms_entity_remove_child(static_cast<struct tms_entity*>(this), &this->slots[s]->layer_entities[x]);
            if (this->scene) tms_scene_remove_entity(this->scene, &this->slots[s]->layer_entities[x]);
        }
    }

    *(c->grass_entity[0].mesh) = *(grass_pool[s]);
    *(c->grass_entity[1].mesh) = *(grass_pool[s]);
    tmat4_load_identity(this->slots[s]->grass_entity[0].M);
    tmat4_translate(this->slots[s]->grass_entity[0].M, c->pos_x * 8.f, c->pos_y*8.f, 0.f);
    tmat4_copy(this->slots[s]->grass_entity[1].M, this->slots[s]->grass_entity[0].M);

    for (int x=0; x<3; x++) {
        *(c->layer_entities[x].mesh) = *(mesh_pool[s]);
        tms_entity_set_material(&this->slots[s]->layer_entities[x], static_cast<struct tms_material*>(&m_tpixel));

        tmat4_load_identity(this->slots[s]->layer_entities[x].M);
        tmat4_translate(this->slots[s]->layer_entities[x].M, c->pos_x * 8.f, c->pos_y*8.f, x*LAYER_DEPTH);

        tms_entity_add_child(static_cast<struct tms_entity*>(this), &this->slots[s]->layer_entities[x]);
        if (this->scene) tms_scene_add_entity(this->scene, &this->slots[s]->layer_entities[x]);
    }

    this->slots[s]->add_to_world();

    int num = 0;

    struct tms_mesh *mm = mesh_factory::get_mesh(MODEL_BOX_TEX);
    struct tms_gbuffer *r_vbuf = mm->vertex_array->gbufs[0].gbuf;

    struct cvert *rv = (struct cvert*)((char*)tms_gbuffer_get_buffer(r_vbuf)+mm->v_start);
    int num_rv = mm->v_count;

    struct cvert *rv_tri[4];
    for (int x=0; x<4; x++) {
        struct tms_mesh *mm2 = mesh_factory::get_mesh(MODEL_TRIBOX_TEX0+x);
        struct tms_gbuffer *vbuf = mm2->vertex_array->gbufs[0].gbuf;
        rv_tri[x] = (struct cvert*)((char*)tms_gbuffer_get_buffer(vbuf)+mm2->v_start);
    }

    struct vertex *v_first = (struct vertex*)tms_gbuffer_get_buffer(vbuf[s]);

    struct grass_vertex *v_grass = (struct grass_vertex*)tms_gbuffer_get_buffer(grass_vbuf[s]);
    int num_grass = 0;
    int num_grass_01 = 0;

    float diffuse_1, diffuse_2, diffuse_lim;
    if (_tms.gamma_correct) {
        diffuse_1 = GAMMA_CORRECTF(1.f*.7f);
        diffuse_2 = GAMMA_CORRECTF(.5f*.7f);
        diffuse_lim = GAMMA_CORRECTF(.75f*.7f);
    } else {
        diffuse_1 = 1.f*.7f;
        diffuse_2 = .5f*.7f;
        diffuse_lim = .75f*.7f;
    }

    for (int z=0; z<3; z++) {
        if (z == 2) {
            num_grass_01 = num_grass;
        }
        for (int q=0; q<NUM_GRASS_PER_PIXEL; q++) {
            for (int px=0; px<c->num_merged[z]; px++) {
                struct tpixel_desc desc = c->merged[z][px];

                if (desc.hp <= 0.f) {
                    continue;
                }

                int x = (desc.pos & 15);
                int y = desc.pos >> 4;
                int size = desc.size;

                /* XXX */
                if (y < 15 && desc.desc_1_5_1.half == 0 && size == 0
                        && desc.material == TPIXEL_MATERIAL_GRASS
                        && c->pixels[z][y+1][x] == TERRAIN_EMPTY
                        && desc.grass > 0) {
                    int gx = q;
                    //for (int gx=0; gx<NUM_GRASS_PER_PIXEL; gx++) {
                        float height = .22f + ((float)desc.grass) / 256.f * .53f;
                        uint8_t rr = (uint8_t)((desc.r+z*53+q*43+px*33) & 0xff);
                        float u = (rr ^ 0xf1)/256.f * .75f;
                        float w1 = (rr ^ 0xb8)/256.f * .15f;
                        float w2 = (rr ^ 0x3c)/256.f * .15f;
                        float h1 = (rr ^ 0xa1)/256.f * .05f;
                        float h2 = (rr ^ 0x22)/256.f * .05f;
                        float diffuse = gx == NUM_GRASS_PER_PIXEL-1 ?  diffuse_1 : diffuse_2;

                        float b1 = (rr ^ 0x99)/256.f*.05f;
                        float b2 = (rr ^ 0xf5)/256.f*.05f;

                        if (x > 0 && x < 15) {
                            if (c->pixels[z][y+1][x-1]) {
                                h1*=2.f;
                            } else if (!((c->pixels[z][y][x-1]))) {
                                h1 = -h1*.5f;
                                w1 = 0.f;
                            }

                            if (c->pixels[z][y+1][x+1]) {
                                h2*=2.f;
                            } else if (!c->pixels[z][y][x+1]) {
                                h2 = -h2*.5f;
                                w2 = 0.f;
                            }
                        }

                        v_grass[num_grass++] = (struct grass_vertex){
                            (tvec4){x*.5f + .25f+w2, y*.5f + height+h2, gx*.25f+z*LAYER_DEPTH - .25f+.01f, 1.f},
                            (tvec2){.25f+u, .9f},
                        };
                        v_grass[num_grass++] = (struct grass_vertex){
                            (tvec4){x*.5f - .25f-w1, y*.5f + height+h1, gx*.25f+z*LAYER_DEPTH - .25f+.01f, 1.f},
                            (tvec2){u, 0.9f},
                        };
                        v_grass[num_grass++] = (struct grass_vertex){
                            (tvec4){x*.5f - .25f, y*.5f + .22f+b1, gx*.25f+z*LAYER_DEPTH - .25f+.01f, diffuse},
                            (tvec2){u, 0.f},
                        };
                        v_grass[num_grass++] = (struct grass_vertex){
                            (tvec4){x*.5f + .25f, y*.5f + .22f+b2, gx*.25f+z*LAYER_DEPTH - .25f+.01f, std::max(diffuse, diffuse_lim)},
                            (tvec2){.25f+u, 0.f},
                        };
                    //}
                }
            }
        }
    }


    for (int z=0; z<3; z++) {
        int start=num;

        for (int px=0; px<c->num_merged[z]; px++) {
            struct tpixel_desc desc = c->merged[z][px];

            if (desc.hp <= 0.f) continue;

            struct cvert *_rv = desc.desc_1_5_1.half > 0 && desc.desc_1_5_1.half < 4 ? rv_tri[desc.desc_1_5_1.half-1] : rv;

            int x = (desc.pos & 15);
            int y = desc.pos >> 4;
            int size = desc.size;

            /* this is render-code! */
            /* ok */
            int mat = desc.material;

            float offs = 0.f;
            if (size > 0) {
                offs = .25f*(1<<(size)) - .25f;
            }

            int base = (num*vertices_per_tpixel);
            struct vertex *v = v_first + base;

            //float as = .5f;//tp->get_size()*2.f;
            float rs = (3.f - size)*(.125f);
            float as = (float)(1 << size)*.25f*2.f;
            //float zs = 1.5f+.125f*(((float)size));
            float zs = 2.f;
            for (int n=0; n<num_rv; n++) {
                v[n].pos = tvec3f(
                    _rv[n].p.x * as + (x)*.5 + offs,
                    _rv[n].p.y * as + (y)*.5 + offs,
                    _rv[n].p.z * zs
                );

                v[n].nor = (tvec3){
                    _rv[n].n.x,
                    _rv[n].n.y,
                    _rv[n].n.z
                };

                tvec2 bv = (tvec2){_rv[n].u.x - .125f/2.f, _rv[n].u.y-.5f-.125f/2.f};

                if (_rv[n].p.z < 0.f) {
                    bv.x /= std::max(0.5f, (float)size*2);
                    bv.y /= std::max(0.5f, (float)size*2);
                    //bv.x /= .5f;
                    //bv.y /= .5f;
                }

                if (size == 0) {
                    bv.x += .125f;
                    bv.y += .125f;
                } else {
                    bv.x += .125f/2.f;
                    bv.y += .125f/2.f;
                }

                v[n].uv = (tvec2){
                    //(bv.x) * (.5+(float)this->properties[0].v.i8) + (float)(this->properties[1].v.i8%2) * .5f + rs*(this->properties[2].v.i8 >> 4)/15.f,
                    //(bv.y) * (.5f+(float)this->properties[0].v.i8) + (float)(this->properties[1].v.i8/2) * .5f + rs*(this->properties[2].v.i8 & 15)/15.f

                    (bv.x) * (.5f + size) + (float)(mat%2) * .5f + rs*(desc.r >> 4)/15.f,
                    (bv.y) * (.5f + size) + (float)(mat/2) * .5f + rs*(desc.r & 15)/15.f
                };
            }

            num++;
        }

        c->layer_entities[z].mesh->i_start = start*indices_per_tpixel;
        c->layer_entities[z].mesh->i_count = (num-start)*indices_per_tpixel;
    }

    if (num)
        tms_gbuffer_upload_partial(vbuf[s], num*vertices_per_tpixel*sizeof(struct vertex));

    c->grass_entity[0].mesh->i_start = 0;
    c->grass_entity[0].mesh->i_count = num_grass_01/4*6;

    c->grass_entity[1].mesh->i_start = num_grass_01/4*6;
    c->grass_entity[1].mesh->i_count = (num_grass-num_grass_01)/4*6;

    if (num_grass)
        tms_gbuffer_upload_partial(grass_vbuf[s], num_grass*sizeof(struct grass_vertex));
}

static level_chunk *_c = 0;
static int last_cx;
static int last_cy;
static float _px, _py;
static float _ax, _ay;
static unsigned char tex[CAVEVIEW_SIZE*CAVEVIEW_SIZE];

void flood_fill(chunk_window *win, int x, int y)
{
    level_chunk *c;
    int sx;
    int sy;
    int cx,cy;

    if (x < 0 || x >= CAVEVIEW_SIZE) return;
    if (y < 0 || y >= CAVEVIEW_SIZE) return;
    if (tex[y*CAVEVIEW_SIZE+x] > 0 ) return;

    int yy = (int)floorf(2*_py) + y - CAVEVIEW_SIZE/2;
    cy = (int)floorf(yy / 16.f);
    sy = yy - cy*16;

    int xx = (int)floorf(2*_px) + x - CAVEVIEW_SIZE/2;
    cx = (int)floorf(xx / 16.f);
    sx = xx - cx*16;

    if (cx != last_cx || cy != last_cy) {
        _c = win->get_chunk(cx, cy);
        last_cx = cx;
        last_cy = cy;
    }

    c = _c;

    if (!c) {
        return;
    }

    unsigned char col;
    bool fill = true;

    float distsq = powf(xx*.5f - _ax, 2.f)+powf(yy*.5-_ay, 2.f);

    if (!c->pixels[2][sy][sx]) {

        if (c->pixels[1][sy][sx]) {
            fill = false;
            col = 1;
        } else {
            col = 255;
        }
    } else {
        if (c->pixels[1][sy][sx]) {
            /* pixel is blocked */
            col = 1;
        } else {

            col = 255-(unsigned char)tclampf(distsq*4.f, 0, 250.f);
        }
    }

    tex[y*CAVEVIEW_SIZE+x] = col;

    if (col <= 1) return;

    if (fill) {
        flood_fill(win, x+1, y);
        flood_fill(win, x, y+1);
        flood_fill(win, x-1, y);
        flood_fill(win, x, y-1);
    }
}

/** 
 * Create the texture that is used for terrain visibility
 *
 * This function is NOT thread safe
 **/
void
chunk_window::recreate_caveview_texture(float px, float py, float ax, float ay)
{
    memset(tex, 0, sizeof(tex));

    _px = px;
    _py = py;
    _ax = ax;
    _ay = ay;

    terrain_coord coord(px, py);
    int cx = coord.chunk_x, cy = coord.chunk_y;
    last_cx = ~cx;
    last_cy = ~cy;

    flood_fill(this, CAVEVIEW_SIZE/2, CAVEVIEW_SIZE/2);

    level_chunk *c;
    int sx;
    int sy;

    unsigned char *buf = tms_texture_get_buffer(this->caveview);

    /* blur it once */
#if 1
    for (int y=0; y<CAVEVIEW_SIZE; y++) {
        for (int x=0; x<CAVEVIEW_SIZE; x++) {

            /* light up if layer 3 is empty */
            {
                int yy = (int)floorf(2*_py) + y - CAVEVIEW_SIZE/2;
                cy = (int)floorf(yy / 16.f);
                sy = yy - cy*16;

                int xx = (int)floorf(2*_px) + x - CAVEVIEW_SIZE/2;
                cx = (int)floorf(xx / 16.f);
                sx = xx - cx*16;

                if (cx != last_cx || cy != last_cy) {
                    _c = this->get_chunk(cx, cy);
                    last_cx = cx;
                    last_cy = cy;
                }

                c = _c;

                if (!c->pixels[2][sy][sx]) {
                    tex[y*CAVEVIEW_SIZE+x] = 255;
                }
            }

            buf[y*CAVEVIEW_SIZE + x] = tex[y*CAVEVIEW_SIZE + x];

            if (buf[y*CAVEVIEW_SIZE + x] <= 1) {
                int n = 0;
                for (int dy=-2; dy<=2; dy++) {
                    for (int dx=-2; dx<=2; dx++) {
                        if (y+dy >= 0 && y+dy < CAVEVIEW_SIZE && x+dx >= 0 && x+dx < CAVEVIEW_SIZE && !(dx == 0 && dy == 0)) {
                            n += tex[(y+dy)*CAVEVIEW_SIZE + x+dx]/8;
                        }
                    }
                }
                buf[y*CAVEVIEW_SIZE + x] = n > 255 ? 255 : (unsigned char)n;
            }
        }
    }
#else
    memcpy(buf, tex, sizeof(tex));
#endif

    tms_texture_upload(this->caveview);

#if 0
    glDisable(GL_DEPTH_TEST);
    tms_texture_render(this->caveview);
    SDL_GL_SwapWindow((SDL_Window*)_tms._window);
    SDL_Delay(20);
#endif
}

/**
 * Step the chunk window, handle unloading of distant chunks
 **/
void
chunk_window::step()
{
    if (!(W->level.flags & LVL_CHUNKED_LEVEL_LOADING)) {
        return;
    }
    /**
     * first loop through all loaded chunks and mark them as garbage if their content is inactive,
     **/
    for (std::map<chunk_pos, level_chunk*>::iterator i = this->preloader.active_chunks.begin();
            i != this->preloader.active_chunks.end(); i++) {

        level_chunk *c = i->second;

        c->garbage = false;

#if defined DEBUG_SPECIFIC_CHUNK && defined DEBUG_SPECIFIC_CHUNK_PIXELS
        if (c->pos_x == DEBUG_CHUNK_X && c->pos_y == DEBUG_CHUNK_Y) {
            for (int z=0; z<3; z++) {
                for (int y=0; y<16; y++) {
                    for (int x=0; x<16; x++) {
                        printf("%.2x", c->pixels[z][y][x]);

                    }
                    printf("\n");
                }
                printf("-----\n");
            }
        }
#endif

        if (c->slot == -1) {
            //tms_debugf("num fixtures %p = %d", c, i->second->num_fixtures);
            /* this chunk is loaded but not in a slot, we can unload it if we wish */
            if (i->second->num_fixtures == 0 && i->second->num_non_unloadable == 0) {
                if (!c->is_focused()) {
                    c->unload_ticks --;
                    if (c->unload_ticks <= 0) {
                        c->garbage = true;
                    }
                    //tms_infof("num active at %p %dx%d %d", i->second, i->first.x, i->first.y, i->second->num_fixtures);
                } else {
                    c->unload_ticks = c->unload_ticks_max;
                }
            } else {
                c->unload_ticks = c->unload_ticks_max;
            }

            if (!c->garbage && (c->is_focused() || i->second->num_fixtures > 0 || i->second->num_dyn_fixtures > 0)) {
                c->load_neighbours();
            }
        } else {
            this->preloader.load(c, 2);
            c->unload_ticks = c->unload_ticks_max;
        }
    }

    /* second iteration, remove chunks marked as garbage if all their neighbours are marked
     * as garbage or are unloaded */
    for (std::map<chunk_pos, level_chunk*>::iterator i = this->preloader.active_chunks.begin();
            i != this->preloader.active_chunks.end(); ) {
        level_chunk *c = i->second;

        if (c->garbage) {
            bool found_active = false;

            for (int x=0; x<8; x++) {
                if (c->neighbours[x] && !c->neighbours[x]->garbage) {
                    found_active = true;
                    break;
                }
            }

            if (!found_active) {
                //tms_debugf("moving chunk [%d,%d] to wastebin", c->pos_x, c->pos_y);
                c->waste_retry_wait = 0;
                c->clear_chunk_neighbours();
                this->preloader.wastebin.insert(std::make_pair(i->first, i->second));
                this->preloader.active_chunks.erase(i++);
            } else {
                i++;
            }
        } else {
            i++;
        }
    }

    static const int MAX_REMOVE_PER_STEP = 1 + this->preloader.wastebin.size()/64;
    float num_rem = 0.f;
    /* lastly remove a few chunks in the wastebin permanently */

    for (std::map<chunk_pos, level_chunk*>::iterator i = this->preloader.wastebin.begin();
            i != this->preloader.wastebin.end(); i++) {

        level_chunk *c = i->second;

        if (c->waste_retry_wait > 0) {
            c->waste_retry_wait --;
        }
    }
    bool cont;
    do {
        cont = false;
        for (std::map<chunk_pos, level_chunk*>::iterator i = this->preloader.wastebin.begin();
                i != this->preloader.wastebin.end() && num_rem < MAX_REMOVE_PER_STEP; ) {

            if (this->preloader.wastebin.size() > 1) {
                std::advance(i, rand()%(this->preloader.wastebin.size()-1));
            }

            if (i == this->preloader.wastebin.end()) {
                cont = false;
                break;
            }

            chunk_pos p = i->first;
            level_chunk *c = i->second;

            if (!c->garbage) {
                this->preloader.unwaste_chunk(c);
                //cont = true;
                break;
            }

            if (c->waste_retry_wait > 0) {
                i ++;
                continue;
            }

            bool had_body = c->body != 0;

            if (this->preloader.unload(c)) {
                this->preloader.wastebin.erase(p);
                cont = true;

                if (had_body) {
                    num_rem += 1.f;
                }
                //delete c;
                break;
            } else {
                num_rem += .125f;
            }

            c->waste_retry_wait = 5+rand()%100;
            tms_debugf("could not unload chunk! something is probably connected to a non-garbage chunk");
            i++;
        }
    } while (cont && num_rem < MAX_REMOVE_PER_STEP);

    this->preloader.push_loaded();
}

void
chunk_window::set(b2Vec2 lower, b2Vec2 upper)
{
    b2Vec2 c = (upper-lower);

    int new_x = floorf(lower.x / 8.f)-2;
    int new_y = floorf(lower.y / 8.f)-2;
    int new_w = ceilf(c.x / 8.f)+4;
    int new_h = ceilf(c.y / 8.f)+4;

    if (!this->isset
        || (new_x != this->x
        || new_y != this->y
        || new_w != this->w
        || new_h != this->h)
        ) {
        //tms_debugf("update: x,y = [%d,%d], w,h = [%d,%d]",new_x,new_y,new_w,new_h);

        /* unload chunks that left the window */
        for (int x=0; x<MAX_CHUNKS; x++) {
            if (this->slots[x]) {
                if (this->slots[x]->pos_y < new_y || this->slots[x]->pos_y > new_y+new_h
                    || this->slots[x]->pos_x < new_x || this->slots[x]->pos_x > new_x+new_w) {
                    /* chunk is outside */
                    this->unload_slot(x);
                }
            }
        }

        for (int y=new_y; y<=new_y+new_h; y++) {
            for (int x=new_x; x<=new_x+new_w; x++) {

                /* we only need to load the chunk if it wasn't contained
                 * in the old window */
                if (!this->isset ||
                        (y < this->y+1 || y > this->y+this->h-1
                         || x < this->x+1 || x > this->x+this->w-1)) {

                    level_chunk *c = this->get_chunk(x,y);

                    //tms_debugf("chunk window set loaded chunk %d %d", c->pos_x, c->pos_y);

                    if (c->slot != -1) {
                        continue;
                    }

                    this->preloader.load(c, 2);

                    /* the outer-most chunks will not load into the view but only
                     * be added to the world */

                    if (y == new_y || x == new_x || y == new_y+new_h || x == new_x+new_w) {
                        /* do nothing */
                    } else {
                        int s = this->find_slot();
                        if (s != -1) {
                            this->load_slot(s, c);
                        } else {
                            tms_debugf("could not find open slot");
                        }
                    }
                }

            }
        }

        this->x = new_x;
        this->y = new_y;
        this->w = new_w;
        this->h = new_h;

        this->isset = true;
    }

    this->preloader.push_loaded();
}
