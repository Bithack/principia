#include "scup.hh"
#include "model.hh"
#include "world.hh"
#include "game.hh"

class scup_ray_cb : public b2RayCastCallback
{
  public:
    entity *result[SCUP_NUM_JOINTS];
    b2Fixture *result_fx[SCUP_NUM_JOINTS];
    b2Vec2 result_point[SCUP_NUM_JOINTS];
    float result_dist[SCUP_NUM_JOINTS];
    uint8_t result_frame[SCUP_NUM_JOINTS];
    entity *ignore;
    int n;

    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
    {
        if (f->IsSensor()) {
            return -1.f;
        }

        entity *e = (entity*)f->GetUserData();

        if (e && e != this->ignore && e->get_layer() == this->ignore->get_layer()) {
            this->result[this->n] = e;
            this->result_fx[this->n] = f;
            this->result_point[this->n] = pt;
            this->result_frame[this->n] = (uint8_t)(uintptr_t)f->GetBody()->GetUserData();

            return fraction;
        }

        return fraction;
    }
} scup_cb;

static void
create_joint(b2Joint **dest, joint_info *ji, entity *a, entity *b, uint8_t fr, float force, b2Vec2 pt=b2Vec2(0.f, 0.f))
{
    b2WeldJointDef wjd;
    wjd.localAnchorA = a->world_to_body(a->get_position(), 0);
    wjd.localAnchorB = (pt.Length() < .0001f ? b->world_to_body(a->get_position(), fr) : pt);
    wjd.bodyA = a->get_body(0);
    wjd.bodyB = b->get_body(fr);
    wjd.collideConnected = true;
    wjd.frequencyHz = 0.f;
    wjd.referenceAngle = wjd.bodyB->GetAngle()-wjd.bodyA->GetAngle();

    tms_debugf("Created joint between A(%p) and B(%p)", a, b);
    ((*dest) = W->b2->CreateJoint(&wjd))->SetUserData(ji);
    G->add_destructable_joint(*dest, force);
}

scup::scup()
{
    this->ji = new joint_info(JOINT_TYPE_SCUP, this);

    /* memory leakage!! :-) */
    this->ji->should_destroy = false;

    this->set_flag(ENTITY_DO_STEP,              true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);

    this->set_mesh(mesh_factory::get_mesh(MODEL_SUCTIONCUP));
    this->set_material(&m_edev_dark);

    this->num_s_in  = 1;
    this->num_s_out = 1;

    this->s_in[0].lpos = b2Vec2(-.125f, .125f);
    this->s_out[0].lpos = b2Vec2(.125f, .125f);

    this->set_as_rect(.5f/1.5f, .35f/1.5f);

    this->query_sides[2].SetZero(); /* Disable connections downwards */
}

void
scup::setup()
{
    this->stuck = false;
    this->strength_mod = 0.f;

    for (int n=0; n<SCUP_NUM_JOINTS;++n) {
        this->j[n] = 0;
        this->a[n].id = 0;
    }
}

void
scup::restore()
{
    entity *e;

    for (int x=0; x<SCUP_NUM_JOINTS; x++) {
        if (this->a[x].id) {
            if ((e = W->get_entity_by_id(this->a[x].id))) {
                create_joint(&this->j[x], this->ji, this, e, this->a[x].frame, this->a[x].force, this->a[x].body_pt);
                continue;
            }
        }

        this->j[x] = 0;
    }
}

void
scup::step()
{
    if (this->strength_mod <= 0.f) {
        this->stuck = false;
    }

    if (this->stuck) {
        return;
    }

    for (int x=0; x<SCUP_NUM_JOINTS; ++x) {
        if (this->j[x]) {
            this->j[x]->SetUserData((void*)0);
            G->destroy_joint(this->j[x]);
            this->j[x] = 0;
        }
    }

    if (this->strength_mod > 0.f) {
        b2Vec2 from[SCUP_NUM_JOINTS];
        b2Vec2 to[SCUP_NUM_JOINTS];

        for (int x=0; x<SCUP_NUM_JOINTS; x++) {
            from[x] = this->local_to_world(b2Vec2(-SCUP_WIDTH/2.f + SCUP_WIDTH * ((float)x/SCUP_NUM_JOINTS), 0.f), 0);
            to[x] = this->local_to_world(b2Vec2(-SCUP_WIDTH/2.f + SCUP_WIDTH * ((float)x/SCUP_NUM_JOINTS), -SCUP_REACH), 0);
        }

        scup_cb.ignore = this;
        int num_results = 0;
        for (int n=0; n<SCUP_NUM_JOINTS; ++n) {
            scup_cb.result[n] = 0;
            scup_cb.result_dist[n] = 10.f;
            scup_cb.n = n;

            W->b2->RayCast(&scup_cb, from[n], to[n]);
            if (scup_cb.result[n]) {
                num_results ++;
            }
        }

        if (num_results == SCUP_NUM_JOINTS) {
            struct {
                entity *e;
                uint8_t fr;
                int count;
            } joined[SCUP_NUM_JOINTS];
            memset(&joined, 0, sizeof(joined));

            for (int x=0; x<SCUP_NUM_JOINTS; x++) {
                for (int y=0; y<SCUP_NUM_JOINTS; y++) {
                    if (joined[y].e != 0) {
                        if (joined[y].e != scup_cb.result[x] || joined[y].fr != scup_cb.result_frame[x]) {
                            continue;
                        }
                    }

                    joined[y].e = scup_cb.result[x];
                    joined[y].fr = scup_cb.result_frame[x];
                    joined[y].count ++;
                    break;
                }
            }

            for (int x=0; x<SCUP_NUM_JOINTS; x++) {
                if (joined[x].e == 0)
                    break;

                float force = SCUP_JOINT_FORCE*joined[x].count;
                create_joint(&this->j[x], this->ji, this, joined[x].e, joined[x].fr, force);

                this->a[x].id = joined[x].e->id;
                this->a[x].frame = joined[x].fr;
                this->a[x].force = force;
                this->a[x].body_pt = static_cast<b2WeldJoint*>(this->j[x])->GetLocalAnchorB();
            }

            this->stuck = true;
        }
    }
}

void
scup::write_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::write_state(lvl, lb);

    lb->w_uint8((uint8_t)this->stuck);
    lb->w_float(this->strength_mod);

    for (int x=0; x<SCUP_NUM_JOINTS; x++) {
        if (this->j[x]) {
            lb->w_s_uint8(1);
            lb->w_s_id(this->a[x].id);
            lb->w_s_uint8(this->a[x].frame);
            lb->w_s_float(this->a[x].force);
            lb->w_s_float(this->a[x].body_pt.x);
            lb->w_s_float(this->a[x].body_pt.y);
        } else {
            lb->w_s_uint8(0);
        }
    }
}

void
scup::read_state(lvlinfo *lvl, lvlbuf *lb)
{
    entity::read_state(lvl, lb);

    this->stuck = (bool)lb->r_uint8();
    this->strength_mod = lb->r_float();

    for (int x=0; x<SCUP_NUM_JOINTS; x++) {
        uint8_t v = lb->r_uint8();

        if (v) {
            p_id entity_id = lb->r_id();
            uint8_t fr = lb->r_uint8();
            float force = lb->r_float();
            float ptx = lb->r_float();
            float pty = lb->r_float();

            this->a[x].id = entity_id;
            this->a[x].frame = fr;
            this->a[x].force = force;
            this->a[x].body_pt = b2Vec2(ptx, pty);
        } else {
            this->a[x].id = 0;
        }
    }
}

edevice*
scup::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    this->strength_mod = this->s_in[0].get_value();

    this->s_out[0].write(this->stuck?1.f:0.f);

    return 0;
}
