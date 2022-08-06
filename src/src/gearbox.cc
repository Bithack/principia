#include "gearbox.hh"
#include "model.hh"
#include "game.hh"
#include "world.hh"
#include "material.hh"

#define MAX_MSLOTS 32

static struct tms_varray *va;
static struct tms_gbuffer *vbuf;
static struct tms_gbuffer *ibuf;
static bool initialized = false;

static gearbox *mslots[MAX_MSLOTS];
static int num_mslots;
static int vp;
static int ip;

/* from */
struct vertex {
    tvec3 pos;
    tvec3 nor;
    tvec2 uv;
};

/* to */
struct vertex2 {
    tvec3 pos;
    tvec3 nor;
} __attribute__ ((packed));

static void init()
{
    num_mslots = 0;

    vbuf = tms_gbuffer_alloc(4096*sizeof(struct vertex2));
    ibuf = tms_gbuffer_alloc(32*4096*sizeof(uint16_t));

    va = tms_varray_alloc(2);
    tms_varray_map_attribute(va, "position", 3, GL_FLOAT, vbuf);
    tms_varray_map_attribute(va, "normal", 3, GL_FLOAT, vbuf);
    initialized = true;
}

static void
addmesh(struct tms_mesh *from, float dx, float dy, int *num_v, int *num_i)
{
    struct vertex *v = (struct vertex*)(from->vertex_array->gbufs[0].gbuf->buf+from->v_start);
    uint16_t *i = (uint16_t*)(from->indices->buf+from->i_start*sizeof(uint16_t));

    struct vertex2 *nv = (struct vertex2*)(vbuf->buf+vp*sizeof(struct vertex2));
    uint16_t *ni = (uint16_t *)(ibuf->buf+ip*sizeof(uint16_t));

    int last_base = from->v_start / sizeof(struct vertex);

    for (int x=0; x<from->i_count; x++)
        ni[x] = i[x] - last_base + vp;

    for (int x=0; x<from->v_count; x++) {
        nv[x].pos = v[x].pos;
        nv[x].nor = v[x].nor;

        nv[x].pos.x += dx;
        nv[x].pos.y += dy;
    }

    vp += from->v_count;
    ip += from->i_count;

    (*num_v) += from->v_count;
    (*num_i) += from->i_count;
}

static void
recreate_meshes()
{
    //tms_infof("recreate meshes- ------------------------------------------------------");
    vp = 0;
    ip = 0;

    int num_v, num_i;

    for (int x=0; x<num_mslots; x++) {
        //tms_infof("fixing slot:%d", x);
        gearbox *gb = mslots[x];

        for (int a=0; a<3; a++) {
            //tms_infof("a:%d", a);
            struct tms_mesh *m = gb->aent[a]->mesh;

            num_v = 0;
            num_i = 0;

            m->v_start = vp*sizeof(struct vertex2);
            m->i_start = ip;

            if (a == 0) {
            //addmesh(a == 0 ? mesh_factory::gb_axle : mesh_factory::gb_axle2,
            addmesh(mesh_factory::get_mesh(MODEL_GB_AXLE),
                    0.f, 0,
                    &num_v, &num_i);
            }

            for (int y=0; y<16; y++) {
                if (gb->properties[a*16+y].v.i8 > 0 && gb->properties[a*16+y].v.i8 <= 5) { /* XXX */
                    addmesh(mesh_factory::get_mesh(MODEL_GB0+gb->properties[a*16+y].v.i8-1),
                            (y - 8) * .04f, 0,
                            &num_v, &num_i);
                }
            }

            m->v_count = num_v;
            m->i_count = num_i;

            /*
            tms_infof("v start count: %d", m->v_start, num_v);
            tms_infof("i start count: %d", m->i_start, num_i);
            */
        }
    }

    tms_gbuffer_upload_partial(vbuf, vp*sizeof(struct vertex2));
    tms_gbuffer_upload_partial(ibuf, ip*sizeof(uint16_t));
}

uint32_t
gearbox::get_num_bodies()
{
    return 2;
}

b2Body*
gearbox::get_body(uint8_t n)
{
    if (n == 0) return this->body;
    else return this->body2;
}

gearbox::gearbox()
{
    if (!initialized) {
        init();
    }

    this->set_flag(ENTITY_DO_STEP,      true);
    this->set_flag(ENTITY_HAS_CONFIG,   true);

    this->set_mesh(mesh_factory::get_mesh(MODEL_GEARBOX));
    this->set_material(&m_metal);

    this->menu_scale = .5f;
    this->menu_pos = b2Vec2(-.8f, 0.f);

    this->update_method = ENTITY_UPDATE_CUSTOM;

    for (int x=0; x<3; x++) {
        this->aent[x] = tms_entity_alloc();
        this->aent[x]->mesh = tms_mesh_alloc(va, ibuf);

        //tms_infof("child %p with mesh %p", this->aent[x], this->aent[x]->mesh);
        this->aent[x]->material = static_cast<struct tms_material*>(&m_iron);
        //tms_entity_set_uniform4f(this->aent[x], "~color", 1.2f, 1.2f, 1.2f, 1.f);
        tms_entity_add_child(this, this->aent[x]);
    }

    this->set_num_properties(16*3 + 1);

    for (int x=0; x<16; x++) {
        this->properties[x].type = P_INT8;
        this->properties[16+x].type = P_INT8;
        this->properties[32+x].type = P_INT8;

        this->properties[x].v.i8 = 0;
        this->properties[16 + x].v.i8 = 0;
        this->properties[32 + x].v.i8 = 0;
    }

    this->properties[48].type = P_INT8;
    this->properties[48].v.i8 = 0;

    this->scaleselect = true;
    this->num_s_in = 2;
    this->s_in[0].lpos = b2Vec2(.794f+.8f, -.15f);
    this->s_in[0].ctype = CABLE_RED;
    this->s_in[1].lpos = b2Vec2(.794f+.8f, .15f);
    this->s_in[1].ctype = CABLE_RED;

    this->c_out.init_owned(0, this);
    //this->c_out.type = CONN_PIVOT;

    this->body2 = 0;

    //this->set_as_rect(1.5f*.5f, 1.25f*.5f);

    if (num_mslots < MAX_MSLOTS) {
        this->mslot = (num_mslots ++);
        mslots[this->mslot] = this;
    } else
        this->mslot = -1;

    this->active_conf = 0;

    this->update_configurations();
}

void
gearbox::find_pairs()
{
    if (this->c_out.pending) {
        b2Vec2 p = this->body->GetWorldPoint(b2Vec2(0.f, 0.f));
        this->q_result = 0;
        this->q_point = p;
        this->q_frame = 0;
        b2AABB aabb;
        aabb.lowerBound.Set(p.x - .05f, p.y - .05f);
        aabb.upperBound.Set(p.x + .05f, p.y + .05f);
        W->b2->QueryAABB(this, aabb);

        if (this->q_result) {
            this->c_out.type = CONN_CUSTOM;
            this->c_out.o = this->q_result;
            this->c_out.p = this->q_point;
            this->c_out.f[1] = this->q_frame;
            G->add_pair(this, this->q_result, &this->c_out);
        }
    }
}

bool
gearbox::ReportFixture(b2Fixture *f)
{
    entity *e = (entity*)f->GetUserData();

    if (!f->IsSensor() && e && f->TestPoint(this->q_point)
        && e->allow_connections()) {
        this->q_frame = (uint8_t)(uintptr_t)f->GetBody()->GetUserData();
        this->q_result = e;
        return false;
    }

    return true;
}

void
gearbox::connection_create_joint(connection *c)
{
    b2World *w = this->body->GetWorld();

    b2RevoluteJointDef rjd;
    rjd.maxMotorTorque = 0.f;
    rjd.motorSpeed = 0.f;
    rjd.enableMotor = true;
    rjd.localAnchorA = b2Vec2(-.0f,0.f);
    if (c->o->type == ENTITY_PLANK) {
        rjd.localAnchorB = c->o->world_to_body(this->local_to_world(b2Vec2(-.0f,0.f), 0), c->f[1]);
        rjd.localAnchorB.y = 0.f; /* XXX */
    } else {
        rjd.localAnchorB = c->o->world_to_body(c->o->get_position(), c->f[1]);
    }
    rjd.bodyA = this->body;
    rjd.bodyB = c->o->get_body(c->f[1]);

    c->j = w->CreateJoint(&rjd);
}

connection *
gearbox::load_connection(connection &conn)
{
    if (conn.o_index == 0) {
        this->c_out = conn;
        return &this->c_out;
    }
    return 0;
}

void
gearbox::set_layer(int n)
{
    entity::set_layer(n);

    if (this->body2) {
        b2Filter filter = world::get_filter_for_layer(n, this->layer_mask);
        this->body2->GetFixtureList()->SetFilterData(filter);
    }
}

bool
gearbox::allow_connection(entity *asker, uint8_t frame, b2Vec2 p)
{
    if (frame == 1) {
        return (asker->type != ENTITY_PLANK);
    }
    return (asker->type == ENTITY_PLANK);
}

void
gearbox::remove_from_world()
{
    if (this->body)
        W->b2->DestroyBody(this->body);
    if (this->body2)
        W->b2->DestroyBody(this->body2);

    this->body = 0;
    this->body2 = 0;
}

void
gearbox::add_to_world()
{
    float sx=.950f*2.f,sy=1.25f;
    //this->create_rect(w, this->get_dynamic_type(), .950f, 1.25f*.5f, this->material);
    {
        b2BodyDef bd;
        bd.type = this->get_dynamic_type();
        bd.position = this->_pos;
        bd.angle = this->_angle;
        b2Body *b = W->b2->CreateBody(&bd);
        this->body = b;

        b2PolygonShape box;
        box.SetAsBox(width, height, b2Vec2(0.8f, 0.f), 0);

        b2FixtureDef fd;

        this->width = width;
        this->height = height;

        fd.shape = &box;
        fd.density = this->get_material()->density;
        fd.friction = this->get_material()->friction;
        fd.restitution = this->get_material()->restitution;
        fd.filter = world::get_filter_for_layer(this->prio);

        (this->body->CreateFixture(&fd))->SetUserData(this);
    }
    /*
    b2MassData m;
    m.mass = 1.f * sx*sy;
    m.center = b2Vec2(-.8f, 0.f);
    m.I = (m.mass/12.f)*(sx*sx+sy*sy) + m.mass*(-.8f*-.8f+0.f*0.f);
    tms_infof("%f", m.I);
    this->body->SetMassData(&m);
    */
    b2MassData m;
    m.mass = this->get_material()->density * sx*sy;
    m.center = b2Vec2(0.f, 0.f);
    m.I = (m.mass/12.f)*(sx*sx+sy*sy) + m.mass*(-.8f*-.8f+0.f*0.f);
    this->body->SetMassData(&m);

    b2BodyDef bd;
    bd.type = this->get_dynamic_type();
    bd.position = this->local_to_world(b2Vec2(1.4f+.8f, 0.f), 0);
    bd.angle = 0;

    b2CircleShape circle;
    circle.m_radius = .25f;

    b2FixtureDef fd;

    fd.shape = &circle;
    fd.density = 1.f;
    fd.friction = FLT_EPSILON;
    fd.restitution = 0.f;
    fd.filter = world::get_filter_for_layer(this->prio);

    this->body2 = W->b2->CreateBody(&bd);
    (body2->CreateFixture(&fd))->SetUserData(this);
    body2->SetUserData((void*)1);

    b2RevoluteJointDef rjd;
    rjd.collideConnected = false;
    rjd.localAnchorA = b2Vec2(1.4f+.8f, 0.f);
    rjd.localAnchorB = b2Vec2(0.f, 0.f);
    rjd.bodyA = this->body;
    rjd.bodyB = this->body2;

    W->b2->CreateJoint(&rjd);

    this->checked_b2conn = false;

    this->active_conf = this->properties[48].v.i8;
    this->gearj = 0;
}

void
gearbox::step()
{
    if (!this->checked_b2conn) {
        this->checked_b2conn = true;
        bool have_b2conn = false;
        connection *c_b2conn = 0;

        /* see if we have anything connected to body2 */
        connection *c = this->conn_ll;
        if (c) {
            int i;
            do {
                i = (c->e == this)?0:1;
                if (c->f[i] == 1) {
                    /* found a body2 conn */
                    have_b2conn = true;
                    this->c_b2conn = c;
                    break;
                }
            } while ((c = c->next[i]));
        }

        if (have_b2conn) {
            if (!this->c_out.pending) {
                this->create_gearjoint();
            }
        }
    }
}

float
gearbox::get_ratio()
{
    //tms_infof("active conf: %d, num:%d", this->active_conf, this->num_configs);
    if (this->active_conf < this->num_configs && this->active_conf >= 0) {
        return this->configs[this->active_conf].ratio - this->configs[this->active_conf].ratio2;
    }

    return 0;
}

void gearbox::update()
{
    entity_fast_update(this);

    float ratio = this->get_ratio();

    tmat4_copy(this->aent[0]->M, this->M);
    if (this->num_configs) {
        tmat4_translate(this->aent[0]->M, .8f+7.25f*.04f - .04f*this->configs[this->active_conf].pos, 0.f, 0.f);
        if (this->body2)
            tmat4_rotate(this->aent[0]->M, this->body2->GetAngle()*(180.f/M_PI), 1.f, 0.f, 0.f);
    }
    tmat3_copy_mat4_sub3x3(this->aent[0]->N, this->aent[0]->M);

    tmat4_copy(this->aent[1]->M, this->M);
    tmat4_translate(this->aent[1]->M, .8f+-.625f/2.f, .275f, 0.f);
    if (this->body2)
        tmat4_rotate(this->aent[1]->M, this->body2->GetAngle()*(180.f/M_PI) * -ratio, 1.f, 0.f, 0.f);
    tmat3_copy_mat4_sub3x3(this->aent[1]->N, this->aent[1]->M);

    tmat4_copy(this->aent[2]->M, this->M);
    tmat4_translate(this->aent[2]->M, .8f+-.625f/2.f, -.275f, 0.f);
    if (this->body2)
        tmat4_rotate(this->aent[2]->M, this->body2->GetAngle()*(180.f/M_PI) * ratio, 1.f, 0.f, 0.f);
    tmat3_copy_mat4_sub3x3(this->aent[2]->N, this->aent[2]->M);
}

void gearbox::on_load(bool created, bool has_state)
{
    for (int x=0; x<48; x++) {
        if (this->properties[x].v.i8 > 5)
            this->properties[x].v.i8 = 5;
    }

    this->update_configurations();
}

gearbox::~gearbox()
{
    /* TODO: free child entities */
    if (this->mslot != -1) {
        if (num_mslots > 1) {
            mslots[num_mslots-1]->mslot = this->mslot;
            mslots[this->mslot] = mslots[num_mslots-1];
            num_mslots --;
        } else {
            num_mslots = 0;
        }
    }
}

void
gearbox::set_position(float x, float y, uint8_t frame)
{
    tms_infof("set position");
    switch (frame) {
        case 0:
            this->body->SetTransform(b2Vec2(x,y), this->body->GetAngle());
            this->body2->SetTransform(this->local_to_world(b2Vec2(1.4f+.8f, 0.f), 0), this->body2->GetAngle());
            break;
    }
}

void
gearbox::update_configurations()
{
    bool block = false;
    bool save = false;

    this->num_configs = 0;
    int num_a;

    gearbox::configuration conf;

    this->min_pos = 0;
    this->max_pos = 15;

    for (int p=0; p<16; p++) {
        int n = p+1;

        num_a = 0;
        conf.pos = p;
        conf.ratio = 0.f;
        conf.ratio2 = 0.f;

        save = false;
        block = false;

        for (int x=0; x<n; x++) {
            int g0 = x;
            int g1 = 15-p + x;

            uint8_t v0 = this->properties[g0].v.i8;
            uint8_t v1 = this->properties[16+g1].v.i8;
            uint8_t v2 = this->properties[32+g1].v.i8;
            uint8_t sum = v0+v1;

            uint8_t sum2 = v0+v2;

            //tms_infof("compare %d %d = %d", g0, g1, sum);

            if (sum > 6) {
                /* colliding */
                block = true;
                save = false;
                break;
            } else if (sum == 6) {
                /* valid configuration */
                save = true;
                if (conf.ratio == 0.f) conf.ratio = 1.f;
                conf.ratio *= (float)v0 / (float)v1;
            } else {
                /* valid but not touching */
            }

            if (sum2 > 6) {
                /* colliding */
                block = true;
                save = false;
                break;
            } else if (sum2 == 6) {
                /* valid configuration */
                save = true;
                if (conf.ratio2 == 0.f) conf.ratio2 = 1.f;
                conf.ratio2 *= (float)v0 / (float)v2;
            } else {
                /* valid but not touching */
            }
        }

        if (block) {
            if (this->num_configs == 0) {
                this->min_pos = p+1;
            } else {
                this->max_pos = p-1;
                break;
            }
        }

        if (save) {
            /*
            if (this->num_configs == 0)
                this->min_pos = p;

            this->max_pos = p;
            */
            this->configs[this->num_configs] = conf;
            this->num_configs ++;
        }
    }

    //tms_infof("found %d configs, min: %d, max: %d", this->num_configs, this->min_pos, this->max_pos);

    recreate_meshes();
}

edevice*
gearbox::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    bool up = (bool)roundf(this->s_in[1].get_value());
    bool down = (bool)roundf(this->s_in[0].get_value());

    int a = this->active_conf;

    this->active_conf += (int)up;
    this->active_conf -= (int)down;

    if (this->active_conf >= this->num_configs)
        this->active_conf = this->num_configs-1;
    else if (this->active_conf < 0)
        this->active_conf = 0;

    if (this->active_conf != a) {
        if (this->gearj) {
            this->body->GetWorld()->DestroyJoint(this->gearj);
            this->create_gearjoint();
            //this->gearj->SetRatio(this->get_ratio());
            //tms_infof("set ratio to: %f", this->get_ratio());
        }
    }

    return 0;

}

void
gearbox::create_gearjoint()
{
    b2GearJointDef gjd;
    gjd.bodyB = this->body2;
    gjd.bodyA = this->c_out.o->get_body(this->c_out.f[1]);
    gjd.joint1 = this->c_out.j;
    gjd.joint2 = c_b2conn->j;
    gjd.ratio = this->get_ratio();
    tms_infof("ratio: %f", gjd.ratio);
    this->gearj = (b2GearJoint*)this->body->GetWorld()->CreateJoint(&gjd);
}

