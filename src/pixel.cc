#include "pixel.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"
#include "ui.hh"

static bool initialized = false;
static bool _modified = false;

struct vertex {
    tvec3 pos;
    tvec3 nor;
    tvec3 col;
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

static int vertices_per_pixel = 0;
static int indices_per_pixel = 0;

static float _cam_x = 0.f, _cam_y = 0.f;

#define MAX_PIXELS 8192*2

void
pixel::initialize()
{
    if (initialized)
        return;

    {
        struct tms_mesh *mm = mesh_factory::get_mesh(MODEL_BOX_NOTEX);
        vertices_per_pixel = mm->v_count;
        indices_per_pixel = mm->i_count;

        tms_infof("num verts %d num indices %d", vertices_per_pixel, indices_per_pixel);

        _ibuf = new tms::gbuffer(MAX_PIXELS * indices_per_pixel * sizeof(uint16_t));
        _ibuf->usage = GL_STATIC_DRAW;

        for (int x=0; x<3; x++) {

            _buf[x] = new tms::gbuffer(MAX_PIXELS * vertices_per_pixel * sizeof(struct vertex));
            _buf[x]->usage = GL_STREAM_DRAW;

            _va[x] = new tms::varray(3);
            _va[x]->map_attribute("position", 3, GL_FLOAT, _buf[x]);
            _va[x]->map_attribute("normal", 3, GL_FLOAT, _buf[x]);
            _va[x]->map_attribute("color", 3, GL_FLOAT, _buf[x]);

            _mesh[x] = tms_mesh_alloc(_va[x], _ibuf);
            _e[x] = tms_entity_alloc();
            _e[x]->prio = x;
            tms_entity_set_mesh(_e[x], _mesh[x]);
            tms_entity_set_material(_e[x], static_cast<struct tms_material*>(&m_pixel));
            tmat4_load_identity(_e[x]->M);
            tmat3_load_identity(_e[x]->N);
        }

        uint16_t *i = (uint16_t*)_ibuf->get_buffer();
        uint16_t *ri = (uint16_t*)((char*)tms_gbuffer_get_buffer(mm->indices)+mm->i_start*2);
        uint16_t ibase = mm->v_start / sizeof(struct cvert);

        for (int x=0; x<MAX_PIXELS; x++) {
            for (int y=0; y<indices_per_pixel; y++) {
                i[x*indices_per_pixel + y] = ri[y] - ibase + x*vertices_per_pixel;
            }
        }

        _ibuf->upload();
    }

    initialized = true;
}

void
pixel::remove_from_world()
{
    basepixel::remove_from_world();
    W->remove_receiver(this->properties[4].v.i, this);
}

void
pixel::reset_counter()
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
pixel::get_entity(int x)
{
    return _e[x];
}

void
pixel::upload_buffers(void)
{
    if (!_modified) return;

    _modified = false;

    for (int x=0; x<3; x++) {
        int count = _counter[x];
        if (count > (MAX_PIXELS/2)-1) {
            count = (MAX_PIXELS/2)-1;
        }
        _mesh[x]->i_start = 0;
        _mesh[x]->i_count = (count*indices_per_pixel);

        if (count) {
            _buf[x]->upload_partial((count*vertices_per_pixel) * sizeof(struct vertex));
        }
    }
}

pixel::pixel()
{
    if (!initialized) {
        initialize();
    }

    this->last_size = -1.f;

    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_PIXEL_COLOR;

    this->set_num_properties(5);
    this->properties[0].type = P_INT8;
    this->properties[0].v.i8 = 1; /* Size */
    this->properties[1].type = P_FLT;
    this->properties[2].type = P_FLT;
    this->properties[3].type = P_FLT;
    this->properties[4].type = P_INT8;
    this->set_property(1, .8f); /* red */
    this->set_property(2, .8f); /* green */
    this->set_property(3, .8f); /* blue */
    this->properties[4].v.i8 = 0; /* Receiver Frequency */

    this->update_method = ENTITY_UPDATE_STATIC_CUSTOM;

    this->set_mesh(mesh_factory::get_mesh(MODEL_BOX_NOTEX));
    this->set_material(&m_pv_colored);

    this->r = 0.f;
    this->g = 0.f;
    this->b = 0.f;

    this->update_appearance();

    this->alpha = 1.f;
    this->dynamic = false;
    this->optimized_render = false;
}

void pixel::update()
{
    //if (this->body) {

    if (this->optimized_render) {
        b2Vec2 p = this->get_position();
        float a = this->get_angle();
        float cs,sn;
        tmath_sincos(a, &sn, &cs);

        int l = this->prio;

        float r = this->properties[1].v.f;
        float g = this->properties[2].v.f;
        float b = this->properties[3].v.f;

        if (_counter[l] < MAX_PIXELS) {
            int num = __sync_fetch_and_add(&_counter[l], 1);

            int base = (num*vertices_per_pixel);
            struct vertex *v = ((struct vertex*)_buf[l]->get_buffer());
            v += base;

            struct tms_mesh *mm = mesh_factory::get_mesh(MODEL_BOX_NOTEX);
            struct tms_gbuffer *r_vbuf = mm->vertex_array->gbufs[0].gbuf;

            struct cvert *rv = (struct cvert*)((char*)tms_gbuffer_get_buffer(r_vbuf)+mm->v_start);
            int num_rv = mm->v_count;

            float as = this->alpha * this->get_size()*2.f;
            float zs = tclampf(as, 0.f, 1.f);

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
                if (_tms.gamma_correct) {
                    v[x].col = (tvec3){
                        powf(r, 2.2),powf(g, 2.2),powf(b, 2.2)
                    };
                } else {
                    v[x].col = (tvec3){
                        r,g,b
                    };
                }
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

        float as = this->alpha * this->get_size()*2.f;

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
pixel::mstep()
{
    this->alpha = 1.f-this->pending_value;

    if (fabsf(this->alpha - this->last_size) > .01f) {
        this->update_fixture();
        this->last_size = this->alpha;
        G->force_static_update = 1;
    }
    this->reset_recv_value();

#if 0
    if (!this->dynamic) {

        buf[buf_p] = (bool)this->alpha;
        buf_p ++;
        buf_p = buf_p%PIXEL_DYN_BUF;

        if (buf_p == 0) {

            bool last = buf[0];
            int x;
            for (x=1; x<PIXEL_DYN_BUF; x++) {
                if (buf[x] == last)
                    break;

                last = buf[x];
            }

            if (false && x == PIXEL_DYN_BUF) {
                this->dynamic = true;
                //this->recreate_shape(0);
                this->alpha = 1.f;
                this->update_appearance();
                this->alpha = .95f;
                this->update_fixture();
                this->alpha = 1.f;

                //this->recreate_shape(true, true);
                //this->recreate_all_connections();
                /*
                this->body->SetType(b2_dynamicBody);
                this->body->ResetMassData();
                */
                return;
            }
        }
    }
    this->update();
#endif
}

void
pixel::init()
{
    if (this->properties[4].v.i == 0) {
        /* make sure we are NOT added to the worlds stepable list */
        W->mstepable.erase(this);
    } else {
        W->mstepable.insert(this);
        W->add_receiver(this->properties[4].v.i, this);
        memset(this->buf, 0, sizeof(this->buf));
        this->buf_p = 0;
    }
}

void
pixel::setup()
{
    this->last_size = -1.f;
    this->reset_recv_value();

    this->dynamic = false;
    this->update_appearance();
}

void
pixel::on_pause()
{
    this->dynamic = false;
    this->alpha = 1.f;
    this->last_size = -1.f;

    basepixel::on_pause();
}

void
pixel::on_load(bool created, bool has_state)
{
    basepixel::on_load(created, has_state);

    this->set_optimized_render(this->properties[4].v.i8 == 0);
}

void
pixel::set_optimized_render(bool enable)
{
    this->optimized_render = enable;

    if (enable) {
        tms_entity_set_mesh((struct tms_entity*)this, 0);
        //this->set_mesh((struct tms_mesh*)0);
    } else {
        this->set_mesh(mesh_factory::get_mesh(MODEL_BOX_NOTEX));
    }
}

void
pixel::update_fixture()
{
    if (this->body) {
        b2PolygonShape *shape = static_cast<b2PolygonShape*>(this->fx->GetShape());
        float s = this->get_size() * 1.001f; /* XXX: This is very important */
        float sz = fmaxf(this->alpha, .1f)*s;
        shape->SetAsBox(sz, sz, this->_pos, this->_angle);
    }
}

void
pixel::update_appearance()
{
    float r = this->properties[1].v.f;
    float g = this->properties[2].v.f;
    float b = this->properties[3].v.f;

    this->set_uniform("~color", r, g, b, 1.f);
    this->set_optimized_render(this->properties[4].v.i8 == 0);

    if (this->r != this->properties[1].v.f || this->g != this->properties[2].v.f || this->b != this->properties[3].v.f) {
        this->r = r;
        this->g = g;
        this->b = b;
        if (G) G->force_static_update = 1;
    }
}

void
pixel::construct()
{
    basepixel::construct();

    this->set_optimized_render(this->properties[4].v.i8 == 0);
}

void
pixel::recreate_shape(bool skip_search, bool dynamic)
{
    if (this->body && this->fx) {
        this->body->DestroyFixture(this->fx);
        this->fx = 0;
    }

    float s = this->get_size() * fmaxf(this->alpha, 0.1f);

    this->create_rect(dynamic?b2_dynamicBody : b2_staticBody, s, s, this->material);

    if (!skip_search) {
        this->set_position(this->get_position().x, this->get_position().y, 0);
    }
}

void
pixel::set_color(tvec4 c)
{
    this->properties[1].v.f = c.r;
    this->properties[2].v.f = c.g;
    this->properties[3].v.f = c.b;

    this->update_appearance();
}

tvec4
pixel::get_color()
{
    float r = this->properties[1].v.f;
    float g = this->properties[2].v.f;
    float b = this->properties[3].v.f;

    return tvec4f(r, g, b, 1.0f);
}
