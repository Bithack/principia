#include "rail.hh"
#include "material.hh"
#include "model.hh"
#include "world.hh"

static b2Vec2 r_offs[] = {
    b2Vec2(0.f, 0.f),
    b2Vec2(0.f, 0.5f),
    b2Vec2(-.727, .846),
    b2Vec2(0.f, 0.5f),
};

rail::rail(int type)
{
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_IS_MAGNETIC,          true);
    this->set_flag(ENTITY_IS_STATIC,            true);

    this->railtype = type;

    this->menu_scale = .25f;
    this->width = 1.f;
    this->set_material(&m_rail);
    //this->set_uniform("~color", .4f, .4f, .4f, 1.f);
    this->update_method = ENTITY_UPDATE_STATIC;

    switch (this->railtype) {
        default:case RAIL_STRAIGHT:
            this->set_mesh(mesh_factory::get_mesh(MODEL_RAILSTRAIGHT));
            break;
        case RAIL_SKEWED:
            this->set_mesh(mesh_factory::get_mesh(MODEL_RAILSKEWED));
            break;
        case RAIL_SKEWED2:
            this->set_mesh(mesh_factory::get_mesh(MODEL_RAILSKEWED2));
            break;
        case RAIL_45DEG:
            this->set_mesh(mesh_factory::get_mesh(MODEL_RAILTURN));
            break;
    }

    this->layer_mask = 6;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
}

void
rail::set_angle(float a)
{
    a /= M_PI/2.f;
    a = roundf(a);
    a *= M_PI/2.f;

    entity::set_angle(a);

    b2Vec2 p = this->get_position();
    this->set_position(p.x, p.y);
}

void
rail::set_position(float x, float y, uint8_t frame/*=0*/)
{
    b2Vec2 v = b2Vec2(x,y);

    b2Vec2 d = r_offs[this->railtype];

    if (this->body) d = this->body->GetWorldVector(d);

    v -= d;

    v.x = roundf(v.x);
    v.y = roundf(v.y);

    v += d;

    entity::set_position(v.x, v.y);
}

void
rail::add_to_world()
{
    if (!this->body) {
        b2BodyDef bd;
        bd.type = b2_staticBody;
        bd.position = this->_pos;
        bd.angle = this->_angle;
        b2Body *b = W->b2->CreateBody(&bd);
        this->body = b;
    }

    b2PolygonShape box;
    b2FixtureDef fd;
    fd.shape = &box;
    fd.density = m_metal.density;
    fd.friction = m_metal.friction;
    fd.restitution = m_metal.restitution;
    fd.filter = world::get_filter_for_layer(this->prio, 6);

    b2Vec2 verts[4];

    switch (this->railtype) {
        default:case RAIL_STRAIGHT:
            box.SetAsBox(2.f, .125f); 
            (this->body->CreateFixture(&fd))->SetUserData(this);
            break;
        case RAIL_SKEWED:
            verts[0] = b2Vec2(2.f, 1.f-.125f);
            verts[1] = b2Vec2(2.f, 1.f+.125f);
            verts[2] = b2Vec2(-2.f, +.125f);
            verts[3] = b2Vec2(-2.f, -.125f);
            verts[0] -= r_offs[RAIL_SKEWED];
            verts[1] -= r_offs[RAIL_SKEWED];
            verts[2] -= r_offs[RAIL_SKEWED];
            verts[3] -= r_offs[RAIL_SKEWED];
            box.Set(verts, 4);
            (this->body->CreateFixture(&fd))->SetUserData(this);
            break;

        case RAIL_SKEWED2:
            verts[0] = b2Vec2(-2.f, 1.f-.125f);
            verts[1] = b2Vec2(-2.f, 1.f+.125f);
            verts[2] = b2Vec2(2.f, +.125f);
            verts[3] = b2Vec2(2.f, -.125f);
            verts[0] -= r_offs[RAIL_SKEWED2];
            verts[1] -= r_offs[RAIL_SKEWED2];
            verts[2] -= r_offs[RAIL_SKEWED2];
            verts[3] -= r_offs[RAIL_SKEWED2];
            box.Set(verts, 4);
            (this->body->CreateFixture(&fd))->SetUserData(this);
            break;

        case RAIL_45DEG:
            {
                float step = (M_PI/2.f) / 6;
                float offs = - M_PI/2.f;
                for (int x=0; x<6; x++) {
                    float p = offs+step*x;
                    float pn = offs+step*(x+1);

                    b2Vec2 rp = b2Vec2(cosf(p), sinf(p));
                    b2Vec2 rpn = b2Vec2(cosf(pn), sinf(pn));

                    verts[0] = b2Vec2(rp.x * 2.f + rp.x*.125f - 2.f, rp.y*2.f + rp.y*.125f + 2.f);
                    verts[1] = b2Vec2(rp.x * 2.f - rp.x*.125f - 2.f, rp.y*2.f - rp.y*.125f + 2.f);
                    verts[2] = b2Vec2(rpn.x * 2.f + rpn.x*.125f -2.f, rpn.y*2.f + rpn.y*.125f+2.f);
                    verts[3] = b2Vec2(rpn.x * 2.f - rpn.x*.125f -2.f, rpn.y*2.f - rpn.y*.125f+2.f);

                    verts[0] -= r_offs[RAIL_45DEG];
                    verts[1] -= r_offs[RAIL_45DEG];
                    verts[2] -= r_offs[RAIL_45DEG];
                    verts[3] -= r_offs[RAIL_45DEG];
                    box.Set(verts, 4);
                    (this->body->CreateFixture(&fd))->SetUserData(this);
                }
            }
            break;
    }
    this->set_layer(this->prio);
}
