#include "oilrig.hh"
#include "material.hh"
#include "model.hh"
#include "object_factory.hh"
#include "tpixel.hh"
#include "game.hh"
#include "item.hh"
#include "ledbuffer.hh"
#include "display.hh"

#define OIL_AMOUNT .0005f
#define RANGE 3.5f

class oil_handler  : public b2RayCastCallback
{
  public:
    tpixel_desc *result;

    oil_handler(){
        this->result = 0;
    };

    float32 ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
    {
        entity *e;

        if (f->IsSensor()) {
            return -1;
        }

        if ((e = static_cast<entity*>(f->GetUserData()))) {
            if (e->g_id == O_TPIXEL || e->g_id == O_CHUNK)  {
                tpixel_desc *desc = static_cast<tpixel_desc*>(f->GetUserData2());

                if (!desc) {
                    tms_debugf("tpixel does not have a desc!");
                    return -1;
                }

                if (desc->oil <= 0.f) {
                    return -1;
                }

                this->result = desc;
                return 0;
            }
        }

        return fraction;
    }
};

oilrig::oilrig()
{
    this->menu_scale = .25f;
    this->set_flag(ENTITY_IS_BETA,           true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);
    this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);
    this->set_flag(ENTITY_IS_MAGNETIC,       true);
    this->size = .5f;
    this->set_material(&m_colored);
    this->set_uniform("~color", .3f, .3f, .3f, 1.f);
    this->set_mesh(mesh_factory::get_mesh(MODEL_OILRIG));

    this->query_vec = b2Vec2(0.f, -.375f*this->size);
    this->query_pt = b2Vec2(0.f, -2.75f*this->size);

    this->set_flag(ENTITY_DO_STEP, true);
}

void oilrig::update_effects()
{
    if (this->get_body(0) && G && W && !W->is_paused()) {
        b2Vec2 pt = this->local_to_world(b2Vec2(-.5f, -1.25f), 0);
        b2Vec2 pt2 = this->local_to_world(b2Vec2(.5f, -1.25f), 0);
        ledbuffer::add(pt.x, pt.y, this->get_layer()*LAYER_DEPTH+.355f, this->c.pending?0.f:1.f);
        ledbuffer::add(pt2.x, pt2.y, this->get_layer()*LAYER_DEPTH+.355f, (!this->c.pending && !this->active)?roundf(.5f+.5f*sin((double)_tms.last_time*.00001)):0.f);

        float sn = sin(this->get_angle());
        float cs = cos(this->get_angle());
        b2Vec2 ptm = this->local_to_world(b2Vec2(0, -1.25f), 0);
        b2Vec2 ptm2 = this->local_to_world(b2Vec2(- .4f + this->oil_accum/2.f * .8f, -1.25f), 0);

        display::add_custom(
                ptm.x,
                ptm.y,
                this->get_layer()*LAYER_DEPTH+.55f,

                1.f / 0.08f * .8f,
                1.f / 0.08f * .10f,

                sn,cs,
                0.1f
                );

        display::add_custom(
                ptm2.x,
                ptm2.y,
                this->get_layer()*LAYER_DEPTH+.551f,

                1.f / 0.08f * this->oil_accum * .8f,
                1.f / 0.08f * .10f,

                sn,cs,
                0.9f
                );
    }
}

void
oilrig::setup()
{
    this->oil_accum = 0.f;
    this->active = false;
}

void
oilrig::step()
{
    this->active = false;

    if (!this->c.pending) {
        if (this->c.o->g_id == O_TPIXEL || this->c.o->g_id == O_CHUNK) {
            oil_handler h;

            for (int x=0; x<3 && !this->active; x++) {
                h.result = 0;

                b2Vec2 start = this->local_to_world(b2Vec2(-.3f*this->size+.3f*x, -2.75f*this->size), 0);
                b2Vec2 end = this->local_to_world(b2Vec2(-.3f*this->size+.3f*x*this->size, -2.75f*this->size-RANGE), 0);

                W->b2->RayCast(&h, start, end);

                if (h.result) {
                    tms_debugf("found pixel");
                    tpixel_desc *desc = h.result;

                    desc->oil -= 1.f*G->get_time_mul();
                    //this->oil_accum += OIL_AMOUNT * (1.f+tp->properties[1].v.i8 / 5.f) * G->get_time_mul();
                    this->oil_accum += OIL_AMOUNT * G->get_time_mul();
                    this->active = true;
                }
            }
        }
    }

    if (this->oil_accum > 1.f) {
        G->play_sound(SND_DING, this->get_position().x, this->get_position().y, 0, 1.f);

        item *barrel = (item*)of::create(O_ITEM);
        barrel->set_item_type(ITEM_OIL);
        barrel->_pos = this->local_to_world(b2Vec2(-1.f, 2.f),0);
        barrel->_angle = this->get_angle();
        barrel->set_layer(this->get_layer());

        G->emit(barrel, 0, b2Vec2(0.f,0.f));
        this->oil_accum -= 1.f;
    }
}

void
oilrig::add_to_world()
{
    b2BodyDef bd;
    bd.type = b2_dynamicBody;
    bd.position = _pos;
    bd.angle = _angle;
    b2Body *b = W->b2->CreateBody(&bd);
    this->body = b;

    b2PolygonShape box;
    box.SetAsBox(.8f*this->size, 3.6f*this->size, b2Vec2(0.f,.686f), 0);

    b2FixtureDef fd;
    fd.shape = &box;
    fd.density = 0.2f;
    fd.friction = .5f;
    fd.restitution = .3f;
    fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

    (this->body->CreateFixture(&fd))->SetUserData(this);
    
    /* bottom */
    box.SetAsBox(1.5f*this->size, .25f*this->size, b2Vec2(0.f, -2.5f*this->size), 0);
    (this->body->CreateFixture(&fd))->SetUserData(this);

    /* triangle top */

    b2Vec2 verts[3] = {
        b2Vec2(-.8f*this->size, 5.f*this->size),
        b2Vec2(.8f*this->size, 5.f*this->size),
        b2Vec2(0.f, 7.f*this->size)
    };
    box.Set(verts, 3);
    (this->body->CreateFixture(&fd))->SetUserData(this);
}

