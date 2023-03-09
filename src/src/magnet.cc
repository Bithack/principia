#include "magnet.hh"
#include "model.hh"
#include "material.hh"
#include "world.hh"

magnet::magnet(int type)
{
    this->type = type;
    this->set_mesh(mesh_factory::get_mesh(MODEL_MAGNET));
    this->set_material(&m_magnet);
    this->set_uniform("~color", .2f, .2f, .2f, 1.f);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->width = 1.0f;
    this->height = .5f;
    this->set_flag(ENTITY_IS_DEV, true);
    this->set_flag(ENTITY_DO_STEP, true);
    this->strength = 400.f;

    switch (type) {
        case 0:
            this->strength_mul = 1.f;
            break;

        case 1:
            this->strength_mul = 1.f;
            this->num_s_in = 1;
            this->s_in[0].lpos = b2Vec2(-.5f, .25f);
            this->s_in[0].ctype = CABLE_BLACK;
            break;
    }
}

void
magnet::recreate_shape()
{
    if (!this->body) {
        b2BodyDef bd;
        bd.type = this->get_dynamic_type();
        bd.position = _pos;
        bd.angle = _angle;

        b2Body *b = W->b2->CreateBody(&bd);
        this->body = b;
    } else {
        while (this->body->GetFixtureList()) {
            this->body->DestroyFixture(this->body->GetFixtureList());
        }
    }

    b2PolygonShape bd_shape;
    bd_shape.SetAsBox(.5f, .25f);

    b2FixtureDef bd_fd;
    bd_fd.shape = &bd_shape;
    bd_fd.density = .5f;
    bd_fd.friction = 0.3f;
    bd_fd.restitution = .0f;
    bd_fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

    (this->body->CreateFixture(&bd_fd))->SetUserData(this);

    if (!W->is_paused()) {
        b2PolygonShape shape;
        shape.SetAsBox(2.f, 2.f);

        b2FixtureDef mag_a_fd;
        mag_a_fd.shape = &shape;
        mag_a_fd.density = .0f;
        mag_a_fd.isSensor = true;
        mag_a_fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

        this->sensor = this->body->CreateFixture(&mag_a_fd);
        this->sensor->SetUserData(this);
    }
}

void
magnet::add_to_world()
{
    this->active = true;
    this->objects.clear();
    this->recreate_shape();
}

void
magnet::step()
{
    if (this->active) {
        object_map_iter m_it;
        object_map_iter s_it;

        float mult = 1.f / objects.size();

        for (m_it = objects.begin(); m_it != objects.end();) {
            entity *e = static_cast<entity*>((*m_it).first);
            b2Fixture *f = static_cast<b2Fixture*>((*m_it).second);

            int num_fixtures = objects.count(e);
            if (num_fixtures > 1) {
                /* special handling for objects that have multiple fixtures */
                std::pair<object_map_iter, object_map_iter> key_range = objects.equal_range(e);
                b2Fixture *k_f;
                float closest = INFINITY;
                b2Fixture *closest_fixture = 0;

                for (s_it = key_range.first; s_it != key_range.second; ++s_it) {
                    b2Fixture *k_f = static_cast<b2Fixture*>((*s_it).second);

                    float dist = entity::distance_to_fixture(this->get_position(), k_f);
                    this->apply_magnetism(k_f, dist, mult);

                    if (!closest_fixture || dist < closest) {
                        closest = dist;
                        closest_fixture = k_f;
                    }
                }

                //this->apply_magnetism(closest_fixture, closest);
                m_it = s_it;
            } else {
                this->apply_magnetism(f, entity::distance_to_fixture(this->get_position(), f), mult);
                ++m_it;
            }
        }
    }
}

void
magnet::on_touch(b2Fixture *my, b2Fixture *other)
{
    entity *e = (entity*)other->GetUserData();
    if (other->IsSensor())
        return;
    if (e && e != this && e->flag_active(ENTITY_IS_MAGNETIC)) {
        objects.insert(std::pair<entity*, b2Fixture*>(e, other));
        other->m_isHighlighted = true;
    }
}

void
magnet::on_untouch(b2Fixture *my, b2Fixture *other)
{
    entity *e = (entity*)other->GetUserData();
    if (other->IsSensor()) return;
    if (e && e != this && e->flag_active(ENTITY_IS_MAGNETIC)) {
        object_map_iter it;
        std::pair<object_map_iter, object_map_iter> key_range = objects.equal_range(e);

        for (it = key_range.first; it != key_range.second; ++it) {
            if ((*it).second == other) {
                objects.erase(it);
                other->m_isHighlighted = false;
                return;
            }
        }
    }
}

edevice*
magnet::solve_electronics()
{
    switch (this->type) {
        case 1:
            if (!this->s_in[0].is_ready())
                return this->s_in[0].get_connected_edevice();

            float v = this->s_in[0].get_value();
            if (v <= .0f) {
                this->active = false;
            } else {
                this->strength_mul = v/2.f;
                tms_infof("strenght mul: %.2f", this->strength_mul);
                this->active = true;
            }
            break;
    }

    return 0;
}

void
magnet::apply_magnetism(b2Fixture *f, float dist, float multiplier)
{
    entity *e = (entity*)f->GetUserData();
    if (!e) return;

    tvec2 _nearest = entity::get_nearest_point(this->get_position(), f);
    b2Vec2 nearest = b2Vec2(_nearest.x, _nearest.y);

    b2Vec2 vec = nearest - this->get_position();

    b2Vec2 local = this->body->GetLocalVector(vec);
    float a = atan2f(local.y, local.x);

    if (local.x > 0.f) {
        a = M_PI/2.f - fabsf(tmath_adist(a, M_PI/2.f)*2.f);
    } else
        a = M_PI/2.f + fabsf(tmath_adist(a, M_PI/2.f)*2.f);

    vec = b2Vec2(cosf(a), sinf(a));

    /*
    if (local.x > 0.f) {
        vec.y = -vec.y;
    }
    */

    vec = this->body->GetWorldVector(vec);

    /* TODO: apply force on both objects */
    /* use <----------------- code */

    if (dist <= 1.f) {
        dist = 1.f;
    }

    //dist *= 2.f;

    float force = 1.f/dist;

    force = powf(force, 4.f);

    vec *= -force*this->strength * this->strength_mul * multiplier;

    f->GetBody()->ApplyForce(vec, nearest);

    /*
    b2Vec2 vel = f->GetBody()->GetLinearVelocity();
    float a = f->GetBody()->GetAngularVelocity();
    vel *= .95f;
    a   *= .95f;
    f->GetBody()->SetLinearVelocity(vel);
    f->GetBody()->SetAngularVelocity(a);
    */

    b2Vec2 ml = this->get_body(0)->GetLocalPoint(nearest);
    if (ml.x < -.5f) ml.x = -.5f;
    if (ml.x > .5f) ml.x = .5f;
    ml.y = 0;

    this->get_body(0)->ApplyForce(-vec, this->get_body(0)->GetWorldPoint(ml));
}
