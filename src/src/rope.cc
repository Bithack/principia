#include "rope.hh"
#include "world.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"

#define SEGSIZE .5f
#define WIDTH (.125f*.75f)
#define EPS .075f
#define QUALITY 6

#define MAX_ROPES 20

static bool initialized = false;
static struct tms_mesh   *_mesh;
static struct tms_entity *_e;
static float _cam_x = 0.f, _cam_y = 0.f;
static tms::varray *va = 0;
static tms::gbuffer *buf = 0;
static tms::gbuffer *ibuf = 0;

static volatile int counter = 1;

struct vertex {
    tvec3 pos;
    tvec3 nor;
    tvec2 tex;
} __attribute__ ((packed));

void
rope::_init()
{
    if (initialized)
        return;

    b2Vec2 dir;

    buf = new tms::gbuffer(MAX_ROPES * ROPE_LENGTH * QUALITY * sizeof(struct vertex));
    ibuf = new tms::gbuffer(MAX_ROPES * (ROPE_LENGTH-1) * (QUALITY) * 6 * sizeof(short));

    buf->usage = GL_STREAM_DRAW;
    ibuf->usage = GL_STATIC_DRAW;

    va = new tms::varray(3);
    va->map_attribute("position", 3, GL_FLOAT, buf);
    va->map_attribute("normal", 3, GL_FLOAT, buf);
    va->map_attribute("texcoord", 2, GL_FLOAT, buf);

    _mesh = tms_mesh_alloc(va, ibuf);
    _e = tms_entity_alloc();
    tms_entity_set_mesh(_e, _mesh);
    tms_entity_set_material(_e, static_cast<struct tms_material*>(&m_rope));
    tmat4_load_identity(_e->M);
    tmat3_load_identity(_e->N);

    struct vertex *v = (struct vertex*)buf->get_buffer();
    short *id = (short*)ibuf->get_buffer();

    int num_i = 0;
    int v_offs = 0;

    for (int z=0; z<MAX_ROPES; z++) {
        v_offs = (z*ROPE_LENGTH*QUALITY);
        for (int y=0; y<ROPE_LENGTH; y++) {
            for (int x=0; x<QUALITY; x++) {
                v[v_offs+y*QUALITY+x].tex = (tvec2){y/(float)ROPE_LENGTH, 1.f - ((float)x/QUALITY)};
            }

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
    //buf->upload();

    initialized = true;
}

struct tms_entity*
rope::get_entity(void)
{
    return _e;
}

void
rope::reset_counter(void)
{
    counter = 0;

    if (G) {
        _cam_x = G->cam->_position.x;
        _cam_y = G->cam->_position.y;
    }

    if (_e) {
        tmat4_load_identity(_e->M);
        tmat4_translate(_e->M, _cam_x, _cam_y, 0.f);
    }
}

uint32_t
rope::get_num_bodies()
{
    return 2;
}

b2Body*
rope::get_body(uint8_t frame)
{
    return this->ends[frame]->body;
}

void
rope::set_angle(float a, uint8_t frame)
{
    this->ends[frame]->body->SetTransform(this->ends[frame]->body->GetPosition(), a);
}

void
rope::set_position(float x, float y, uint8_t frame)
{
    b2Body *b = this->ends[frame]->body;
    if (b) {
        b->SetTransform(b2Vec2(x,y), b->GetAngle());
    }
}

void
rope::upload_buffers(void)
{
    if (counter > MAX_ROPES-1) counter = MAX_ROPES-1;
    _mesh->i_start = 0;
    _mesh->i_count = counter*(ibuf->size/MAX_ROPES) / sizeof(uint16_t);
    if (counter)
        buf->upload_partial(counter * QUALITY * ROPE_LENGTH * sizeof(struct vertex));
    //ibuf->upload();
}

void
rope::find_pairs()
{
    //tms_infof("find pairs ---");
    for (int x=0; x<2; x++) {
        if (this->end_conns[x].pending) {
            this->query_result = 0;

            b2Vec2 v = b2Vec2(0, .35f * (x == 0 ? 1.f : -1.f));

            W->b2->RayCast(this,
                    this->ends[x]->get_position(),
                    this->ends[x]->local_to_world(v, 0));

            if (this->query_result) {
                //tms_infof("FOUND -------------------------------");
                this->end_conns[x].o = this->query_result;
                this->end_conns[x].f[0] = x;
                this->end_conns[x].f[1] = this->query_frame;
                //this->end_conns[x].angle = atan2f(v.y, v.x);
                this->end_conns[x].angle = M_PI/2.f;
                this->end_conns[x].o_data = this->query_result->get_fixture_connection_data(this->query_result_fx);
                b2Vec2 vv = v;
                vv *= this->query_fraction*.75f;
                //vv*=.5f;
                this->end_conns[x].p = this->ends[x]->local_to_world(vv, 0);
                G->add_pair(this, this->query_result, &this->end_conns[x]);
            }
        }
    }
}

connection *
rope::load_connection(connection &conn)
{
    this->end_conns[conn.o_index] = conn;
    this->end_conns[conn.o_index].render_type = CONN_RENDER_SMALL;
    return &this->end_conns[conn.o_index];
}

rope_end::rope_end()
{
    this->set_mesh(mesh_factory::get_mesh(MODEL_ROPEEND));
    this->set_material(&m_wood);

    this->set_flag(ENTITY_IS_OWNED, true);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
}

void
rope_end::add_to_world()
{
    this->create_rect(b2_dynamicBody, .125f, .125f, this->material);
    if (this->body) {this->body->GetFixtureList()[0].SetDensity(2.f);this->body->ResetMassData();}
}

float32
rope::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    if (f->IsSensor()) {
        return -1.f;
    }

    b2Body *b = f->GetBody();
    entity *e = static_cast<entity*>(f->GetUserData());

    if (e && e->allow_connections() && e->get_layer() == this->get_layer()) {
        if (e == this) {
            return 1;
        }
        this->query_result = e;
        this->query_result_fx = f;
        this->query_frame = VOID_TO_UINT8(b->GetUserData());
        this->query_fraction = fraction;
    }

    return -1;
}

void
rope::set_layer(int l)
{
    this->ends[0]->set_layer(l);
    this->ends[1]->set_layer(l);

    struct tms_scene *scene = this->scene;

    if (scene)
        tms_scene_remove_entity(scene, this);

    this->prio = l;

    if (this->rb[0]) {
        for (int x=0; x<ROPE_LENGTH-1; x++) {
            b2Filter filter = world::get_filter_for_layer(l, this->layer_mask);
            for (b2Fixture *f = this->rb[x]->GetFixtureList(); f; f=f->GetNext()) {
                b2Filter curr = f->GetFilterData();
                filter.groupIndex = curr.groupIndex;
                f->SetFilterData(filter);
            }
        }
    }

    if (scene)
        tms_scene_add_entity(scene, this);
}

rope::rope()
{
    rope::_init();

    this->set_flag(ENTITY_CUSTOM_GHOST_UPDATE, true);
    this->set_flag(ENTITY_ALLOW_ROTATION, false);
    this->set_flag(ENTITY_DO_STEP, true);

    this->menu_scale = .6f;
    this->rb[0] = 0;
    this->update_method = ENTITY_UPDATE_CUSTOM;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    /*
    tms_infof("init rope with num %d", counter);
    this->num = 1;

    tms::mesh *m = new tms::mesh(va, ibuf);
    m->i_start = ((ibuf->size/MAX_ROPES) * this->num)/sizeof(short);
    m->i_count = (ibuf->size/MAX_ROPES) / sizeof(short);

    this->set_mesh(m);
    this->set_material(&m_rope);
    */

    this->set_num_properties((ROPE_LENGTH-1)*3 + 6);
    for (int x=0; x<(ROPE_LENGTH-1)*3 + 6; x++) {
        this->properties[x].type = P_FLT;
    }

    this->ends[0] = new rope_end();
    this->ends[1] = new rope_end();

    this->add_child(this->ends[0]);
    this->add_child(this->ends[1]);

    this->end_conns[0].init_owned(0, this);
    this->end_conns[0].f[0] = 0;
    this->end_conns[0].type = CONN_PLATE;
    this->end_conns[0].render_type = CONN_RENDER_SMALL;

    this->end_conns[1].init_owned(1, this);
    this->end_conns[1].f[1] = 1;
    this->end_conns[1].type = CONN_PLATE;
    this->end_conns[1].render_type = CONN_RENDER_SMALL;

    this->set_uniform("~color", 1.f, 1.f, 1.f, 1.f);
    this->refresh_predef_form();
}

void
rope::pre_write()
{
    for (int x=0; x<(ROPE_LENGTH-1); x++) {
        b2Vec2 p = this->rb[x]->GetPosition();
        float a = this->rb[x]->GetAngle();

        this->properties[x*3+0].v.f = p.x;
        this->properties[x*3+1].v.f = p.y;

        /*
        float tx,ty;
        tx = cosf(a);
        ty = sinf(a);

        float aa = atan2f(ty,tx);
        if (aa < 0.f) aa+= M_PI*2.f;
*/
      //  this->properties[x*3+2].v.f = aa;
        this->properties[x*3+2].v.f = a;
    }

    b2Vec2 p = this->ends[0]->get_position();
    this->properties[(ROPE_LENGTH-1)*3+0].v.f = p.x;
    this->properties[(ROPE_LENGTH-1)*3+1].v.f = p.y;
    this->properties[(ROPE_LENGTH-1)*3+2].v.f = this->ends[0]->get_angle();

    p = this->ends[1]->get_position();
    this->properties[(ROPE_LENGTH-1)*3+3].v.f = p.x;
    this->properties[(ROPE_LENGTH-1)*3+4].v.f = p.y;
    this->properties[(ROPE_LENGTH-1)*3+5].v.f = this->ends[1]->get_angle();
}

float
rope::get_angle(uint8_t frame)
{
    return this->ends[frame]->get_angle();
}

void
rope::refresh_predef_form()
{
    this->properties[0*3+0].v.f = 0.166879+this->_pos.x;
    this->properties[0*3+1].v.f = 1.112272+this->_pos.y;
    this->properties[0*3+2].v.f = -0.432185;
    this->properties[1*3+0].v.f = -0.069718+this->_pos.x;
    this->properties[1*3+1].v.f = 0.672875+this->_pos.y;
    this->properties[1*3+2].v.f = -0.555693;
    this->properties[2*3+0].v.f = -0.252231+this->_pos.x;
    this->properties[2*3+1].v.f = 0.215672+this->_pos.y;
    this->properties[2*3+2].v.f = -0.203932;
    this->properties[3*3+0].v.f = -0.148051+this->_pos.x;
    this->properties[3*3+1].v.f = -0.225448+this->_pos.y;
    this->properties[3*3+2].v.f = 0.667778;
    this->properties[4*3+0].v.f = 0.248905+this->_pos.x;
    this->properties[4*3+1].v.f = -0.483922+this->_pos.y;
    this->properties[4*3+2].v.f = 1.319462;
    this->properties[5*3+0].v.f = 0.732940+this->_pos.x;
    this->properties[5*3+1].v.f = -0.482933+this->_pos.y;
    this->properties[5*3+2].v.f = 1.826215;
    this->properties[6*3+0].v.f = 1.180168+this->_pos.x;
    this->properties[6*3+1].v.f = -0.277167+this->_pos.y;
    this->properties[6*3+2].v.f = 2.177806;
    this->properties[7*3+0].v.f = 1.566566+this->_pos.x;
    this->properties[7*3+1].v.f = 0.037825+this->_pos.y;
    this->properties[7*3+2].v.f = 2.331669;
    this->properties[8*3+0].v.f = 1.621603+this->_pos.x;
    this->properties[8*3+1].v.f = 0.426127+this->_pos.y;
    this->properties[8*3+2].v.f = 3.669916;
    this->properties[9*3+0].v.f = 1.250687+this->_pos.x;
    this->properties[9*3+1].v.f = 0.692306+this->_pos.y;
    this->properties[9*3+2].v.f = -1.773242;
    this->properties[10*3+0].v.f = 0.810644+this->_pos.x;
    this->properties[10*3+1].v.f = 0.586314+this->_pos.y;
    this->properties[10*3+2].v.f = -0.895622;
    this->properties[11*3+0].v.f = 0.271593+this->_pos.x;
    this->properties[11*3+1].v.f = 1.339285+this->_pos.y;
    this->properties[11*3+2].v.f = -0.432185;
    this->properties[12*3+0].v.f = 0.615494+this->_pos.x;
    this->properties[12*3+1].v.f = 0.430056+this->_pos.y;
    this->properties[12*3+2].v.f = -0.895622;
}

void
rope::ghost_update(void)
{
    this->num = __sync_fetch_and_add(&counter, 1);

    if (this->num >= MAX_ROPES) this->num = MAX_ROPES-1;

    int base = this->num*(((buf->size/MAX_ROPES))/sizeof(struct vertex));
    struct vertex *v = ((struct vertex*)buf->get_buffer()) + base;

    this->refresh_predef_form();

    float step = (M_PI*2.f) / (float)QUALITY;

    b2Vec2 last_axis;

    for (int y=0; y<ROPE_LENGTH; y++) {
        b2Vec2 bp;
        b2Vec2 nbp;

        b2Vec2 axis;

        if (y < ROPE_LENGTH-1) {
            bp = b2Vec2(this->properties[y*3+0].v.f, this->properties[y*3+1].v.f);
            nbp = bp;
            float a = this->properties[y*3+2].v.f+M_PI/2.f;
            float cs,sn;
            tmath_sincos(a, &sn, &cs);
            bp += b2Vec2(SEGSIZE/2.f * cs, SEGSIZE/2.f * sn);
            nbp += b2Vec2(-SEGSIZE/2.f * cs, -SEGSIZE/2.f * sn);
            axis = nbp - bp;
        } else {
            bp = b2Vec2(this->properties[(y-1)*3+0].v.f, this->properties[(y-1)*3+1].v.f);
            nbp = bp;
            float a = this->properties[(y-1)*3+2].v.f+M_PI/2.f;
            float cs,sn;
            tmath_sincos(a, &sn, &cs);
            bp += b2Vec2(-SEGSIZE/2.f * cs, -SEGSIZE/2.f * sn);
            nbp += b2Vec2(SEGSIZE/2.f * cs,  SEGSIZE/2.f * sn);
            axis = bp - nbp;
        }

        axis *= 1.f/axis.Length();

        float width = WIDTH;

        if (y > 0) {
            b2Vec2 tmp = axis;

            float adiff = tmath_adist(atan2f(tmp.y, tmp.x), atan2f(last_axis.y, last_axis.x));

            /*if (adiff > M_PI/2.f)
                width *= 2.f;*/
            width += fabsf(adiff)/25.f;

            axis += last_axis;
            axis *= .5f;

            last_axis = tmp;
        } else
            last_axis = axis;

        for (int x=0; x<QUALITY; x++) {
            float mat[16];
            float a = step * (float)x;
            tmat4_load_identity(mat);
            tmat4_rotate(mat, a * (180.f / M_PI), axis.x, axis.y, 0.f);

            tvec4 pt = (tvec4){0.f, 0.f, -width, 1.f};
            tvec4_mul_mat4(&pt, mat);

            v[y*QUALITY+x].pos = (tvec3){pt.x + bp.x - _cam_x, bp.y + pt.y - _cam_y, pt.z};

            tvec3_normalize((tvec3*)&pt);
            v[y*QUALITY+x].nor = (tvec3){pt.x, pt.y, pt.z};
        }
    }

    /*
    this->ends[0]->_pos = this->_pos;
    this->ends[0]->_pos+=b2Vec2(-.5f, 0.f);
    this->ends[1]->_pos = this->_pos;
    this->ends[1]->_pos+=b2Vec2(.5f, 0.f);
    */

    this->ends[0]->_pos = b2Vec2(this->properties[(ROPE_LENGTH-1)*3+0].v.f, this->properties[(ROPE_LENGTH-1)*3+1].v.f);
    this->ends[0]->_angle = this->properties[(ROPE_LENGTH-1)*3+2].v.f;

    this->ends[1]->_pos = b2Vec2(this->properties[(ROPE_LENGTH-1)*3+3].v.f, this->properties[(ROPE_LENGTH-1)*3+4].v.f);
    this->ends[1]->_angle = this->properties[(ROPE_LENGTH-1)*3+5].v.f;

    this->ends[0]->update();
    this->ends[1]->update();
}

void
rope::construct()
{
}

void
rope::update(void)
{
    this->num = __sync_fetch_and_add(&counter, 1);

    if (this->num >= MAX_ROPES) this->num = MAX_ROPES-1;

    int base = this->num*(((buf->size/MAX_ROPES))/sizeof(struct vertex));
    struct vertex *v = ((struct vertex*)buf->get_buffer()) + base;

    float step = (M_PI*2.f) / (float)QUALITY;

    b2Vec2 last_axis;

    for (int y=0; y<ROPE_LENGTH; y++) {
        b2Vec2 bp;
        b2Vec2 nbp;

        b2Vec2 axis;

        if (y < ROPE_LENGTH-1) {
            bp = this->rb[y]->GetWorldPoint(b2Vec2(0.f, SEGSIZE/2.f));
            nbp = this->rb[y]->GetWorldPoint(b2Vec2(0.f, -SEGSIZE/2.f));
            axis = nbp - bp;
        } else {
            bp = this->rb[y-1]->GetWorldPoint(b2Vec2(0.f, -SEGSIZE/2.f));
            axis = bp - this->rb[y-1]->GetWorldPoint(b2Vec2(0.f, SEGSIZE/2.f));
        }

        axis *= 1.f/axis.Length();

        float width = WIDTH;

        if (y > 0) {
            b2Vec2 tmp = axis;

            float adiff = tmath_adist(atan2f(tmp.y, tmp.x), atan2f(last_axis.y, last_axis.x));

            /*if (adiff > M_PI/2.f)
                width *= 2.f;*/
            width += fabsf(adiff)/25.f;

            axis += last_axis;
            axis *= .5f;

            last_axis = tmp;
        } else
            last_axis = axis;

        for (int x=0; x<QUALITY; x++) {
            float mat[16];
            float a = step * (float)x;
            tmat4_load_identity(mat);
            tmat4_rotate(mat, a * (180.f / M_PI), axis.x, axis.y, 0.f);

            tvec4 pt = (tvec4){0.f, 0.f, -width, 1.f};
            tvec4_mul_mat4(&pt, mat);

            v[y*QUALITY+x].pos = (tvec3){pt.x + bp.x - _cam_x, bp.y + pt.y - _cam_y, pt.z + this->get_layer() * LAYER_DEPTH};

            tvec3_normalize((tvec3*)&pt);
            v[y*QUALITY+x].nor = (tvec3){pt.x, pt.y, pt.z};
        }
    }

    this->ends[0]->update();
    this->ends[1]->update();
}

void
rope::remove_from_world()
{
    if (this->rb[0]) {
        for (int x=0; x<ROPE_LENGTH-1; x++) {
            W->b2->DestroyBody(this->rb[x]);
        }

        this->rb[0] = 0;
    }

    /* XXX ??? */
    this->ends[0]->remove_from_world();
    this->ends[1]->remove_from_world();
}

void
rope::add_to_world()
{
    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
    this->ends[0]->_pos = b2Vec2(this->properties[(ROPE_LENGTH-1)*3+0].v.f, this->properties[(ROPE_LENGTH-1)*3+1].v.f);
    this->ends[0]->_angle = this->properties[(ROPE_LENGTH-1)*3+2].v.f;

    this->ends[1]->_pos = b2Vec2(this->properties[(ROPE_LENGTH-1)*3+3].v.f, this->properties[(ROPE_LENGTH-1)*3+4].v.f);
    this->ends[1]->_angle = this->properties[(ROPE_LENGTH-1)*3+5].v.f;

    this->ends[0]->add_to_world();
    this->ends[1]->add_to_world();

    this->ends[0]->get_body(0)->GetFixtureList()->SetUserData(this);
    this->ends[1]->get_body(0)->GetFixtureList()->SetUserData(this);
    this->ends[0]->get_body(0)->SetUserData(0);
    this->ends[1]->get_body(0)->SetUserData((void*)1);

    b2BodyDef bd;
    bd.type = b2_dynamicBody;

    b2PolygonShape box;
    box.SetAsBox(WIDTH, SEGSIZE/2.f + EPS);

    b2FixtureDef fd;
    fd.shape = &box;
    fd.density = 1.f; /* XXX */
    fd.friction = .3f;
    fd.restitution = .0f;

    fd.filter = world::get_filter_for_layer(this->prio, 6);
    fd.filter.groupIndex = -2;

    b2RevoluteJointDef rjd;
    b2WeldJointDef wjd;

    b2WeldJointDef wjd2;
    wjd2.localAnchorA.Set(0.f, 0.f);
    wjd2.localAnchorB.Set(0.f, SEGSIZE/2.f);
    wjd2.frequencyHz = 0.f;
    wjd2.collideConnected = false;

    if (true || W->is_paused()) {
        rjd.localAnchorA.Set(0.f, -SEGSIZE/2.f);
        rjd.localAnchorB.Set(0.f, SEGSIZE/2.f);
        rjd.collideConnected = false;
    } else {
        wjd.localAnchorA.Set(0.f, -SEGSIZE/2.f);
        wjd.localAnchorB.Set(0.f, SEGSIZE/2.f);
        //rjd.lowerAngle = -.9f;
        //rjd.upperAngle = .9f;
        //rjd.enableLimit = true;
        wjd.frequencyHz = 8.f;
        wjd.collideConnected = false;
    }

    for (int x=0; x<ROPE_LENGTH-1; x++) {
        bd.type = b2_dynamicBody;

        float a = M_PI+(M_PI/(float)(ROPE_LENGTH-1))*x;

        bd.position.x = this->properties[x*3+0].v.f;
        bd.position.y = this->properties[x*3+1].v.f;
        bd.angle = this->properties[x*3+2].v.f;

        b2Body *b = W->b2->CreateBody(&bd);
        (b->CreateFixture(&fd));//->SetUserData(this);

        this->rb[x] = b;

        if (x>0) {
            if (true || W->is_paused()) {
                rjd.bodyA = this->rb[x-1];
                rjd.bodyB = this->rb[x];
                W->b2->CreateJoint(&rjd);
            } else {
                wjd.bodyA = this->rb[x-1];
                wjd.bodyB = this->rb[x];
                W->b2->CreateJoint(&wjd);

            }

            if (x == ROPE_LENGTH-2) {
                wjd2.bodyA = this->ends[1]->get_body(0);
                wjd2.bodyB = this->rb[x];
                wjd2.localAnchorB.Set(0.f, -SEGSIZE/2.f);
                W->b2->CreateJoint(&wjd2);
                /* last body, connect to the second rope end */
            }
        } else {
            /* x == 0, connect to the first rope end */
            wjd2.localAnchorB.Set(0.f, SEGSIZE/2.f);
            wjd2.bodyA = this->ends[0]->get_body(0);
            wjd2.bodyB = this->rb[x];
            W->b2->CreateJoint(&wjd2);
        }
    }

    b2RopeJointDef ropejd;
    ropejd.bodyA = this->ends[0]->get_body(0);
    ropejd.localAnchorA = b2Vec2(0.f,0.f);
    ropejd.bodyB = this->ends[1]->get_body(0);
    ropejd.localAnchorB = b2Vec2(0.f,0.f);
    ropejd.maxLength = ROPE_LENGTH*SEGSIZE;
    W->b2->CreateJoint(&ropejd);

    if (W->is_paused()) {
        for (int x=0; x<ROPE_LENGTH-1; x++) {
            this->rb[x]->SetLinearDamping(INFINITY);
            this->rb[x]->SetAngularDamping(INFINITY);
            this->rb[x]->SetLinearVelocity(b2Vec2(0,0));
            this->rb[x]->SetAngularVelocity(0);
            this->rb[x]->SetSleepingAllowed(false);
        }
    } else {
        for (int x=0; x<ROPE_LENGTH-1; x++) {
            if (W->level.version >= LEVEL_VERSION_1_2_4) {
                this->rb[x]->SetLinearDamping(1.f);
                this->rb[x]->SetAngularDamping(0.1f);
            } else {
                this->rb[x]->SetLinearDamping(0.0f);
                this->rb[x]->SetAngularDamping(0.0f);
            }
            this->rb[x]->SetLinearVelocity(b2Vec2(0,0));
            this->rb[x]->SetAngularVelocity(0);
            this->rb[x]->SetAwake(true);
        }
    }
}

void
rope::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    for (uint32_t x=0; x<this->get_num_bodies(); ++x) {
        b2Vec2 velocity = this->get_body(x) ? this->get_body(x)->GetLinearVelocity() : b2Vec2(0.f, 0.f);
        float avel = this->get_body(x) ? this->get_body(x)->GetAngularVelocity() : 0.f;

        lb->ensure(3*sizeof(float));
        lb->w_float(velocity.x);
        lb->w_float(velocity.y);
        lb->w_float(avel);
    }

    for (uint32_t x=0; x<ROPE_LENGTH-1; ++x) {
        b2Body *b = this->rb[x];

        b2Vec2 velocity = b ? b->GetLinearVelocity() : b2Vec2(0.f, 0.f);
        float avel = b ? b->GetAngularVelocity() : 0.f;

        lb->ensure(3*sizeof(float));
        lb->w_float(velocity.x);
        lb->w_float(velocity.y);
        lb->w_float(avel);
    }
}

void
rope::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    for (uint32_t x=0; x<this->get_num_bodies(); ++x) {
        this->state[x*3 + 0] = lb->r_float();
        this->state[x*3 + 1] = lb->r_float();
        this->state[x*3 + 2] = lb->r_float();
    }

    uint32_t offset = this->get_num_bodies() * 3;
    for (uint32_t x=0; x<ROPE_LENGTH-1; ++x) {
        this->state[offset + x*3 + 0] = lb->r_float();
        this->state[offset + x*3 + 1] = lb->r_float();
        this->state[offset + x*3 + 2] = lb->r_float();
    }
}

void
rope::restore()
{
    for (uint32_t x=0; x<this->get_num_bodies(); ++x) {
        this->get_body(x)->SetLinearVelocity(b2Vec2(this->state[x*3 + 0], this->state[x*3 + 1]));
        this->get_body(x)->SetAngularVelocity(this->state[x*3 + 2]);
    }

    uint32_t offset = this->get_num_bodies() * 3;
    for (uint32_t x=0; x<ROPE_LENGTH-1; ++x) {
        b2Body *b = this->rb[x];
        b->SetLinearVelocity(b2Vec2(this->state[offset + x*3+0], this->state[offset + x*3+1]));
        b->SetAngularVelocity(this->state[offset + x*3+2]);
    }
}

void
rope::step()
{
}
