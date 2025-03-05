#include "plant.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"
#include "world.hh"
#include "font.hh"
#include "textbuffer.hh"
#include "spritebuffer.hh"

#define QUALITY 6
#define MAX_SECTIONS 36
#define MAX_BRANCHES 300

struct plant_predef plant_predefs[NUM_PLANT_PREDEFS] = {
    {
        "Tree",
        {1.8f, 2.0f}, /* angle jitter */
        {.6f, .6f}, /* section width */
        {.0f, .0f}, /* gravity influence */
        {.22f, .25f}, /* sun influence */
        10, /* num sections */
        {.73f, .77f}, /* section length */
        {.122f, .127f}, /* section width multiplier */
        {.75f, .75f}, /* leaf size */
        {.4, .6f, .4}, /* leaf color */
        LEAF_DEFAULT,
    }, {
        "Bush",
        {1.9f, 2.1f}, /* angle jitter */
        {.2f, .2f}, /* section width */
        {.0f, .0f}, /* gravity influence */
        {.22f, .25f}, /* sun influence */
        3, /* num sections */
        {.33f, .33f}, /* section length */
        {.122f, .127f}, /* section width multiplier */
        {.25f, .35f}, /* leaf size */
        {.35f*1.25f, .52f*1.25f, .32f*1.25f}, /* leaf color */
        LEAF_SIMPLE,
    }, {
        "Colorful Bush",
        {1.9f, 2.1f}, /* angle jitter */
        {.2f, .2f}, /* section width */
        {.0f, .0f}, /* gravity influence */
        {.22f, .25f}, /* sun influence */
        4, /* num sections */
        {.53f, .53f}, /* section length */
        {.122f, .127f}, /* section width multiplier */
        {.2f, .2f}, /* leaf size */
        {.8f, .5f, .8f}, /* leaf color */
        LEAF_DEFAULT,
    }, {
        "Big Tree",
        {1.9f, 2.1f}, /* angle jitter */
        {1.f, 1.f}, /* section width */
        {.0f, .0f}, /* gravity influence */
        {.32f, .35f}, /* sun influence */
        16, /* num sections */
        {.73f, .77f}, /* section length */
        {.05f, .075f}, /* section width multiplier */
        {.75f, .75f}, /* leaf size */
        //{.55f, .55f, .3f}, /* leaf color */
        {.35f*1.125, .55f*1.125, .32f*1.125}, /* leaf color */
        LEAF_SIMPLE,
    }, {
        "Sand Tree",
        {2.2f, 2.4f}, /* angle jitter */
        {.6f, .6f}, /* section width */
        {.0f, .0f}, /* gravity influence */
        {.20f, .22f}, /* sun influence */
        10, /* num sections */
        {.73f, .77f}, /* section length */
        {.10f, .12f}, /* section width multiplier */
        {.75f, .75f}, /* leaf size */
        {.35f*2.0f, .45f*2.0f, .3f*2.0f}, /* leaf color */
        LEAF_CIRCLES,
    }, {
        "Rough Tree",
        {2.0f, 2.2f}, /* angle jitter */
        {.6f, .6f}, /* section width */
        {.0f, .0f}, /* gravity influence */
        {.20f, .22f}, /* sun influence */
        11, /* num sections */
        {.53f, .57f}, /* section length */
        {.10f, .12f}, /* section width multiplier */
        {.45f, .55f}, /* leaf size */
        {.35f*.85f, .55f*.85f, .4f*.85f}, /* leaf color */
        LEAF_SIMPLE,
    }, {
        "Climber",
        {2.2f, 2.4f}, /* angle jitter */
        {.15f, .2f}, /* section width */
        {.1f, .15f}, /* gravity influence */
        {.0f, .0f}, /* sun influence */
        11, /* num sections */
        {.33f, .37f}, /* section length */
        {.10f, .12f}, /* section width multiplier */
        {.1f, .15f}, /* leaf size */
        {.2f, .2f, .2f}, /* leaf color */
        LEAF_CIRCLES,
    },
};

enum {
    SERIALIZE_SECTION_BEGIN = 1,
    SERIALIZE_BRANCH_BEGIN  = 2,
    SERIALIZE_BRANCH_END    = 3,
};

static bool initialized = false;
static struct tms_gbuffer *ibuf = 0;
static struct tms_gbuffer *vbuf = 0;
static struct tms_varray  *va = 0;
static int branch_slots[MAX_BRANCHES];
static int top_slot = 0;
static bool buffer_modified = false;

static int                 counter;

struct vertex {
    tvec3 pos;
    tvec3 nor;
    tvec3 tan;
    tvec2 tex;
} __attribute__ ((packed));

static void _init()
{
    vbuf = tms_gbuffer_alloc(MAX_BRANCHES*sizeof(struct vertex)*QUALITY*MAX_SECTIONS);
    ibuf = tms_gbuffer_alloc(MAX_BRANCHES*MAX_SECTIONS * (QUALITY) * 6 * sizeof(short));

    vbuf->usage = GL_STREAM_DRAW;
    ibuf->usage = GL_STATIC_DRAW;

    va = tms_varray_alloc(4);
    tms_varray_map_attribute(va, "position", 3, GL_FLOAT, vbuf);
    tms_varray_map_attribute(va, "normal", 3, GL_FLOAT, vbuf);
    tms_varray_map_attribute(va, "tangent", 3, GL_FLOAT, vbuf);
    tms_varray_map_attribute(va, "texcoord", 2, GL_FLOAT, vbuf);

    short *id = (short*)tms_gbuffer_get_buffer(ibuf);
    int num_i = 0;
    for (int z=0; z<MAX_BRANCHES; z++) {
        for (int y=1; y<MAX_SECTIONS; y++) {
            for (int x=0; x<QUALITY; x++) {
                int c = y*(QUALITY) + x%QUALITY;
                int p = (y-1)*(QUALITY) + x%QUALITY;

                int c1 = y*(QUALITY) + (x+1)%QUALITY;
                int p1 = (y-1)*(QUALITY) + (x+1)%QUALITY;

                id[num_i+1] = c+z*(MAX_SECTIONS-1)*QUALITY;
                id[num_i+0] = p+z*(MAX_SECTIONS-1)*QUALITY;
                id[num_i+2] = c1+z*(MAX_SECTIONS-1)*QUALITY;

                id[num_i+4] = c1+z*(MAX_SECTIONS-1)*QUALITY;
                id[num_i+3] = p+z*(MAX_SECTIONS-1)*QUALITY;
                id[num_i+5] = p1+z*(MAX_SECTIONS-1)*QUALITY;

                num_i+=6;
            }
        }
    }
    tms_gbuffer_upload(ibuf);
    initialized = true;
}

/**
 * properties:
 * max thickness
 * max length
 * randomness
 *
 *
 **/
plant::plant()
    : root_section(), root_branch()
{
    if (!initialized) _init();

    this->num_bodies = 0;

    this->set_mesh(mesh_factory::get_mesh(MODEL_STONE));
    this->set_material(&m_pv_colored);
    this->set_uniform("~color", .5f, .4f, .3f, 1.f);

    this->set_flag(ENTITY_DO_STEP, true);
    this->set_flag(ENTITY_DO_TICK, true);
    this->set_flag(ENTITY_DYNAMIC_UNLOADING, true);
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);
    this->set_flag(ENTITY_CAN_BE_GRABBED, false);
    this->set_flag(ENTITY_IS_ZAPPABLE, true);
    this->set_flag(ENTITY_MUST_BE_DYNAMIC, true);

    this->set_num_properties(14);
    this->properties[0].type = P_STR; /* serialization of constructed structure */
    this->properties[12].type = P_FLT;
    this->properties[12].v.f = 0.f;
    this->properties[13].type = P_INT8;
    this->properties[13].v.i8 = LEAF_DEFAULT;

    this->set_property(0, "");
    this->set_from_predef(PLANT_TREE);

    this->root_branch.first = &this->root_section;
    this->root_branch.last = &this->root_section;
    this->root_branch.leaf = 0;
    this->root_branch.dir = -1;
    this->root_branch.depth = 0;

    this->promote_branch(&this->root_branch, true);

    this->root_section.growth = .25f;
    this->root_section.next = 0;
    this->root_section.branch = &this->root_branch;
    this->root_section.extension = 0;

    this->update_method = ENTITY_UPDATE_CUSTOM;

    this->query_vec = b2Vec2(-.25f, 0.f);
    this->query_pt = b2Vec2(0.f, 0.f);

    this->c.type = CONN_CUSTOM;
}

lvlbuf *lb;

void
plant::serialize_leaf(lvlbuf *lb, plant_leaf *l)
{

}

void
plant::serialize_section(lvlbuf *lb, plant_section *s)
{
    lb->w_s_uint8(SERIALIZE_SECTION_BEGIN);
    lb->w_s_float(s->pos.x);
    lb->w_s_float(s->pos.y);
    lb->w_s_float(s->angle);
    lb->w_s_float(s->growth);
    lb->w_s_float(s->width_growth);
    lb->w_s_float(s->shift);
    lb->w_s_int32(s->stage);

    if (s->extension) {
        this->serialize_branch(lb, s->extension);
    }
}

void
plant::serialize_branch(lvlbuf *lb, plant_branch *br)
{
    lb->w_s_uint8(SERIALIZE_BRANCH_BEGIN);
    lb->w_s_int32(br->dir);
    lb->w_s_uint8(br->dead);
    lb->w_s_uint8((uint8_t)br->derived_body);

    lb->w_s_float(br->b?br->b->GetPosition().x:0.f);
    lb->w_s_float(br->b?br->b->GetPosition().y:0.f);
    lb->w_s_float(br->b?br->b->GetAngle():0.f);

    if (!W->is_paused() && !br->derived_body) {
        lb->w_s_float(br->b?br->b->GetLinearVelocity().x:0.f);
        lb->w_s_float(br->b?br->b->GetLinearVelocity().y:0.f);
        lb->w_s_float(br->b?br->b->GetAngularVelocity():0.f);
        lb->w_s_float(br->b_tmp?br->b_tmp->GetPosition().x:0.f);
        lb->w_s_float(br->b_tmp?br->b_tmp->GetPosition().y:0.f);
        lb->w_s_float(br->b_tmp?br->b_tmp->GetAngle():0.f);

        lb->w_s_float(br->b_tmp?br->b_tmp->GetLinearVelocity().x:0.f);
        lb->w_s_float(br->b_tmp?br->b_tmp->GetLinearVelocity().y:0.f);
        lb->w_s_float(br->b_tmp?br->b_tmp->GetAngularVelocity():0.f);
    } else {
        lb->w_s_float(0.f);
        lb->w_s_float(0.f);
        lb->w_s_float(0.f);
        lb->w_s_float(0.f);
        lb->w_s_float(0.f);
        lb->w_s_float(0.f);
        lb->w_s_float(0.f);
        lb->w_s_float(0.f);
        lb->w_s_float(0.f);
    }

    lb->w_s_float(br->reference_angle);
    lb->w_s_float(br->reference_point.x);
    lb->w_s_float(br->reference_point.y);
    lb->w_s_int32(br->depth);
    lb->w_s_int32(br->dir_toggle);
    lb->w_s_float(br->section_length);
    lb->w_s_float(br->section_width);
    lb->w_s_float(br->section_width_multiplier);
    lb->w_s_int32(br->sections_left);
    lb->w_s_int32(br->sections_done);
    lb->w_s_int32(br->extension_probability);
    lb->w_s_float(br->leaf ? br->leaf->growth : 0.f);

#if 0
    tms_debugf("serialize BRANCH");
    tms_debugf("reference_angle: %f", br->reference_angle);
    tms_debugf("depth: %d", br->depth);
    tms_debugf("section_length: %f", br->section_length);
    tms_debugf("section_width: %f", br->section_width);
    tms_debugf("section_width_multiplier: %f", br->section_width_multiplier);
    tms_debugf("sections_left: %d", br->sections_left);
    tms_debugf("sections_done: %d", br->sections_done);
#endif

    /* write all sections */
    plant_section *s = br->first;
    while (s) {
        this->serialize_section(lb, s);
        s = s->next;
    }

    lb->w_s_uint8(SERIALIZE_BRANCH_END);
}

void
plant::unserialize_section(lvlbuf *lb, plant_section *s)
{
    s->pos.x = lb->r_float();
    s->pos.y = lb->r_float();
    s->angle = lb->r_float();
    s->growth = lb->r_float();
    s->width_growth = lb->r_float();
    s->shift = lb->r_float();
    s->stage = lb->r_int32();
}

void
plant::unserialize_branch(lvlbuf *lb, plant_section *parent_section, plant_branch *br)
{
    br->dir = lb->r_int32();
    br->dead = lb->r_uint8();
    br->derived_body = lb->r_uint8();

    float body_pos_x = lb->r_float();
    float body_pos_y = lb->r_float();
    float body_angle = lb->r_float();

    float b_vel_x = lb->r_float();
    float b_vel_y = lb->r_float();
    float b_vel_a = lb->r_float();

    float b_tmp_pos_x = lb->r_float();
    float b_tmp_pos_y = lb->r_float();
    float b_tmp_angle = lb->r_float();

    float b_tmp_vel_x = lb->r_float();
    float b_tmp_vel_y = lb->r_float();
    float b_tmp_vel_a = lb->r_float();

    br->reference_angle = lb->r_float();
    br->reference_point.x = lb->r_float();
    br->reference_point.y = lb->r_float();
    br->depth = lb->r_int32();
    br->dir_toggle = lb->r_int32();
    br->section_length = lb->r_float();
    br->section_width = lb->r_float();
    br->section_width_multiplier = lb->r_float();
    br->sections_left = lb->r_int32();
    br->sections_done = lb->r_int32();
    br->extension_probability = lb->r_int32();

#if 0
    tms_debugf("unserialize BRANCH");
    tms_debugf("reference_angle: %f", br->reference_angle);
    tms_debugf("depth: %d", br->depth);
    tms_debugf("section_length: %f", br->section_length);
    tms_debugf("section_width: %f", br->section_width);
    tms_debugf("section_width_multiplier: %f", br->section_width_multiplier);
    tms_debugf("sections_left: %d", br->sections_left);
    tms_debugf("sections_done: %d", br->sections_done);
#endif

    if (parent_section) {
        br->parent = parent_section;
        if (br->derived_body) {
            br->b = parent_section->branch->b;
        } else {
            b2BodyDef bd;
            bd.position.Set(body_pos_x, body_pos_y);
            bd.angle = body_angle;
            bd.type = b2_dynamicBody;
            br->b = this->create_body(&bd);

            if (!W->is_paused()) {
                br->b->SetLinearVelocity(b2Vec2(b_vel_x, b_vel_y));
                br->b->SetAngularVelocity(b_vel_a);
            }
        }
    } else {
        /* root branch */
        br->b->SetTransform(b2Vec2(body_pos_x, body_pos_y), body_angle);

        if (!W->is_paused()) {
            br->b->SetLinearVelocity(b2Vec2(b_vel_x, b_vel_y));
            br->b->SetAngularVelocity(b_vel_a);
        }
    }

    float leaf_size = lb->r_float();

    plant_section *last = 0;

    uint8_t sig;
    while (!lb->eof() && (sig = lb->r_uint8()) != SERIALIZE_BRANCH_END) {
        switch (sig) {
            case SERIALIZE_SECTION_BEGIN:
                {
                    plant_section *s = new plant_section();
                    s->branch = br;

                    if (br->first == 0) {
                        br->first = s;
                    }

                    this->unserialize_section(lb, s);

                    if (last) {
                        last->next = s;
                    }
                    last = s;
                }
                break;

            case SERIALIZE_BRANCH_BEGIN:
                {
                    if (!last) {
                        tms_warnf("wtf? a branch extending from nothing");
                        continue;
                    }

                    plant_branch *n = new plant_branch();
                    last->extension = n;
                    this->unserialize_branch(lb, last, n);

                }
                break;

            default:
                {
                    tms_warnf("Read unknown section!!!!!!!!!!! %d", sig);
                }
                break;
        }
    }

    if (!last) {
        tms_warnf("We unserialized a plant which had no sections.");
        return;
    }

    br->last = last;
    if (br->last->growth < 1.f) {
        this->begin_section_fixture(last, b2Vec2(b_tmp_vel_x, b_tmp_vel_y), b_tmp_vel_a);
    }

    last = br->first;

    while (last) {
        this->update_section_fixture(last, 0);
        last = last->next;
    }

    if (!br->derived_body && br != &this->root_branch) {
        if (br->parent && br->parent->branch->b) {
            br->create_joint(br->parent->branch->b, br->reference_point, false);
        }
    }

    if (leaf_size > .01f) {
        br->leaf = this->create_leaf(br);
        br->leaf->growth = leaf_size;
        this->update_leaf(br);
    }
}

void
plant::unserialize(lvlbuf *lb)
{
    uint8_t a = lb->r_uint8();

    this->root_branch.first = 0;
    this->root_branch.last = 0;

    if (a == SERIALIZE_BRANCH_BEGIN) {
        this->unserialize_branch(lb, 0, &this->root_branch);
    } else {
        this->root_branch.first = &this->root_section;
        this->root_branch.last = &this->root_section;
    }
}

void
plant::on_load(bool created, bool has_state)
{
    if (created) {
        this->properties[12].v.f = 10.f + (rand()%100) / 100.f * 20.f;
    }
}

void
plant::pre_write()
{
    /* XXX use static buffer */
    lvlbuf lb;
    lb.cap = 20480;
    lb.size = 0;
    lb.buf = (unsigned char*)malloc(20480);

    this->serialize_branch(&lb, &this->root_branch);
    this->set_property(0, (const char*)lb.buf, lb.size);

    free(lb.buf);
}

plant_branch::plant_branch()
{
    this->needs_update = true;
    this->dead = false;
    this->first = 0;
    this->last = 0;
    this->leaf = 0;
    this->dir = 0;
    this->b = 0;
    this->j = 0;
    this->b_tmp = 0;
    this->f_tmp = 0;
    this->j_tmp = 0;
    this->parent = 0;
    this->derived_body = false;
    this->depth = 0;
    this->dir_toggle = -1;
    this->reference_angle = 0;
    this->reference_point = b2Vec2(0.f, 0.f);
    this->slot = -1;
    this->sections_left = 0;
    this->sections_done = 0;
    this->extension_probability = 0;
    tms_entity_init(this);
}

void
plant::update(void)
{
    tmat4_load_identity(this->M);
    entity_fast_update(this);

    if (!this->c.pending && this->root_branch.first) {
        float scale = this->root_branch.first->get_width()*8.f;
        scale += 1.5f;
        tmat4_scale(this->M, scale, scale, scale);
    }

    this->update_meshes(&this->root_branch);
}

void
plant::clear_branch_slots(plant_branch *br)
{
    if (br->slot != -1) {
        branch_slots[br->slot] = 0;
        br->slot = -1;
    }

    plant_section *s = br->first;
    while (s) {
        if (s->extension) {
            this->clear_branch_slots(s->extension);
        }
        s = s->next;
    }
}

void
plant::remove_from_world()
{
    for (int x=0; x<this->num_bodies; x++) {
        if (this->bodies[x]) {
            W->b2->DestroyBody(this->bodies[x]);
            this->bodies[x] = 0;
        }
    }

    /* TODO: make sure all slots are lst */

    W->b2->DestroyBody(this->body);
    this->body = 0;

    this->clear_branch_slots(&this->root_branch);
}

plant::~plant()
{
    this->clear_branch_slots(&this->root_branch);
    /* TODO delete all branches, sections, leafs */
}

void
plant::add_to_world()
{
    if (this->body) {
        return;
    }

    this->create_circle(b2_dynamicBody, .125f, this->material);

    this->body->SetAngularDamping(3.f);
    this->root_branch.b = this->body;

    if (W->is_paused()) {
        this->_angle = M_PI/2.f;
        this->body->SetTransform(this->body->GetPosition(), M_PI/2.f);
        this->body->SetFixedRotation(true);
    }

    lvlbuf lb;
    lb.rp = 0;
    lb.cap = 0;
    lb.size = this->properties[0].v.s.len;
    lb.buf = (uint8_t*)this->properties[0].v.s.buf;

    this->unserialize(&lb);

    //this->begin_section_fixture(this->root_branch.first);
}

plant_section::plant_section(plant_section *s)
    : ud2_info(UD2_PLANT_SECTION)
{
    this->clear();
    this->branch = s->branch;
    this->pos = s->get_end_point();
    //this->angle = s->angle * .5f;
    //this->angle = s->angle;
    this->angle = s->angle;
}

void
plant::update_leaf(plant_branch *br)
{
    if (!br->last) {
        return;
    }

    if (br->leaf || br->sections_left == 0 || (br->depth != 0)) {
        if (br->leaf == 0) {
            br->leaf = this->create_leaf(br);
        }

        b2CircleShape _c;
        b2CircleShape *sh;

        plant_leaf *l = br->leaf;

        if (l->f)
            sh = (b2CircleShape*)(l->f->GetShape());
        else {
            sh = &_c;
        }

        sh->m_radius = tclampf(br->leaf->growth, 0.1, 1.f)*.125f;
        sh->m_p = br->last->get_end_point();

        if (!l->f) {
            b2FixtureDef fd;
            fd.shape = sh;
            fd.restitution = .0f;
            fd.isSensor = true;
            fd.friction = 1.f;
            fd.density = .01f;
            fd.filter = world::get_filter_for_layer(this->get_layer(), 8);
            (l->f = br->b->CreateFixture(&fd))->SetUserData(this);
        }

        br->b->ResetMassData();
    }
}

plant_leaf::plant_leaf(int leaf_type) : entity()
{
    f = 0;
    growth = 0.1f;
    switch(leaf_type) {
        default:
        case LEAF_DEFAULT:
            this->set_mesh(mesh_factory::get_mesh(MODEL_LEAVES0));
            this->set_material(&m_leaves);
            break;
        case LEAF_SIMPLE:
            this->set_mesh(mesh_factory::get_mesh(MODEL_LEAVES2));
            this->set_material(&m_leaves);
            break;
        case LEAF_CIRCLES:
            this->set_mesh(mesh_factory::get_mesh(MODEL_LEAVES3));
            this->set_material(&m_leaves);
            break;
        case LEAF_CYLINDER:
            this->set_mesh(mesh_factory::get_mesh(MODEL_CYLINDER1));
            this->set_material(&m_pv_colored);
            break;
    }
}

plant_leaf *
plant::create_leaf(plant_branch *br)
{
    plant_leaf *l = new plant_leaf(this->properties[13].v.i8);
    l->set_uniform("~color",
            this->properties[9].v.f,
            this->properties[10].v.f,
            this->properties[11].v.f, 1.f);
    l->prio = this->get_layer();
    l->growth = 0.1f;

    tms_entity_add_child(this, l);
    if (this->scene) tms_scene_add_entity(this->scene, l);

    return l;
}

void
plant::init_section(plant_section *n)
{
    n->angle += (((rand()%10000)/10000.f) - .5f) * this->get_angle_jitter(n->branch);

    b2Vec2 g = W->get_gravity();
    float g_strength = g.Normalize();
    float g_angle = atan2f(g.y, g.x);

    float s_angle = M_PI/2.f-n->branch->b->GetAngle();

    n->angle += tmath_adist(g_angle, n->angle) * this->get_gravity_influence(n->branch) * g_strength/20.f;
    n->angle -= tmath_adist(s_angle, n->angle) * this->get_sun_influence(n->branch);
}

plant_section *
plant::create_section(plant_section *s)
{
    if (!s->branch->dead && s->branch->sections_left > 0) {
        plant_section *n = new plant_section(s);

        this->init_section(n);

        s->branch->last = n;
        s->branch->sections_left --;
        return n;
    }

    return 0;
}

void
plant_branch::create_joint(b2Body *other, b2Vec2 point, bool collide)
{
    if (!this->j && !this->derived_body) {
#if 0
        b2WeldJointDef rjd;
        rjd.localAnchorA = point;
        rjd.localAnchorB = b2Vec2(0.f, 0.f);
        rjd.bodyA = other;
        rjd.bodyB = this->b;
        rjd.frequencyHz = 2.f;
        rjd.dampingRatio = .9f;
        rjd.referenceAngle = this->b->GetAngle() - other->GetAngle();
        rjd.collideConnected = collide;

        this->j = W->b2->CreateJoint(&rjd);
#else
        b2RevoluteJointDef rjd;
        rjd.localAnchorA = point;
        rjd.localAnchorB = b2Vec2(0.f, 0.f);
        rjd.bodyA = other;
        rjd.bodyB = this->b;
        //rjd.referenceAngle = this->b->GetAngle() - other->GetAngle();
        rjd.collideConnected = false;//collide;
        rjd.enableMotor = true;
        rjd.maxMotorTorque = 0.f;
        rjd.motorSpeed = 0.f;

        this->reference_angle = this->b->GetAngle() - other->GetAngle();
        this->reference_point = point;

        this->j = (b2RevoluteJoint*)W->b2->CreateJoint(&rjd);
#endif
    }
}

/* promote a branch to have its own body */
void
plant::promote_branch(plant_branch *n, bool mesh_only)
{
    plant_section *parent_section = n->parent;
    b2Body *parent_body = n->parent ? n->parent->branch->b : 0;

    if (mesh_only || (parent_body && n->derived_body && n->depth < 3)) {
        //tms_debugf("promoting branch");

        n->derived_body = false;

        if (!mesh_only) {
            b2BodyDef bd;
            bd.position = parent_body->GetWorldPoint(b2Vec2(parent_section->get_mid_point()));
            bd.angle = parent_body->GetAngle() + n->first->angle;
            bd.type = b2_dynamicBody;

            n->first->angle = 0;
            n->first->pos = b2Vec2(0.f, 0.f);

            n->b = this->create_body(&bd);
            n->create_joint(parent_body, parent_section->get_mid_point(), false);
        }
    }
}

float
plant::get_section_width(plant_branch *br)
{
    if (W->level.version >= LEVEL_VERSION_1_5_1) {
        return tclampf(this->properties[2].v.f/powf((br->depth+1), 0.7f)/16.f * std::max(this->get_num_sections(), 8), 0.05f, 1.f);
    } else {
        return tclampf(this->properties[2].v.f/(br->depth+1)/16.f * std::max(this->get_num_sections(), 8), 0.05f, 1.f);
    }
}

void
plant::init_branch(plant_branch *n, plant_section *parent_section)
{
    n->section_length = this->get_section_length(n);
    n->section_width = this->get_section_width(n);
    n->section_width_multiplier = this->get_section_width_multiplier(n);
    n->extension_probability = n->depth;
    n->parent = parent_section;

    if (parent_section) {
        b2Body *parent_body = parent_section->branch->b;
        n->b = parent_body;
        n->derived_body = true;
    }
}

/**
 * create a new branch that extends from s in br
 **/
plant_branch *
plant::create_branch(plant_section *s)
{
    plant_branch *br = s->branch;
    plant_branch *n = new plant_branch();

    n->depth = br->depth+1;
    n->dir = br->dir_toggle;
    n->sections_left = std::min(br->sections_left, 16 / (n->depth*2));//16 / (n->depth*2);
    n->sections_done = 0;
    if (br->dir_toggle == 1) br->dir_toggle = -1; else br->dir_toggle = 1;

    this->init_branch(n, s);

    n->first = new plant_section();
    n->first->angle = (s->angle) + n->dir*M_PI/2.f;
    n->first->pos = s->get_mid_point();
    n->first->branch = n;
    n->last = n->first;

    this->begin_section_fixture(n->first, br->b->GetLinearVelocity(), br->b->GetAngularVelocity());
    this->update_section_fixture(n->first, 0.f);

    return n;
}

b2Vec2
plant_section::get_mid_point()
{
    float s, c;
    tmath_sincos(this->angle, &s, &c);

    return this->pos + this->branch->section_length*tclampf(this->growth, 0.f, 1.f)*.5f*b2Vec2(c,s);
}

float
plant_section::get_width()
{
    return tclampf(this->width_growth * this->branch->section_width_multiplier, 0.1f, 1.f)*.5f*this->branch->section_width;
}

float
plant_section::get_shift()
{
    return tclampf(this->shift * this->branch->section_width_multiplier, 0.1f, 1.f)*.5f*this->branch->section_width;
}

/**
 * Get the displacement due to external obstacles
 **/
float
plant_section::get_angle_displacement()
{
    /* if this is the last section of the branch, it will be on a temp body
     * that can be rotated we need to adjust the axis */
    if (this == this->branch->last && this->branch->b_tmp && this->branch->b) {
        return this->branch->b_tmp->GetAngle() - this->branch->b->GetAngle();
    }

    return 0.f;
}

b2Vec2
plant_section::get_vector()
{
    float s, c;
    float a = this->angle;

    a += this->get_angle_displacement();

    tmath_sincos(a, &s, &c);

    return b2Vec2(c,s);
}

b2Vec2
plant_section::get_end_point()
{
    return this->pos + this->branch->section_length*tclampf(this->growth, 0.f, 1.f)*this->get_vector();
}

void
plant::begin_section_fixture(plant_section *s, b2Vec2 b_tmp_vel, float b_tmp_avel)
{
    b2Body *b = s->branch->b_tmp;

    if (!b) {
        b2BodyDef bd;
        bd.position = s->branch->b->GetPosition();
        bd.angle = s->branch->b->GetAngle();
        bd.type = b2_dynamicBody;

        b = (s->branch->b_tmp = this->create_body(&bd));

        if (!W->is_paused()) {
            b->SetLinearVelocity(b_tmp_vel);
            b->SetAngularVelocity(b_tmp_avel);
        }
    } else {
        b->SetTransform(s->branch->b->GetPosition(), s->branch->b->GetAngle());
    }

    if (!s->branch->f_tmp) {
        b2CircleShape sh;
        sh.m_radius = .125f;
        sh.m_p = s->get_end_point();

        b2FixtureDef fd;
        fd.shape = &sh;
        fd.restitution = .0f;
        fd.friction = FLT_EPSILON;
        fd.density = .5f;
        fd.filter = world::get_filter_for_layer(this->get_layer(), 6);
        s->branch->f_tmp = b->CreateFixture(&fd);
    }

    if (s->branch->j_tmp) {
        W->b2->DestroyJoint(s->branch->j_tmp);
        s->branch->j_tmp = 0;
    }

    /* revolute joint with motor, used for the temporary body when
     * growing */
    b2RevoluteJointDef rjd;
    rjd.localAnchorA = s->pos;
    rjd.localAnchorB = s->pos;
    rjd.bodyA = s->branch->b;
    rjd.bodyB = s->branch->b_tmp;
    rjd.enableMotor = true;
    rjd.maxMotorTorque = this->get_grow_strength(s->branch);
    rjd.motorSpeed = 0.f;
    rjd.collideConnected = false;
#if 0
    b2WeldJointDef rjd;
    rjd.localAnchorA = s->pos;
    rjd.localAnchorB = s->pos;
    rjd.bodyA = s->branch->b;
    rjd.bodyB = s->branch->b_tmp;
    rjd.referenceAngle = 0.f;
    rjd.collideConnected = false;
#endif

    s->branch->j_tmp = (b2RevoluteJoint*)b->GetWorld()->CreateJoint(&rjd);
}

void
plant::end_section_fixture(plant_section *s)
{
    if (s->f && s->f->GetBody() == s->branch->b_tmp) {
        /* we're no longer the last section, move the growing fixture to the real body,
         * and save the physically adjusted angle */
        s->angle = s->angle + (s->branch->b_tmp->GetAngle() - s->branch->b->GetAngle());
        if (s->branch->j_tmp) {
            s->f->GetBody()->GetWorld()->DestroyJoint(s->branch->j_tmp);
            s->branch->j_tmp = 0;
        }
        this->destroy_body(s->f->GetBody());
        s->f = 0;
        s->branch->b_tmp = 0;
        s->branch->f_tmp = 0;

        s->branch->needs_update = true;
        tms_debugf("needs update from end_section_fixture");

        this->promote_branch(s->branch);
    }

    s->branch->sections_done ++;
}

void
plant::update_section_fixture(plant_section *s, float time)
{
    b2PolygonShape _box;
    b2PolygonShape *sh;

    plant_branch *br = s->branch;

    if (s->f) {
        sh = (b2PolygonShape*)(s->f->GetShape());

        /* loop through contacts, if we're in contact with anything, do NOT grow width */
        b2ContactEdge *e = s->f->GetBody()->GetContactList();
        bool contact_sides[2] = {false,false};

        b2Vec2 start, end;
        start = s->f->GetBody()->GetWorldPoint(s->get_start_point());
        end = s->f->GetBody()->GetWorldPoint(s->get_end_point());

        for (; e; e = e->next) {
            b2Contact *c = e->contact;

            if ((c->GetFixtureA() == s->f || c->GetFixtureB() == s->f) && c->IsTouching()) {
                b2WorldManifold wm;
                wm.points[0]=b2Vec2(0.f,0.f);
                c->GetWorldManifold(&wm);

                entity *o;
                if ((c->GetFixtureA() == s->f)) {
                    o = static_cast<entity*>(c->GetFixtureB()->GetUserData());
                } else {
                    o = static_cast<entity*>(c->GetFixtureA()->GetUserData());
                }

                if (o && (o->g_id == O_CHUNK || o->g_id == O_TPIXEL)) {
                    if (s == this->root_branch.first) {
                        continue;
                    }
                }

                tvec2 pt = (tvec2){wm.points[0].x, wm.points[0].y};
                tvec2 ss = (tvec2){start.x, start.y};
                tvec2 ee = (tvec2){end.x, end.y};
                float side = (tintersect_point_line_cmp(&ss, &ee, &pt));
                if (side < 0) contact_sides[0] = true;
                else contact_sides[1] = true;

                if (contact_sides[1] && contact_sides[0])
                    break;
            }
        }

        if (contact_sides[0] && contact_sides[1]) {
            if (s == this->root_branch.first->next) {
                //tms_infof("both sides colliding");
            }
            /* do nothing */
        } else if (contact_sides[0]) {
            if (s == this->root_branch.first->next) {
                //tms_infof("side 0 colliding");
            }
            s->width_growth += time;
            s->shift -= time;
        } else if (contact_sides[1]) {
            if (s == this->root_branch.first->next) {
                //tms_infof("side 1 colliding");
            }
            s->width_growth += time;
            s->shift += time;
        } else
            s->width_growth += time;
    } else {
        s->width_growth += time;
        sh = &_box;
    }

    b2Vec2 tangent;
    tmath_sincos(s->angle, &tangent.y, &tangent.x);
    b2Vec2 normal = b2Vec2(-tangent.y, tangent.x);

    float length = tclampf(s->growth, 0.1f, 1.f)*br->section_length;
    float w1 = tclampf((s->width_growth+1.f)* br->section_width_multiplier, 0.1f, 1.f)*.5f*br->section_width;
    float w2 = s->get_width();

    /* if this is the last section, we give it some extra width and length to make sure we're not growing too tight */
    if (s->f && s->f->GetBody() == s->branch->b_tmp) {
        length += 0.05f;
        w1 += .05f;
        w2 += .05f;
    }

    b2Vec2 shift = -s->get_shift()*normal;

    b2Vec2 verts[4] = {
        s->pos + length*tangent - w2*normal + shift,
        s->pos + length*tangent + w2*normal + shift,
        s->pos + w1*normal + shift,
        s->pos - w1*normal + shift
    };

    sh->Set(verts, 4);

    if (!s->f) {
        b2FixtureDef fd;
        fd.shape = sh;
        fd.restitution = .1f;
        fd.friction = .7f;
        fd.density = .8f;
        fd.filter = world::get_filter_for_layer(this->get_layer(), 6);

        b2Body *b;

        if (s == s->branch->last && s->branch->b_tmp) {
            b = s->branch->b_tmp;
        } else {
            b = s->branch->b;
        }

        (s->f = b->CreateFixture(&fd))->SetUserData(this);
        s->f->SetUserData2(s);
    }

    if (s == s->branch->last && s->branch->f_tmp) {
        ((b2CircleShape*)s->branch->f_tmp->GetShape())->m_p = (s->pos+length*tangent);
    }

    if (fabsf(s->update_width - s->width_growth) > .05f
        || fabsf(s->update_growth - s->growth) > .05f) {
        s->branch->needs_update = true;
        /*tms_debugf("needs update because width/growth changed too much! %f->%f, %f->%f",
                s->update_width, s->width_growth,
                s->update_growth, s->growth);*/
    }

    s->f->Refilter();
    s->f->GetBody()->ResetMassData(); /* XXX */
}

static void
count_sections(plant_section *s, int *count)
{
    while (s) {
        (*count) ++;
        if (s->extension && s->extension->first) {
            count_sections(s->extension->first, count);
        }
        s = s->next;
    }
}

#define TEXT_SCALE .005

void
plant::render_damage(plant_branch *br)
{
    plant_section *s = br->first;

    while (s) {
        if (s->extension && s->extension->first) {
            this->render_damage(s->extension);
        }

        if (s->damage_timer > 0.f) {
            s->damage_timer -= _tms.dt*1.f;
            p_font *f = font::large;
            int val = (int)(s->hp * 100.f);
            if (val > 99) val = 99;
            if (val < 0) val = 0;
            char val_str[3];
            sprintf(val_str, "%d", val);
            float width = 0.f;
            int slen = strlen(val_str);
            struct glyph *g;
            float max_height = 0.f;

            for (int i=0; i<slen; ++i) {
                if ((g = f->get_glyph(val_str[i]))) {
                    width += g->ax;
                    if (g->bh > max_height) {
                        max_height = g->bh;
                    }
                }
            }

            b2Vec2 p = s->branch->b->GetWorldPoint(s->get_mid_point());

            spritebuffer::add2(p.x, p.y+max_height*TEXT_SCALE/2.f, this->get_layer()*LAYER_DEPTH + LAYER_DEPTH/2.f,
                             1.f, 1.f, 1.f, .5f*s->damage_timer, .5f, .5f, 1);

            p.x -= width/2.f*TEXT_SCALE;

            for (int i=0; i<slen; ++i) {
                if ((g = f->get_glyph(val_str[i]))) {
                    float x2 = p.x + (g->bl + g->bw/2.f) * TEXT_SCALE;
                    float y2 = p.y + (g->bt - g->bh/2.f) * TEXT_SCALE;

                    p.x += g->ax * TEXT_SCALE;
                    p.y += g->ay * TEXT_SCALE;
                    textbuffer::add_char(g,
                            x2,
                            y2,
                            this->get_layer()*LAYER_DEPTH + LAYER_DEPTH/2.f+.01,
                            0.0f, 0.0f, 0.0f, s->damage_timer,
                            g->bw*TEXT_SCALE,
                            g->bh*TEXT_SCALE
                            );
                }
            }
        }
        s = s->next;
    }
}

void
plant::update_effects()
{
    this->render_damage(&this->root_branch);
}

void
plant::damage_section(plant_section *s, float damage, damage_type dmg_type)
{
    if (!s) {
        return; /* probably was the seed fixture */
    }

    switch (dmg_type) {
        case DAMAGE_TYPE_FORCE:
        case DAMAGE_TYPE_ELECTRICITY:
        case DAMAGE_TYPE_OTHER:
            break;

        case DAMAGE_TYPE_SHARP:
            damage *= .01f;
            break;

        case DAMAGE_TYPE_HEAVY_SHARP:
            damage *= 2.f;
            break;

        default:
            /* ignore all other damage types */
            return;
    }

    int num_sections = 0;
    count_sections(s, &num_sections);

    damage /= num_sections;
    tms_debugf("section hp set to %f->%f (num sections: %d)", s->hp, s->hp-damage, num_sections);
    s->hp -= damage;
    s->damage_timer = 1.f;
}

void
plant::break_branch(plant_branch *br, plant_section *s, bool create_resources)
{
    bool remove = (s == br->first);
    br->dead = true;
    br->needs_update = true;

    plant_section *previous = 0;
    if (!remove) {
        plant_section *tmp = br->first;
        while (tmp) {
            if (tmp->next == s) {
                previous = tmp;
                break;
            }
            tmp = tmp->next;
        }
    }

    br->last = previous;

    while (s) {
        if (s->extension) {
            this->break_branch(s->extension, s->extension->first, create_resources);
        }

        int count = 1 + floor(s->get_width()*s->branch->section_length*s->growth*2);
        for (int x=0; x<count; x++) {
            resource *r = static_cast<resource*>(of::create(O_RESOURCE));
            r->set_resource_type(RESOURCE_WOOD);
            r->set_position(br->b->GetWorldPoint(s->get_mid_point()));
            r->set_layer(this->get_layer());
            r->set_amount(1);
            G->emit(r, 0, br->b->GetLinearVelocityFromLocalPoint(s->get_mid_point()));
        }

        if (s->f) {
            s->f->GetBody()->DestroyFixture(s->f);
            s->f = 0;
        }

        if (previous) {
            previous->next = 0;
        }

        plant_section *next = s->next;

        if (s != &this->root_section) {
            delete s;
        }

        previous = 0;
        s = next;
    }

    if (br->leaf) {
        if (br->leaf->scene) {
            tms_scene_remove_entity(br->leaf->scene, br->leaf);
        }
        if (br->leaf->parent) {
            tms_entity_remove_child(br->leaf->parent, br->leaf);
        }

        if (br->leaf->f) {
            br->leaf->f->GetBody()->DestroyFixture(br->leaf->f);
            br->leaf->f = 0;
        }

        delete br->leaf;
        br->leaf = 0;
    }

    if (br->b_tmp) {
        this->destroy_body(br->b_tmp);
        br->b_tmp = 0;
        br->f_tmp = 0;
        br->j_tmp = 0;
    }

    if (remove) {
        if (br->parent) {
            br->parent->extension = 0;
        }

        tms_entity_remove_child(this, br);
        if (br->scene) {
            tms_scene_remove_entity(br->scene, br);
        }

        if (br == &this->root_branch) {
            br->first = 0;
        } else {
            if (br->b && !br->derived_body) {
                this->destroy_body(br->b);
                br->b = 0;
                br->j = 0;
            }
            delete br;
        }
    }

    // No branches left on this plant, absorb!
    if (!this->root_branch.first) {
        G->lock();
        this->disconnect_all();
        G->absorb(this);
        G->unlock();
    }
}

int
plant::update_mesh(plant_section *s, struct vertex *v, int y, bool search_only)
{
    plant_branch *extensions[64]; /* XXX */
    int num_extensions = 0;

    plant_branch *br = s->branch;

    if (!search_only) {
        y = this->mesh_add_pre_branch_sections(br, v, y);
    }

    plant_section *prev = 0;

    b2Vec2 prev_axis = br->first->get_vector();
    float prev_width = br->first->get_width();

    while (s) {
        if (s->extension) {
            if (s->extension->derived_body) {
                extensions[num_extensions] = s->extension;
                num_extensions ++;
            } else {
                this->update_meshes(s->extension);
            }
        }

        if (!search_only) {
            b2Vec2 bp = s->get_end_point();
            b2Vec2 axis;

            /* TODO fix sharp edges */
            if (/*prev && */s->next) {
                axis = .5*s->next->get_vector() /*+ .25*prev->get_vector()*/ + .5*s->get_vector();
            } else {
                axis = s->get_vector();
            }

            s->update_width = s->width_growth;
            s->update_growth = s->growth;

            float width = s->get_width();

            y = this->mesh_add_section(v, y, bp, axis, width);
            prev_axis = axis;
            prev_width = width;
        }
        prev = s;
        s = s->next;
    }

    if (!search_only) {
        y = this->mesh_add_post_branch_sections(br, v, y);

        for (int x=0; x<num_extensions; x++) {
            y = this->update_mesh(extensions[x]->first, v, y, search_only);
        }
    }

    if (br->leaf) {
        tmat4_load_identity(br->leaf->M);
        b2Vec2 pt = br->b->GetWorldPoint(br->last->get_end_point());
        float scale = tclampf(br->leaf->growth, 0.1, 1.f) * this->get_leaf_size()/**(1.f+br->depth)*.25f*/;
        tmat4_translate(br->leaf->M, pt.x, pt.y, this->get_layer()*LAYER_DEPTH);
        tmat4_rotate(br->leaf->M, br->b->GetAngle() * (180.f/M_PI), 0, 0, -1);
        tmat3_copy_mat4_sub3x3(br->leaf->N, br->leaf->M);
        tmat4_scale(br->leaf->M, scale*.75, scale*.75, scale*.75);//this->get_leaf_size());
    }

    return y;
}

int
plant::mesh_add_section(struct vertex *v, int y, b2Vec2 bp, b2Vec2 axis, float width)
{
    static const float step = (M_PI*2.f) / (float)QUALITY;

    for (int x=0; x<QUALITY; x++) {
        float mat[16];
        float a = step * (float)x;
        tmat4_load_identity(mat);
        tmat4_rotate(mat, a * (180.f / M_PI), axis.x, axis.y, 0.f);

        tvec4 pt = (tvec4){0.f, 0.f, -width, 1.f};
        tvec4_mul_mat4(&pt, mat);

        v[y*QUALITY+x].pos = (tvec3){pt.x + bp.x/* - _cam_x*/, bp.y + pt.y/* - _cam_y*/, pt.z};

        tvec3_normalize((tvec3*)&pt);
        v[y*QUALITY+x].nor = (tvec3){pt.x, pt.y, pt.z};
        v[y*QUALITY+x].tan = (tvec3){-axis.y, axis.x, 0.f};
        v[y*QUALITY+x].tex = (tvec2){
            //.5f + ((float)(x-QUALITY/2.f)/QUALITY/2.f)*width*2.f,
            x/(float)QUALITY,
            y/8.f};
    }

    return y+1;
}

/**
 * we always start and end a branch with a 0-radius section, this
 * closes the branch's edges and also allows us to render
 * multiple branches in the same mesh
 *
 * we also need another extra section at the start point of the first section,
 * since each section renders at the end point
 **/
int
plant::mesh_add_pre_branch_sections(plant_branch *br, struct vertex *v, int y)
{
    y = this->mesh_add_section(v, y, br->first->get_start_point(), br->first->get_vector(), 0.f);
    y = this->mesh_add_section(v, y, br->first->get_start_point(), br->first->get_vector(), br->first->get_width()*1.5f);

    return y;
}

int
plant::mesh_add_post_branch_sections(plant_branch *br, struct vertex *v, int y)
{
    return this->mesh_add_section(v, y, br->last->get_end_point(), br->last->get_vector(), 0.f);
}

void
plant::reset_counter()
{
    counter = 0;
}

void
plant::upload_buffers()
{
    if (buffer_modified) {
        tms_gbuffer_upload_partial(vbuf, (top_slot+1)*sizeof(struct vertex)*QUALITY*MAX_SECTIONS);
        buffer_modified = false;
    } else {
        //tms_debugf("no branch modified");
    }
}

int
plant::get_branch_mesh_slot()
{
    int x;

    for (x=0; x<MAX_BRANCHES-1; x++) {
        if (branch_slots[x] == 0) {
            if (__sync_fetch_and_add(&branch_slots[x], 1) == 0) {
                break;
            }
        }
    }

    if (x > top_slot) {
        top_slot = x;
    }

    //tms_debugf("received slot %d", x);

    return x;
}

void
plant::update_meshes(plant_branch *br)
{
    if (br->derived_body) {
        return;
    }

    if (!br->b) {
        return;
    }

    if (!br->first) {
        return;
    }

    bool do_update = br->needs_update;

    if (!br->mesh) {
        /* this branch does not have a mesh, we need to create one and allocate
         * a slot in the vertex buffer */
        tms_mesh_init(&br->_mesh, va, ibuf);
        tms_entity_set_material(br, static_cast<struct tms_material*>(&m_bark));
        br->mesh = &br->_mesh;
        br->prio = this->get_layer();
        do_update = true;
    }

    if (br->slot == -1 || br->slot == MAX_BRANCHES-1) {
        if (br->slot == -1) {
            do_update = true;
        }
        br->slot = this->get_branch_mesh_slot();
        br->mesh->i_start = br->slot*(MAX_SECTIONS*QUALITY*6);
    }

    plant_section *s = br->first;
    plant_section *last = br->last;

    struct vertex *v = (struct vertex*)tms_gbuffer_get_buffer(vbuf);
    v += br->slot * QUALITY*MAX_SECTIONS;

    int y = this->update_mesh(s, v, 0, !do_update);

    if (do_update) {
        br->mesh->i_count = (y-1)*6*QUALITY;
        buffer_modified = true;
    }

    tmat4_load_identity(br->M);
    b2Vec2 p = br->b->GetPosition();
    float a = br->b->GetAngle();
    tmat4_load_identity(br->M);
    tmat4_translate(br->M, p.x, p.y, this->get_layer()*LAYER_DEPTH);
    tmat4_rotate(br->M, a*(180.f/M_PI), 0, 0, -1.f);
    tmat3_copy_mat4_sub3x3(br->N, br->M);

    if (!((struct tms_entity*)br)->parent) {
        G->lock();
        tms_entity_set_uniform4f(br, "~color1", 73.f/255.f * .2f*1.0, 45.f/255.f * .2f*1.0, 34.f/255.f * .2f*1.0, 1.f);
        tms_entity_set_uniform4f(br, "~color2", 142.f/255.f * .6f*1.75, 141.f/255.f * .6f*1.75, 136.f/255.f * .6f*1.75, 1.f);

        tms_entity_add_child(this, br);
        G->unlock();
    }

    if (this->scene && !br->scene) {
        G->lock();
        tms_scene_add_entity(this->scene, br);
        G->unlock();
    }

    br->needs_update = false;
}

void
plant::kill_branch(plant_branch *br)
{
    if (!br->dead) {
        br->dead = true;
        this->end_section_fixture(br->last);
        br->last->stage = 2;
        this->update_section_fixture(br->last, 0);
    }
}

void
plant::grow_branch(plant_branch *br, float time)
{
    plant_section *s = br->first;

#if 0
    if (br->sections_left == 0) {
        /* don't grow */
        return;
    }
#endif

    while (s) {
        if (s->hp <= 0) {
            this->break_branch(br, s, true);
            return;
        }

        if (s->extension) {
            this->grow_branch(s->extension, time);
        }

        if (br->sections_left == 0 && br->last->growth > 1.f) {
            /* if this branch can not grow any more, don't bother increasing growth of all sections individually */
            if (s == br->last) {
                if (!br->dead) {
                    this->kill_branch(br);
                }
            }
            s = s->next;
            continue;
        }

        if (!br->dead) {
            if (fabsf(s->get_angle_displacement()) > PLANT_MAX_SECTION_ANGLE) {
                tms_debugf("section too bent, killing branch");
                this->kill_branch(br);
                break;
            }

            if (s == s->branch->last && s->branch->j_tmp) {
                float force = s->branch->j_tmp->GetReactionForce(1.f/(G->timemul(WORLD_STEP) * .000001)).LengthSquared() * G->get_time_mul();
                if (force > 200.f) break;
            }
        }

        s->growth += time;

        switch (s->stage) {
            case 0:
                this->update_section_fixture(s, time);

                if (s->growth >= 1.f) {
                    this->end_section_fixture(s);
                    s->stage = 1;
                    s->next = this->create_section(s);

                    if (s->next) {
                        this->begin_section_fixture(s->next, s->branch->b->GetLinearVelocity(), s->branch->b->GetAngularVelocity());
                        this->update_section_fixture(s->next, time);
                    }

                    if (this->should_create_branch(s))
                        s->extension = this->create_branch(s);

                    s = 0; /* break out of the while loop */
                } else
                    s = s->next;
                break;

            case 1:
                //s->stage = 2;
                this->update_section_fixture(s, time);
                s = s->next;
                break;

            default:
            case 2:
                s = s->next;
                break;
        }
    }

    this->adjust_branch_joint(br, false);

    if (br->leaf) {
        br->leaf->growth += time * .125f * (rand()%100)/100.f;
    }

    this->update_leaf(br);
}

void
plant::adjust_branch_joint(plant_branch *br, bool recursive)
{
    if (br->j) {
        /* slowly adjust joints to surrounding forces */
        float blend = .995f;
        float curr = (br->j->GetBodyB()->GetAngle()-br->j->GetBodyA()->GetAngle());
        float offs = (br->reference_angle - curr);
        br->reference_angle = blend*br->reference_angle + (1.f-blend)*curr;

        offs = offs*2.f*(br->depth+1);
        if (br->j->GetBodyA()->IsAwake() && br->j->GetBodyB()->IsAwake()) {
            br->j->SetMotorSpeed(offs);
        } else {
            br->j->SetMotorSpeed(0);
        }
        br->j->SetMaxMotorTorque(this->get_strength() / (br->depth*1.5f+1.f) * br->b->GetMass());
    }

    if (recursive) {
        plant_section *s = br->first;

        while (s) {
            if (s->hp <= 0) {
                this->break_branch(br, s, true);
                break;
            }
            if (s->extension) {
                this->adjust_branch_joint(s->extension, true);
            }
            s = s->next;
        }
    }
}

void
plant::init()
{
    this->pending_fixture = 0;
    this->pending_timer = 0.f;
    this->pending_normal = b2Vec2(0.f, 0.f);
}

void
plant::setup()
{
}

void
plant::tick()
{
    const float step = .5f;

    if (this->properties[12].v.f > 0.f) {
        float tstep = std::min(step, this->properties[12].v.f);

        this->grow_branch(&this->root_branch, tstep);

        this->properties[12].v.f -= tstep;
    }

#if 0
    plant_section *s = this->root_branch.first;
    while (s) {
        tms_debugf("section %f %f %f %f %f", s->pos.x, s->pos.y, s->angle, s->growth, s->width_growth);
        s = s->next;
    }
#endif
}

void
plant::restore()
{
}

void
plant::step()
{
    if (!this->c.pending) {
        const float step = .5f;

        if (this->properties[12].v.f > 0.f) {
            float tstep = std::min(step, this->properties[12].v.f);

            this->grow_branch(&this->root_branch, tstep);

            this->properties[12].v.f -= tstep;
        } else {
            this->grow_branch(&this->root_branch, G->get_time_mul() * (WORLD_DT) * .01f);
        }
    } else {
        this->adjust_branch_joint(&this->root_branch, true);

        /* XXX: This code is currently disabled while the seeds/plants
         * are not interactable while playing. */
        return;

        /* look through all contacts for something we can grow on */
        b2ContactEdge *c;
        entity *e = 0;
        bool found = false;
        for (c=this->body->GetContactList(); c; c = c->next) {

            b2Fixture *f = 0, *my;
            float nor_mul = 1.f;
            if (c->contact->GetFixtureA()->GetBody() == this->body) {
                f = c->contact->GetFixtureB();
                my = c->contact->GetFixtureA();
                nor_mul = -1.f;
            } else {
                f = c->contact->GetFixtureA();
                my = c->contact->GetFixtureB();
                nor_mul = 1.f;
            }

            if (f && (f == this->pending_fixture || this->pending_fixture == 0) && !f->IsSensor() && world::fixture_in_layer(f, this->get_layer())) {
                if ((e = (entity*)f->GetUserData()) && e != this && c->contact->IsTouching()) {
                    if (e->g_id == O_CHUNK || e->g_id == O_TPIXEL) {
                        if (b2Distance(this->body->GetLinearVelocity(), f->GetBody()->GetLinearVelocity()) < 0.05f) {
                            found = true;
                            this->pending_fixture = f;
                            b2WorldManifold man;
                            c->contact->GetWorldManifold(&man);
                            this->pending_normal = nor_mul*man.normal;
                            break;
                        }
                    }
                }
            }
        }

        if (!found) {
            this->pending_timer = 0.f;
        } else {
            this->pending_timer += (G->timemul(WORLD_STEP) * .000001);
        }

        if (this->pending_timer > PLANT_SEED_TIME && this->pending_fixture) {
            this->settle();
        }
    }
}

connection *
plant::load_connection(connection &conn)
{
    if (conn.o_index == 0) {
        this->c = conn;
        return &this->c;
    }

    return 0;
}

bool
plant::connection_destroy_joint(connection *c)
{
    this->root_branch.j = 0;
    return false;
}

void
plant::connection_create_joint(connection *c)
{
    b2RevoluteJointDef rjd;
    rjd.localAnchorA = c->p;
    //rjd.localAnchorB = c->p_s;
    rjd.localAnchorB = c->o->world_to_local(c->e->local_to_world(c->p, 0), 0);
    rjd.bodyA = this->body;
    rjd.bodyB = c->o->get_body(c->f[1]);

    //rjd.referenceAngle = this->b->GetAngle() - other->GetAngle();
    rjd.collideConnected = true;
    rjd.enableMotor = true;
    rjd.maxMotorTorque = 0.f;
    rjd.motorSpeed = 0.f;

    rjd.referenceAngle = this->root_branch.reference_angle = (rjd.bodyB->GetAngle() - rjd.bodyA->GetAngle());

    c->j = W->b2->CreateJoint(&rjd);

    tms_debugf("creating plant(%p) for conn(%p) joint(%p) between %p and %p", this, c, c->j, rjd.bodyA, rjd.bodyB);

    tms_assertf(rjd.bodyA && rjd.bodyB, "invalid body pointers in plant connection create joint");

    this->root_branch.j = (b2RevoluteJoint*)c->j;
}

/* called when the seed has been planted and we can start growing */
void
plant::settle()
{
    if (!this->c.pending) {
        return;
    }

    tms_debugf("planted on fixture %p", this->pending_fixture);

    this->body->SetAngularDamping(0.f);

    b2Body *b = this->pending_fixture->GetBody();
    this->body->SetTransform(this->body->GetPosition(), atan2f(this->pending_normal.y, this->pending_normal.x));

    this->c.p = this->get_position();
    this->c.o = static_cast<entity*>(this->pending_fixture->GetUserData());
    this->c.o_data = this->c.o->get_fixture_connection_data(this->pending_fixture);

    G->apply_connection(&this->c, -1);

    this->begin_section_fixture(&this->root_section, b2Vec2(0.f, 0.f), 0.f);
}


b2Body *
plant::create_body(b2BodyDef *bd)
{
    b2Body *b = W->b2->CreateBody(bd);
    b->SetAngularDamping(3.f);

    for (int x=0; x<this->num_bodies+1; x++) {
        if (x == this->num_bodies) {
            if (x < 64) {
                b->SetUserData((void*)(uintptr_t)(x+1));
                this->bodies[x] = b;
                this->num_bodies ++;
                break;
            } else {
                tms_fatalf("plant max bodies reached!");
            }
        } else {
            if (this->bodies[x] == 0) {
                b->SetUserData((void*)(uintptr_t)(x+1));
                this->bodies[x] = b;
                break;
            }
        }
    }

    return b;
}

void
plant::destroy_body(b2Body *b)
{
    uint8_t index = (uint8_t)(uintptr_t)b->GetUserData()-1;

    if (this->bodies[index] == b) {
        this->bodies[index] = 0;
    }

    W->b2->DestroyBody(b);
}

void
plant::set_position(float x, float y, uint8_t frame)
{
    b2Body *b = this->get_body(frame);

    if (frame == 0 && !b) {
        this->_pos = b2Vec2(x,y);
        return;
    }

    if (b) {
        b2Vec2 offs = b2Vec2(x,y)-this->get_position(frame);
        for (int x=0; x<this->get_num_bodies(); x++) {
            b2Body *b2 = this->get_body(x);

            if (b2) {
                b2->SetTransform(b2->GetPosition()+offs, b2->GetAngle());
            }
        }
    }
}

