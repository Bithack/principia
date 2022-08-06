#include "tpixel.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"
#include "object_factory.hh"
#include "resource.hh"

struct tpixel_material tpixel_materials[NUM_TPIXEL_MATERIALS] = {
    {   "Grass",
        &m_tpixel,
        (tvec4){1.f, 1.f, 1.f, 1.f},
        {
            .999f,  /* Ruby */
            .999f,  /* Sapphire */
            .999f,  /* Emerald */
            .999f,  /* Topaz */
            .9999f, /* Diamond */
            -1.f,   /* Copper */
            -1.f,   /* Iron */
            .5f,    /* Wood */
            -1.f,   /* Aluminium */
        }
    }, {"Dirt",
        &m_tpixel,
        (tvec4){1.f, 1.f, 1.f, 1.f},
        {
            .995f,  /* Ruby */
            .995f,  /* Sapphire */
            .995f,  /* Emerald */
            .995f,  /* Topaz */
            .9995f, /* Diamond */
            .99f,   /* Copper */
            .99f,   /* Iron */
            -1.f,   /* Wood */
            -1.f,   /* Aluminium */
        }
    }, {"Stone",
        &m_tpixel,
        (tvec4){1.f, 1.f, 1.f, 1.f},
        {
            .95f,   /* Ruby */
            .95f,   /* Sapphire */
            .95f,   /* Emerald */
            .95f,   /* Topaz */
            .95f,   /* Diamond */
            .95f,   /* Copper */
            .95f,   /* Iron */
            -1.f,   /* Wood */
            -1.f,   /* Aluminium */
        }
    }, {"Hard Stone",
        &m_tpixel,
        (tvec4){1.f, 1.f, 1.f, 1.f},
        {
            .9f,    /* Ruby */
            .9f,    /* Sapphire */
            .9f,    /* Emerald */
            .9f,    /* Topaz */
            .9f,    /* Diamond */
            .9f,    /* Copper */
            .9f,    /* Iron */
            -1.f,   /* Wood */
            -1.f,   /* Aluminium */
        }
    }, {"Diamond Ore",
        &m_gem,
        (tvec4)RESOURCE_COLOR_DIAMOND,
        {
            -1.f,   /* Ruby */
            -1.f,   /* Sapphire */
            -1.f,   /* Emerald */
            -1.f,   /* Topaz */
            .0f,    /* Diamond */
            -1.f,   /* Copper */
            -1.f,   /* Iron */
            -1.f,   /* Wood */
            -1.f,   /* Aluminium */
        }
    }, {"Ruby Ore",
        &m_gem,
        (tvec4)RESOURCE_COLOR_RUBY,
        {
            0.f,   /* Ruby */
            -1.f,   /* Sapphire */
            -1.f,   /* Emerald */
            -1.f,   /* Topaz */
            -1.f,    /* Diamond */
            -1.f,   /* Copper */
            -1.f,   /* Iron */
            -1.f,   /* Wood */
            -1.f,   /* Aluminium */
        }
    }, {"Sapphire Ore",
        &m_gem,
        (tvec4)RESOURCE_COLOR_SAPPHIRE,
        {
            -1.f,   /* Ruby */
            0.f,   /* Sapphire */
            -1.f,   /* Emerald */
            -1.f,   /* Topaz */
            -1.f,    /* Diamond */
            -1.f,   /* Copper */
            -1.f,   /* Iron */
            -1.f,   /* Wood */
            -1.f,   /* Aluminium */
        }
    }, {"Emerald Ore",
        &m_gem,
        (tvec4)RESOURCE_COLOR_EMERALD,
        {
            -1.f,   /* Ruby */
            -1.f,   /* Sapphire */
            0.f,   /* Emerald */
            -1.f,   /* Topaz */
            -1.f,    /* Diamond */
            -1.f,   /* Copper */
            -1.f,   /* Iron */
            -1.f,   /* Wood */
            -1.f,   /* Aluminium */
        }
    }, {"Topaz Ore",
        &m_gem,
        (tvec4)RESOURCE_COLOR_TOPAZ,
        {
            -1.f,   /* Ruby */
            -1.f,   /* Sapphire */
            -1.f,   /* Emerald */
            0.f,   /* Topaz */
            -1.f,    /* Diamond */
            -1.f,   /* Copper */
            -1.f,   /* Iron */
            -1.f,   /* Wood */
            -1.f,   /* Aluminium */
        }
    }, {"Copper Ore",
        &m_gem,
        (tvec4)RESOURCE_COLOR_COPPER,
        {
            -1.f,   /* Ruby */
            -1.f,   /* Sapphire */
            -1.f,   /* Emerald */
            -1.f,   /* Topaz */
            -1.f,    /* Diamond */
            0.f,   /* Copper */
            -1.f,   /* Iron */
            -1.f,   /* Wood */
            -1.f,   /* Aluminium */
        }
    }, {"Iron Ore",
        &m_gem,
        (tvec4)RESOURCE_COLOR_IRON,
        {
            -1.f,   /* Ruby */
            -1.f,   /* Sapphire */
            -1.f,   /* Emerald */
            -1.f,   /* Topaz */
            -1.f,    /* Diamond */
            -1.f,   /* Copper */
            0.f,   /* Iron */
            -1.f,   /* Wood */
            -1.f,   /* Aluminium */
        }
    }, {"Aluminium Ore",
        &m_gem,
        (tvec4)RESOURCE_COLOR_ALUMINIUM,
        {
            -1.f,   /* Ruby */
            -1.f,   /* Sapphire */
            -1.f,   /* Emerald */
            -1.f,   /* Topaz */
            -1.f,    /* Diamond */
            -1.f,   /* Copper */
            -1.f,   /* Iron */
            -1.f,   /* Wood */
            0.f,   /* Aluminium */
        }
    },
};

static bool initialized = false;
static bool _modified = false;

struct vertex {
    tvec3 pos;
    tvec3 nor;
    tvec2 uv;
} __attribute__ ((packed));

struct cvert {
    tvec3 p;
    tvec3 n;
    tvec2 u;
} __attribute__((packed));

static struct tms_mesh   *_mesh[3];
static struct tms_entity *_e[3];
static tms::varray *_va[3];
static tms::gbuffer *_buf[3];
static tms::gbuffer *_ibuf;
static volatile int _counter[3] = {0,0,0};

static int vertices_per_tpixel = 0;
static int indices_per_tpixel = 0;

static float _cam_x = 0.f, _cam_y = 0.f;

#define MAX_TPIXELS 8192*2

void
tpixel::initialize()
{
    if (initialized)
        return;

    {
        struct tms_mesh *mm = mesh_factory::get_mesh(MODEL_BOX_TEX);
        vertices_per_tpixel = mm->v_count;
        indices_per_tpixel = mm->i_count;

        tms_infof("num verts %d num indices %d", vertices_per_tpixel, indices_per_tpixel);

        _ibuf = new tms::gbuffer(MAX_TPIXELS * indices_per_tpixel * sizeof(uint16_t));
        _ibuf->usage = GL_STATIC_DRAW;

        for (int x=0; x<3; x++) {

            _buf[x] = new tms::gbuffer(MAX_TPIXELS * vertices_per_tpixel * sizeof(struct vertex));
            _buf[x]->usage = GL_STREAM_DRAW;

            _va[x] = new tms::varray(3);
            _va[x]->map_attribute("position", 3, GL_FLOAT, _buf[x]);
            _va[x]->map_attribute("normal", 3, GL_FLOAT, _buf[x]);
            _va[x]->map_attribute("texcoord", 2, GL_FLOAT, _buf[x]);

            _mesh[x] = tms_mesh_alloc(_va[x], _ibuf);
            _e[x] = tms_entity_alloc();
            _e[x]->prio = x;
            tms_entity_set_mesh(_e[x], _mesh[x]);
            tms_entity_set_material(_e[x], static_cast<struct tms_material*>(&m_tpixel));
            tmat4_load_identity(_e[x]->M);
            tmat3_load_identity(_e[x]->N);
        }

        uint16_t *i = (uint16_t*)_ibuf->get_buffer();
        uint16_t *ri = (uint16_t*)((char*)tms_gbuffer_get_buffer(mm->indices)+mm->i_start*2);
        uint16_t ibase = mm->v_start / sizeof(struct cvert);

        for (int x=0; x<MAX_TPIXELS; x++) {
            for (int y=0; y<indices_per_tpixel; y++) {
                i[x*indices_per_tpixel + y] = ri[y] - ibase + x*vertices_per_tpixel;
            }
        }

        _ibuf->upload();
    }

    initialized = true;
}

void
tpixel::reset_counter()
{
    _modified = true;

    for (int x=0; x<3; x++) {
        _counter[x] = 0;
        tmat4_load_identity(_e[x]->M);
        tmat4_translate(_e[x]->M, 0.f, 0.f, x*LAYER_DEPTH);
    }

    /* TODO relative to camera */
}

struct tms_entity*
tpixel::get_entity(int x)
{
    return _e[x];
}

void
tpixel::upload_buffers(void)
{
    if (!_modified) return;

    _modified = false;

    for (int x=0; x<3; x++) {
        int count = _counter[x];
        if (count > (MAX_TPIXELS/2)-1) {
            count = (MAX_TPIXELS/2)-1;
        }
        _mesh[x]->i_start = 0;
        _mesh[x]->i_count = (count*indices_per_tpixel);

        if (count)
            _buf[x]->upload_partial((count*vertices_per_tpixel) * sizeof(struct vertex));
    }
}

tpixel::tpixel()
{
    if (!initialized)
        initialize();

    this->num_sliders = 2;
    this->set_flag(ENTITY_IS_INTERACTIVE, true);
    this->set_flag(ENTITY_IS_ZAPPABLE,    true);

    this->set_num_properties(3);
    this->properties[0].type = P_INT8;
    this->properties[0].v.i8 = 1; /* Size */
    this->properties[1].type = P_INT8;
    this->properties[2].type = P_INT8; /* random seed for texture variation */
    this->properties[2].v.i8 = (uint8_t)(1+rand()%254);

    tms_entity_init(&this->ore);
    tms_entity_set_mesh(&this->ore, mesh_factory::get_mesh((this->properties[2].v.i8 %2) == 0 ?MODEL_ORE:MODEL_ORE2));
    tms_entity_set_material(&this->ore, &m_tpixel);

    this->set_block_type(rand()%NUM_TPIXEL_MATERIALS);

    this->update_method = ENTITY_UPDATE_STATIC_CUSTOM;

    this->update_appearance();
}

void
tpixel::on_load(bool created, bool has_state)
{
    basepixel::on_load(created, has_state);
    this->desc.reset();
    this->desc.size = this->properties[0].v.i8;
    this->set_block_type(this->properties[1].v.i8);
}

void
tpixel::prepare_fadeout()
{
    this->optimized_render = false;
    if (this->mesh == 0) {
        this->set_mesh(mesh_factory::get_mesh(MODEL_BOX_TEX));
    }
    this->update();
    this->M[14]+=.01f;
}

void
tpixel::setup()
{
    //this->interactive_hp = this->properties[0].v.i8;
    this->interactive_hp = 1.f;
    this->oil = 100.f * (1.f+this->properties[1].v.i8*3) * (1.f+this->properties[0].v.i8*3);
    this->desc.reset();

    switch (this->properties[1].v.i8) {
        case 0: break; /* grass */
        case 1: this->interactive_hp = 2.f; break; /* dirt */
        case 2: this->interactive_hp = 20.f; break; /* stone */
        case 3: this->interactive_hp = 100.f; break; /* hard stone */
    }

    basepixel::setup();
}

float
tpixel::get_slider_value(int s)
{
    if (s == 0) {
        return basepixel::get_slider_value(s);
    } else {
        return ((float)this->properties[1].v.i8) / (NUM_TPIXEL_MATERIALS-1);
    }
}

float
tpixel::get_slider_snap(int s)
{
    if (s == 0) {
        return basepixel::get_slider_snap(s);
    } else {
        return 1.f/(NUM_TPIXEL_MATERIALS-1);
    }
}

void
tpixel::set_block_type(int mat)
{
    mat = std::min(NUM_TPIXEL_MATERIALS-1, mat);

    this->properties[1].v.i8 = mat;

    this->desc.material = mat;

    if (this->ore.parent) {
        tms_entity_remove_child(this, &this->ore);
    }
    if (this->ore.scene) {
        tms_scene_remove_entity(this->scene, &this->ore);
    }

    tms_entity_set_mesh(&this->ore, mesh_factory::get_mesh((this->properties[2].v.i8 %2) == 0 ?MODEL_ORE:MODEL_ORE2));

    if (mat <= TPIXEL_MATERIAL_ROCK) {
        this->optimized_render = true;
        tms_entity_set_mesh((struct tms_entity*)this, 0);
        this->set_material(&m_tpixel);
    } else {
        this->optimized_render = false;
        this->set_mesh(mesh_factory::get_mesh((this->properties[2].v.i8 %3) == 0 ?MODEL_ORE_INSIDE:MODEL_ORE2_INSIDE));
        this->set_material(tpixel_materials[mat].material);
        this->set_uniform("~color", TVEC4_INLINE(tpixel_materials[mat].color));

        if (!this->ore.parent) {
            tms_entity_add_child(this, &this->ore);
        }
        if (!this->ore.scene && this->scene) {
            tms_scene_add_entity(this->scene, &this->ore);
        }
    }
}

void
tpixel::on_slider_change(int s, float value)
{
    if (s == 0 || s == -1) {
        basepixel::on_slider_change(s, value);
    } else {
        uint8_t old_block_type = this->properties[1].v.i8;
        uint8_t new_block_type = (uint8_t)roundf(value * NUM_TPIXEL_MATERIALS);

        if (old_block_type != new_block_type) {
            this->set_block_type(new_block_type);
            G->refresh_info_label();
        }
    }
}

void
tpixel::construct()
{
    /* make sure the texture variation is not copied */
    this->properties[2].v.i8 = 1+rand()%254;

    basepixel::construct();
}

void
tpixel::update()
{
    //if (this->body) {

    if (this->optimized_render && this->body) {
        b2Vec2 p = this->get_position();
        float a = this->get_angle();
        float cs,sn;
        tmath_sincos(a, &sn, &cs);

        int l = this->prio;

        float rs = (3.f - this->properties[0].v.i8)*(.125f);

        if (_counter[l] < MAX_TPIXELS) {
            int num = __sync_fetch_and_add(&_counter[l], 1);

            int base = (num*vertices_per_tpixel);
            struct vertex *v = ((struct vertex*)_buf[l]->get_buffer());
            v += base;

            struct tms_mesh *mm = mesh_factory::get_mesh(MODEL_BOX_TEX);
            struct tms_gbuffer *r_vbuf = mm->vertex_array->gbufs[0].gbuf;

            struct cvert *rv = (struct cvert*)((char*)tms_gbuffer_get_buffer(r_vbuf)+mm->v_start);
            int num_rv = mm->v_count;

            float as = this->get_size()*2.f;
            //float zs = tclampf(as, 0.f, 1.f);
            float zs = 1.5f+.125f*(((float)this->properties[0].v.i8));

            for (int x=0; x<num_rv; x++) {

                v[x].pos = (tvec3){
                    rv[x].p.x * as * cs - rv[x].p.y*as * sn + p.x - _cam_x,
                    rv[x].p.x * as * sn + rv[x].p.y*as * cs + p.y - _cam_y,
                    rv[x].p.z * zs
                };
                v[x].nor = (tvec3){
                    rv[x].n.x * cs - rv[x].n.y * sn,
                    rv[x].n.x * sn + rv[x].n.y * cs,
                    rv[x].n.z
                };

                tvec2 bv = (tvec2){rv[x].u.x - .125f/2.f, rv[x].u.y-.5f-.125f/2.f};

                if (rv[x].p.z < 0.f) {
                    bv.x /= std::max(0.5f, (float)this->properties[0].v.i8*2);
                    bv.y /= std::max(0.5f, (float)this->properties[0].v.i8*2);
                }

                if (this->properties[0].v.i8 == 0) {
                    bv.x += .125f;
                    bv.y += .125f;
                } else {
                    bv.x += .125f/2.f;
                    bv.y += .125f/2.f;
                }

                v[x].uv = tvec2f(
                    (bv.x) * (.5+(float)this->properties[0].v.i8) + (float)(this->properties[1].v.i8%2) * .5f + rs*(this->properties[2].v.i8 >> 4)/15.f,
                    (bv.y) * (.5f+(float)this->properties[0].v.i8) + (float)(this->properties[1].v.i8/2) * .5f + rs*(this->properties[2].v.i8 & 15)/15.f
                );
            }
        } else {
        }

    } else {
        b2Vec2 p = this->get_position();
        float a = this->get_angle();

        tmat4_load_identity(this->M);
        tmat4_translate(this->M, p.x, p.y, this->get_layer()*LAYER_DEPTH);
        tmat4_rotate(this->M, a*(180.f/M_PI), 0.f, 0.f, -1.f);

        tmat3_copy_mat4_sub3x3(this->N, this->M);

        float as = this->get_size()*2.f;

        tmat4_copy(this->ore.M, this->M);

        if (this->properties[1].v.i8 > TPIXEL_MATERIAL_ROCK) {
            tmat4_rotate(this->ore.M, -90+90*(this->properties[2].v.i8%5), 1.f, 0.f, 0.f);
            tmat3_copy_mat4_sub3x3(this->ore.N, this->ore.M);
            tmat4_scale(this->ore.M, as, as, tclampf(as, 0.f, 1.f));
        }

        //tmat4_scale(this->M, .8f, .8f,1.f);
        tmat4_scale(this->M, as, as, tclampf(as, 0.f, 1.f));
    }
        /*
    } else {
        tmat4_load_identity(this->M);
        tmat4_translate(this->M, this->_pos.x, this->_pos.y, 0);
        tmat4_rotate(this->M, this->_angle * (180.f/M_PI), 0, 0, -1);
        tmat3_copy_mat4_sub3x3(this->N, this->M);
    }
*/
}

void
tpixel::update_appearance()
{
}

void
tpixel::recreate_shape(bool skip_search, bool dynamic)
{
    if (this->body && this->fx) {
        this->body->DestroyFixture(this->fx);
        this->fx = 0;
    }

    float s = this->get_size() * 1.001f;
    this->create_rect(b2_staticBody, s,s, this->material);

    this->fx->SetUserData2(&this->desc);

    if (this->fx) {
        /* set material-specific properties */
        switch (this->properties[1].v.i8) {
            case 0: /* grass */
            case 1: /* dirt */
                this->fx->SetFriction(.9f);
                this->fx->SetRestitution(.3f);
                break;

            case 2:
            case 3: /* rock */
                this->fx->SetFriction(.4f);
                this->fx->SetRestitution(.6f);
                break;
        }
    }

    if (!skip_search) {
        this->set_position(this->get_position().x, this->get_position().y, 0);
    }
}

void
tpixel::drop_loot(int num/*=1*/)
{
    b2Vec2 pos = this->get_position();

    tms_debugf("jkljkljkl %d", num);

    for (int x=0; x<num; ++x) {
        for (int n=0; n<NUM_RESOURCES; ++n) {
            if (tpixel_materials[this->properties[1].v.i8].drops[n] == -1.f) continue;
            float r = rand() / (float)RAND_MAX;
            if (r > tpixel_materials[this->properties[1].v.i8].drops[n]) {
                resource *r = static_cast<resource*>(of::create(O_RESOURCE));
                r->set_resource_type(n);
                r->set_position(b2Vec2(pos.x+(0.25f*(1.f-(rand() / (float)RAND_MAX))), pos.y+(0.25f*(1.f-(rand() / (float)RAND_MAX)))));
                r->set_layer(this->get_layer());
                G->lock();
                G->emit(r, 0, b2Vec2(0,0));
                G->unlock();
            }
        }
    }
}
