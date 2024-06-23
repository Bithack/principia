#include "cable.hh"
#include "game.hh"
#include "edevice.hh"
#include "material.hh"
#include "model.hh"
#include "world.hh"
#include "ifdevice.hh"

#include <cstdlib>

#define QUALITY 4
#define SEGSIZE .5f
#define WIDTH (.125f*.375f)*.5f

#define EPSILON .1f

#define NUM_SEGS 6
#define MAX_CABLES 2048
#define MAX_PLUGS 2048 /* XXX: this needs to be overlooked */

#define NUM_SUBDIVISIONS 3

struct vertex {
    tvec3 pos;
    tvec3 nor;
} __attribute__ ((packed));

struct cvert {
    tvec3 p;
    tvec3 n;
    tvec2 u;
} __attribute__((packed));

static bool initialized = false;

static struct tms_mesh   *_mesh;
static struct tms_entity *_e;
static tms::varray *va = 0;
static tms::gbuffer *buf = 0;
static tms::gbuffer *ibuf = 0;

static volatile int plug_counter[3] = {0,0,0};
static struct tms_mesh   *plug_mesh[3];
static struct tms_entity *plug_e[3];
static tms::varray *plug_va[3];
static tms::gbuffer *plug_buf[3];
static tms::gbuffer *plug_ibuf;
static float _cam_x = 0.f, _cam_y = 0.f;

static int points_per_cable = 0;
static int vertices_per_plug = 0;
static int indices_per_plug = 0;

static volatile int counter = 0;

void
cable::_init(void)
{
    if (initialized)
        return;

    b2Vec2 dir;

    /* initialize cable buffers */
    {
        points_per_cable = (int)pow(2.f, (float)(NUM_SUBDIVISIONS))+1;
        buf = new tms::gbuffer(MAX_CABLES * points_per_cable * QUALITY * sizeof(struct vertex));
        ibuf = new tms::gbuffer(MAX_CABLES * (points_per_cable-1) * (QUALITY) * 6 * sizeof(uint16_t));

        buf->usage = GL_STREAM_DRAW;
        ibuf->usage = GL_STATIC_DRAW;

        va = new tms::varray(2);
        va->map_attribute("position", 3, GL_FLOAT, buf);
        va->map_attribute("normal", 3, GL_FLOAT, buf);

        _mesh = tms_mesh_alloc(va, ibuf);
        _e = tms_entity_alloc();
        tms_entity_set_mesh(_e, _mesh);
        tms_entity_set_material(_e, static_cast<struct tms_material*>(&m_cable));
        tmat4_load_identity(_e->M);
        tmat3_load_identity(_e->N);

        uint16_t *id = (uint16_t*)ibuf->get_buffer();

        int num_i = 0;
        int v_offs = 0;

        for (int z=0; z<MAX_CABLES; z++) {
            v_offs = (z*(points_per_cable)*QUALITY);
            for (int y=0; y<(points_per_cable); y++) {
                if (y>0) {
                    for (int x=0; x<QUALITY; x++) {
                        int c = y*(QUALITY) + x%QUALITY;
                        int p = (y-1)*(QUALITY) + x%QUALITY;

                        int c1 = y*(QUALITY) + (x+1)%QUALITY;
                        int p1 = (y-1)*(QUALITY) + (x+1)%QUALITY;

                        id[num_i+1] = v_offs + c;
                        id[num_i+0] = v_offs + p;
                        id[num_i+2] = v_offs + c1;

                        id[num_i+4] = v_offs + c1;
                        id[num_i+3] = v_offs + p;
                        id[num_i+5] = v_offs + p1;

                        num_i+=6;
                    }
                }
            }
        }

        ibuf->upload();
    }

    /* initialize plug buffers, one for each layer */
    {
        struct tms_mesh *mm = mesh_factory::get_mesh(MODEL_PLUG_SIMPLE);
        vertices_per_plug = mm->v_count;
        indices_per_plug = mm->i_count;

        plug_ibuf = new tms::gbuffer(MAX_PLUGS * indices_per_plug * sizeof(uint16_t));
        plug_ibuf->usage = GL_STATIC_DRAW;

        for (int x=0; x<3; x++) {
            plug_buf[x] = new tms::gbuffer(MAX_PLUGS * vertices_per_plug * sizeof(struct vertex));
            plug_buf[x]->usage = GL_STREAM_DRAW;

            plug_va[x] = new tms::varray(2);
            plug_va[x]->map_attribute("position", 3, GL_FLOAT, plug_buf[x]);
            plug_va[x]->map_attribute("normal", 3, GL_FLOAT, plug_buf[x]);

            plug_mesh[x] = tms_mesh_alloc(plug_va[x], plug_ibuf);
            plug_e[x] = tms_entity_alloc();
            plug_e[x]->prio = x;
            tms_entity_set_mesh(plug_e[x], plug_mesh[x]);
            tms_entity_set_material(plug_e[x], static_cast<struct tms_material*>(&m_cable_red));
            tmat4_load_identity(plug_e[x]->M);
            tmat3_load_identity(plug_e[x]->N);
        }

        uint16_t *i = (uint16_t*)plug_ibuf->get_buffer();
        uint16_t *ri = (uint16_t*)((char*)tms_gbuffer_get_buffer(mm->indices)+mm->i_start*2);
        uint16_t ibase = mm->v_start / sizeof(struct cvert);

        for (int x=0; x<MAX_PLUGS; x++) {
            for (int y=0; y<indices_per_plug; y++) {
                i[x*indices_per_plug + y] = ri[y] - ibase + x*vertices_per_plug;

                /*
                if (x ==0 && y == 0) {
                    tms_infof("first set to: %d - %d + %d*%d", ri[y], ibase, x, vertices_per_plug);
                    exit(1);
                }
                */
            }
        }

        plug_ibuf->upload();
    }

    tms_entity_add_child(_e, plug_e[0]);
    tms_entity_add_child(_e, plug_e[1]);
    tms_entity_add_child(_e, plug_e[2]);

    initialized = true;
}

void
cable::reset_counter(void)
{
    counter = 0;
    for (int x=0; x<3; x++)
        plug_counter[x] = 0;

    if (G) {
        _cam_x = G->cam->_position.x;
        _cam_y = G->cam->_position.y;
    }

    if (_e) {
        tmat4_load_identity(_e->M);
        tmat4_translate(_e->M, _cam_x, _cam_y, 0.f);
        for (int x=0; x<3; x++) {
            tmat4_load_identity(plug_e[x]->M);
            tmat4_translate(plug_e[x]->M, _cam_x, _cam_y, 0.f);
        }
    }
}

struct tms_entity*
cable::get_entity(void)
{
    return _e;
}

void
cable::upload_buffers(void)
{
    if (counter > MAX_CABLES-1) counter = MAX_CABLES-1;
    _mesh->i_start = 0;
    _mesh->i_count = counter*(ibuf->size/MAX_CABLES) / sizeof(uint16_t);
    if (counter)
        buf->upload_partial(counter * points_per_cable * 4 * sizeof(struct vertex)); /* this 4 = QUALITY? */

    for (int x=0; x<3; x++) {
        int count = plug_counter[x];
        if (count > (MAX_PLUGS)-1) {
            count = (MAX_PLUGS)-1;
        }
        plug_mesh[x]->i_start = 0;
        plug_mesh[x]->i_count = count*(indices_per_plug);

        if (count) {
            plug_buf[x]->upload_partial((count*vertices_per_plug) * sizeof(struct vertex));
        }
    }
}

cable::cable(int type)
{
    cable::_init();
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_ALLOW_ROTATION,       false);
    this->set_flag(ENTITY_CUSTOM_GHOST_UPDATE,  true);
    this->set_flag(ENTITY_DO_STEP,              true);

    this->ji = 0;
    this->freeze = false;
    this->update_method = ENTITY_UPDATE_CUSTOM;

    this->ready = true;

    this->p_in = -1;
    this->p_out = -1;

    this->type = ENTITY_CABLE;
    this->ctype = type;

    this->p[0] = 0;
    this->p[1] = 0;

    this->p[0] = new plug(this);
    this->p[1] = new plug(this);

    this->add_child(this->p[0]);
    this->add_child(this->p[1]);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->saved_length = 0.f;
    this->length = 8.f;

    this->joint = 0;
    this->extra_length = .0f+(rand()%100)/100.f * .2f;
    this->num_sliders = 1;
}

cable::~cable()
{
    if (this->ji) {
        this->ji->data = 0;
    }

    /*
    if (this->p[0]) {
        this->p[0]->c = 0;
    }
    if (this->p[1]) {
        this->p[1]->c = 0;
    }
    */
}

void
cable::construct()
{
    this->set_position(_pos.x, _pos.y);
}

float
cable::get_slider_snap(int s)
{
    return .1f;
}

float
cable::get_slider_value(int s)
{
    return this->extra_length / CABLE_MAX_EXTRA_LENGTH;
}

void
cable::on_slider_change(int s, float value)
{
    this->extra_length = value * CABLE_MAX_EXTRA_LENGTH;
    G->show_numfeed(this->extra_length);
}

void
cable::set_position(float x, float y, uint8_t frame/*=0*/)
{
    this->p[0]->set_position(x+.5f,y+.5f);
    this->p[1]->set_position(x-.5f,y-.5f);
}

void
cable::add_to_world()
{
    this->p[0]->add_to_world();
    this->p[1]->add_to_world();

    this->ready = true;
    this->create_joint();
}

void
cable::remove_from_world()
{
    this->p[0]->remove_from_world();
    this->p[1]->remove_from_world();
}

void
cable::pre_write(void)
{
    this->p[0]->pre_write();
    this->p[1]->pre_write();
}

void
plug::pre_write(void)
{
}

void
cable::destroy_joint()
{
    if (this->joint) {
        W->b2->DestroyJoint(this->joint);
        this->joint = 0;
    }
}


void
cable::create_joint()
{
    this->destroy_joint();

    //b2RopeJointDef rjd;
    //b2WeldJointDef rjd;
    //b2RevoluteJointDef rjd;
    //b2PrismaticJointDef rjd;

    b2Body *b[2];
    b2Vec2 a[2];
    entity *entities[2] = {0, };

    for (int x=0; x<2; x++) {
        if (this->p[x]->is_connected()) {
            entity *e = this->p[x]->plugged_edev->get_entity();
            entities[x] = e;

            if (e->get_body(0)) {
                //tms_infof("%d entity body ------", x);
                b[x] = e->get_body(0);
                a[x] = e->local_to_body(this->p[x]->s->lpos, 0);
            } else {
                b[x] = 0;
                tms_errorf("invalid body setup");
            }
        } else {
            b[x] = this->p[x]->body;
            a[x] = b2Vec2(0,0);
            //tms_infof("%d SELF body ------", x);
        }
    }

    if (entities[0] && entities[0] == entities[1]) {
        /* Both ends of the cable are plugged into the same entity,
         * set length to 0. */
        this->length = 0.f;

        return;
    }

    if (!b[0] || !b[1]) {
        /* Neither plug is connected to something with a body,
         * set length to 0. */
        this->length = 0.f;

        return;
    }

    bool same_group = false;

    if (b[0] == b[1] && entities[0] && entities[1] && entities[0]->gr == entities[1]->gr) {
        same_group = true;
    }

    if (this->saved_length > 0.f) {
        this->length = this->saved_length;
        this->saved_length = 0.f;
    } else {
        this->length = (b[0]->GetWorldPoint(a[0])-b[1]->GetWorldPoint(a[1])).Length() + EPSILON + this->extra_length;
    }

    if (!same_group) {
        b2RopeJointDef rjd;
        rjd.localAnchorA = a[0];
        rjd.localAnchorB = a[1];
        rjd.bodyA = b[0];
        rjd.bodyB = b[1];
        rjd.collideConnected = true;
        rjd.maxLength = this->length;

        if (!W->is_paused()) {
            if (!this->ji) {
                this->ji = new joint_info(JOINT_TYPE_CABLE, this);
            }
            (this->joint = (b2RopeJoint*)W->b2->CreateJoint(&rjd))->SetUserData(this->ji);
        } else {
            this->joint = 0;
        }
    }
}

void
cable::ghost_update(void)
{
    this->update();
}

void
cable::update(void)
{
    int num = __sync_fetch_and_add(&counter, 1);

    if (num > MAX_CABLES - 1) num = MAX_CABLES - 1;

    if (W->is_paused() && W->b2) {
        this->create_joint(); /* update the length of the cable every frame if we are paused */
    }

    int base = num*(((buf->size/MAX_CABLES))/sizeof(struct vertex));
    struct vertex *v = ((struct vertex*)buf->get_buffer())+base;

    float cable_z = fmaxf(this->p[0]->get_layer(), this->p[1]->get_layer()) * LAYER_DEPTH;

    b2Vec2 p0 = this->p[0]->get_position();
    b2Vec2 p1 = this->p[1]->get_position();

    /* .                 . 0 = 2
     *          .          1 = 3
     *      .       .      2 = 5
     *   .    .   .    .   3 = 9
     *  . .  . . . .  . .  4 = 17,
     *
     *  num points = (2^NUM_SUBDIVIES-1)+1
     *  */
    int num_p = points_per_cable;
    tvec3 pt[num_p];

    pt[0] = (tvec3){p0.x, p0.y, this->p[0]->get_layer() * LAYER_DEPTH+.6f};
    pt[num_p-1] = (tvec3){p1.x, p1.y, this->p[1]->get_layer() * LAYER_DEPTH+.6f};

    float b = (p1-p0).Length();
    float r = this->length;

    float b_bias = 0.9f;

    for (int x=0; x<NUM_SUBDIVISIONS; x++) {
        int num_steps = (int)pow(2.f, (float)x);
        int freq = (num_p/num_steps);

        /* Each subdivision needs:
         *   b, the current length
         *   r, the minimum length
         *
         * Each subdivision's job is to make sure the
         * archlength along the cable is as close to
         * the minimum length as possible.
         *
         * We compute h by constructing a right triangle
         * with b/2 as base and r/2 as the hypothenuse.
         *
         * To allow for more subdivisions, 'h' is biased
         * giving the next subdivision a guessed b and r.
         * By adjusting the bias factor (h_bias), we can
         * decide how "roundly" the cable will bend.
         * As the two end vertices gets closer, a higher
         * h_bias will form the cable into a elliptical shape,
         * while a lower h_bias forms a triangle.
         **/

        float lb = fmaxf(r-b, 0.f);
        float leftover = lb * (1.f-b_bias);
        float b_next = r/2.f-leftover;
        float r_next = r/2.f;

        b-=leftover;

        float h;
        if (r-b < .001f) {
            h = 0.f;
        } else {
            h = sqrtf(powf(r/2.f, 2.f) - powf(b/2.f, 2.f));
        }

        for (int y=freq/2; y<num_p; y+=freq) {
            int _p0 = y - freq/2;
            int _p1 = y + freq/2;

            pt[y].x = (pt[_p0].x + pt[_p1].x)/2.f;
            pt[y].y = (pt[_p0].y + pt[_p1].y)/2.f;
            pt[y].z = 0.75f + cable_z;

            tvec2 nor = (tvec2){-(pt[_p1].y-pt[_p0].y), (pt[_p1].x-pt[_p0].x)};
            float norlen = tvec2_magnitude(&nor);
            float il;
            if (norlen < 0.001f) {
                il = 0.f;
            } else {
                il = 1.f/norlen;
            }
            nor.x *= il;
            nor.y *= il;

            /* raise the midpoint along the normal */
            pt[y].x += nor.x*h;
            pt[y].y += nor.y*h;
            //
        }

        b = b_next;
        r = r_next;
    }

    for (int y=0; y<num_p; y++) {
        int _p0 = y - 1;
        int _p1 = y;
        if (_p0 < 0) {_p0 = 0; _p1=1;};

        tvec3 tangent = (tvec3){pt[_p1].x - pt[_p0].x, pt[_p1].y - pt[_p0].y, 0.f};

        float tlen = tvec3_magnitude(&tangent);
        float ilen;

        if (tlen < 0.000001f) {
            ilen = 0.f;
        } else {
            ilen = 1.f/tlen;
        }

        tangent.x *= ilen;
        tangent.y *= ilen;

        tvec3 normal = (tvec3){-tangent.y, tangent.x, 0.f};

        v[y*QUALITY+0].pos = (tvec3){pt[y].x - _cam_x, pt[y].y - _cam_y, pt[y].z-WIDTH};
        v[y*QUALITY+0].nor = (tvec3){0.f, 0.f, -1.f};

        v[y*QUALITY+1].pos = (tvec3){pt[y].x - normal.x*WIDTH - _cam_x, pt[y].y - normal.y*WIDTH - _cam_y, pt[y].z};
        v[y*QUALITY+1].nor = (tvec3){-normal.x, -normal.y, 0.f};

        v[y*QUALITY+2].pos = (tvec3){pt[y].x - _cam_x, pt[y].y - _cam_y, pt[y].z+WIDTH};
        v[y*QUALITY+2].nor = (tvec3){0.f, 0.f, 1.f};

        v[y*QUALITY+3].pos = (tvec3){pt[y].x + normal.x*WIDTH - _cam_x, pt[y].y + normal.y*WIDTH - _cam_y, pt[y].z};
        v[y*QUALITY+3].nor = (tvec3){normal.x, normal.y, 0.f};
    }

    this->p[0]->update();
    this->p[1]->update();
}

plug::plug(cable *c)
{
    this->set_flag(ENTITY_IS_HIGH_PRIO,         true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_ALLOW_ROTATION,       false);
    this->set_flag(ENTITY_IS_OWNED,             true);

    this->plug_type = PLUG_PLUG;
    this->type = ENTITY_PLUG;
    this->c = c;
    this->s = 0;
    this->plugged_edev = 0;
    this->pending.clear();

    /* TODO: red wire should only have one pluggrej */

    this->update_mesh();

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    switch (c->ctype) {
        case CABLE_RED: this->set_material(&m_cable_red); break;
        case CABLE_BLACK: this->set_material(&m_cable_black); break;
        case CABLE_BLUE: this->set_material(&m_cable_blue); break;
        default: tms_fatalf("Invalid cable type %d", c->ctype);
    }
}

void
plug::update_mesh()
{
    switch (this->c->ctype) {
        case CABLE_BLACK:
            this->set_mesh(mesh_factory::get_mesh(MODEL_PLUG_FEMALE));
            break;
        case CABLE_RED:
            /* no mesh for red cables, they are added to a render buffer */
            break;

        case CABLE_BLUE:
            this->set_mesh(mesh_factory::get_mesh(MODEL_IFPLUG_FEMALE));
            break;
    }
}

void
cable::on_load(bool created, bool has_state)
{
}

bool
cable::connect(plug *p, edevice *e, uint8_t s)
{
    isocket *ss;

    if (s > 127) {
        ss = &e->s_out[s-128];
    } else {
        ss = &e->s_in[s];
    }

    bool ret = this->connect(p, e, ss);

    p->update_mesh();

    return ret;
}

void
cable::disconnect(plug *p)
{
    if (p->plugged_edev) {
        edevice *e = p->plugged_edev;
        int dir = e->get_socket_dir(p->s);

        switch (dir) {
            case CABLE_IN: this->p_out = -1; break;
            case CABLE_OUT: this->p_in = -1; break;
        }

        p->plugged_edev = 0;
        p->s->p = 0;
        p->s = 0;

        if (!this->freeze) {
            this->create_joint();

            p->update_mesh();
        }
    }
}

bool
cable::connect(plug *p, edevice *e, isocket *s)
{
    int index;

    if (s->ctype != this->ctype) {
        tms_warnf("incompatible cable types");
        return false;
    }

    if (p == this->p[0]) {
        index = 0;
    } else if (p == this->p[1]) {
        index = 1;
    } else {
        tms_errorf("cable: this is not my plug! :(");
        return false;
    }

    if (p->s) {
        tms_warnf("already have a socket");
        return false;
    }

    int dir = e->get_socket_dir(s);

    switch (dir) {
        case CABLE_IN: this->p_out = index; break;
        case CABLE_OUT: this->p_in = index; break;
        default: tms_errorf("cable: socket does not belong to edevice (%d)", dir); return false;
    }

    p->s = s;
    p->plugged_edev = e;
    //p->set_layer(p->plugged_edev->get_entity()->get_layer());
    s->p = p;

    p->set_prio(e->get_entity()->get_layer());

    if (this->ready) {
        this->create_joint();
    }

    return true;
}

void
plug::remove_from_world()
{
    if (!this->c->freeze) {
        this->disconnect();
    }

    if (this->body != 0) {
        this->body->GetWorld()->DestroyBody(this->body);
        this->body = 0;
    }
}

void
plug::create_body()
{
    if (W && !this->body) {
        b2BodyDef bd;
        bd.type = this->get_dynamic_type();
        bd.fixedRotation = true;
        bd.angle = 0.f;
        bd.position = this->_pos;

        b2CircleShape c;
        c.m_radius = .25f;
        c.m_p = b2Vec2(0,0);

        b2FixtureDef fd;
        fd.shape = &c;
        fd.density = 0.2f;
        fd.friction = .5f;
        fd.restitution = .2f;
        fd.isSensor = true;

        b2Body *b = W->b2->CreateBody(&bd);
        b->SetAngularDamping(1.f);
        b->SetLinearDamping(1.f);
        (b->CreateFixture(&fd))->SetUserData(this);
        this->body = b;
        this->set_layer(this->prio);
    }
}

float
plug::get_angle()
{
    if (this->is_connected()) {
        return this->plugged_edev->get_entity()->get_angle()+this->s->angle;
    } else if (this->body) {
        b2Vec2 p0 = this->get_other()->get_position() - this->get_position();
        p0 *= 1.f/p0.Length();
        float a = atan2f(p0.y, p0.x);
        return a;
    }

    return -M_PI/2.f;
}

b2Vec2
plug_base::get_position()
{
    if (this->is_connected()) {
        return this->plugged_edev->get_entity()->local_to_world(
                this->s->lpos/* + b2Vec2(cosf(this->s->angle)*.15f, sinf(this->s->angle)*.15f)*/
                , 0);
    } else {
        return entity::get_position();
        //return this->body->GetPosition();
    }
}

void
plug_base::update(void)
{
    b2Vec2 p = this->get_position();
    float a = this->get_angle() + M_PI/2.f;
    float cs,sn;
    tmath_sincos(a, &sn, &cs);

    int l = this->prio;

    if (this->c && this->c->ctype == CABLE_RED) {
        int num = __sync_fetch_and_add(&plug_counter[l], 1);

        if (num > MAX_PLUGS - 1) num = MAX_PLUGS - 1;

        int base = (num*vertices_per_plug);
        struct vertex *v = ((struct vertex*)plug_buf[l]->get_buffer());
        v += base;

        struct tms_mesh *mm = mesh_factory::get_mesh(MODEL_PLUG_SIMPLE);
        struct tms_gbuffer *r_vbuf = mm->vertex_array->gbufs[0].gbuf;

        struct cvert *rv = (struct cvert*)((char*)tms_gbuffer_get_buffer(r_vbuf)+mm->v_start);
        int num_rv = mm->v_count;

        for (int x=0; x<num_rv; x++) {
            v[x].pos = (tvec3){
                rv[x].p.x * cs - rv[x].p.y * sn + p.x - _cam_x,
                rv[x].p.x * sn + rv[x].p.y * cs + p.y - _cam_y,
                rv[x].p.z + this->get_layer() + CABLE_Z
            };
            v[x].nor = (tvec3){
                rv[x].n.x * cs - rv[x].n.y * sn,
                rv[x].n.x * sn + rv[x].n.y * cs,
                rv[x].n.z
            };
        }
    } else {
        this->M[0] = cs;
        this->M[1] = sn;
        this->M[4] = -sn;
        this->M[5] = cs;
        this->M[12] = p.x;
        this->M[13] = p.y;
        this->M[14] = this->prio * LAYER_DEPTH + CABLE_Z;

        tmat3_copy_mat4_sub3x3(this->N, this->M);
    }
}

void
plug_base::add_to_world()
{
    this->pending.clear();

    if (!this->is_connected()) {
        this->create_body();
    } else {
        if (this->get_dir() == CABLE_IN) {
            ((socket_in*)this->s)->reset();
        }
    }
}

/* permanently disconnect this plug */
void
plug::disconnect()
{
    if (this->is_connected()) {
        /* displace the plug a little to visualize the disconnect */
        float cs = cosf(this->s->angle);
        float sn = sinf(this->s->angle);

        this->_pos = this->get_position() + b2Vec2(cs*.5f,sn*.5f);
        this->_angle = this->get_angle();
        this->create_body();
        this->c->disconnect(this);
    }
}

int
plug::connect(edevice *e, isocket *s)
{
    plug_base *other = this->get_other();
    int dz = std::abs(this->get_layer() - other->get_layer());

    if (dz > 1) {
        return 1;
    }

    if (!this->c->connect(this, e, s)) {
        return 2;
    }

    if (this->body) {
        this->body->GetWorld()->DestroyBody(this->body);
        this->body = 0;
    }

    return T_OK;
}

ifdevice*
plug::find_ifdevice()
{
    int limit = 0;

    plug *curr = this;
    ifdevice *i;

    while (curr && limit < 20 && (curr = curr->c->get_other(curr))) {
        if (curr->plugged_edev) {
            if ((i = curr->plugged_edev->get_ifdevice())) {
                return i;
            }

            for (int x=0; x<curr->plugged_edev->num_s_out; x++) {
                if (curr->plugged_edev->s_out[x].ctype == CABLE_BLUE) {
                    curr = static_cast<plug*>(curr->plugged_edev->s_out[x].p);
                    break;
                }
            }
        } else {
            break;
        }

        limit ++;
    }

    return 0;
}

void
plug_base::on_grab(game *g)
{
    this->pending.clear();

    //entity::on_grab(g);
}

void
plug_base::setup()
{
    this->pending.clear();
}

void
plug_base::on_pause()
{
    this->pending.clear();
}

void
plug_base::on_paused_touch(b2Fixture *my, b2Fixture *other)
{
    entity *o = (entity*)other->GetUserData();
    //tms_infof("TOUCH");

    if (o && (G->state.sandbox || o->get_property_entity()->is_moveable())) {
        if (o->flag_active(ENTITY_IS_EDEVICE)) {
            edevice *e = o->get_edevice();

            int type;
            if (!this->c)
                type = CABLE_RED;
            else
                type = this->c->ctype;

            int m1;
            int m2 = e->get_inout_mask(type);

            if (this->c)
                m1 = this->c->get_inout_mask(type);
            else
                m1 = m2; /*XXX uglyhack */

            if (m1 & m2) {
                G->add_highlight(o, false);
                this->pending.insert(e);
            }
        }
    }
}

void
plug_base::set_layer(int z)
{
    plug_base *other = this->get_other();

    if (other) {
        int dz = std::abs(z - other->get_layer());

        if (dz > 1) {
            this->disconnect();
            //return;
        }
    }
    entity::set_layer(z);
}

uint8_t
plug_base::get_socket_index(void)
{
    if (this->is_connected())
        return this->plugged_edev->get_socket_index(this->s);

    return 0;
}

void
plug_base::on_paused_untouch(b2Fixture *my, b2Fixture *other)
{
    //tms_infof("untoch");
    this->pending.erase(static_cast<edevice*>(other->GetUserData()));
}

int
plug::get_dir()
{
    if (this->plugged_edev) {
        return this->plugged_edev->get_socket_dir(this->s);
    } else {
        if (this->c->get_other(this)
                && this->c->get_other(this)->plugged_edev) {
            return 3 - this->c->get_other(this)->plugged_edev->get_socket_dir(this->s);
        }
    }

    return 0;
}

void
plug_base::on_release(game *g)
{
    if (this->pending.size() > 0) {
        tms_infof("num pending: %lu", (unsigned long)this->pending.size());
        /* find the nearest pending */
        edevice *nearest_edev = 0;
        float dist = 3.f;
        b2Vec2 p = this->get_position();

        for (std::set<edevice*>::iterator i = this->pending.begin(); i != this->pending.end(); i++) {
            /* loop through all connections and find the clostest */
            edevice *e = *i;
            for (int x=0; x<e->num_s_in; x++) {
                float d = (p - e->get_entity()->local_to_world(e->s_in[x].lpos, 0)).LengthSquared();

                if (d < dist) {
                    nearest_edev = e;
                    dist = d;
                }
            }
            for (int x=0; x<e->num_s_out; x++) {
                float d = (p - e->get_entity()->local_to_world(e->s_out[x].lpos, 0)).LengthSquared();

                if (d < dist) {
                    nearest_edev = e;
                    dist = d;
                }
            }
        }

        if (nearest_edev && (G->state.sandbox || nearest_edev->get_entity()->get_property_entity()->is_moveable())) {
            G->add_highlight(nearest_edev->get_entity(), false);
            g->open_socket_selector(this, nearest_edev);
        }
    }
}

