#include "proximitysensor.hh"
#include "material.hh"
#include "world.hh"
#include "model.hh"
#include "game.hh"

proximitysensor::proximitysensor()
{
    this->set_flag(ENTITY_IS_MAGNETIC, true);
    this->set_mesh(mesh_factory::get_mesh(MODEL_PROXIMITY));
    this->set_material(&m_metal);
    //this->set_uniform("~color", 1.f, 0.f, 1.f, 1.f);

    this->num_sliders = 2;

    this->set_num_properties(2);
    this->set_property(0, 1.f);
    this->set_property(1, 1.f);

    this->num_s_out = 1;

    this->s_out[0].lpos = b2Vec2(.25f, 0.f);

    this->layer_mask = 15;
    this->set_flag(ENTITY_MUST_BE_DYNAMIC, true);

    this->height = .125f;
    this->width = .5f;
    float qw = (this->width  / 2.0f) + 0.35f;
    float qh = (this->height / 2.0f) + 0.15f;
    this->query_sides[0].Set(0.f,  qh); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, 0.f); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */

    for (int x=0; x<4; x++) {
        this->c_side[x].render_type = CONN_RENDER_SMALL;
    }
}

float
proximitysensor::get_slider_snap(int s)
{
    if (s == 0) {
        return 1.f / 17.f;
    } else {
        return 1.f / 12.f;
    }
}

float
proximitysensor::get_slider_value(int s)
{
    if (s == 0) {
        return (this->properties[0].v.f - 1.f) / 17.f;
    } else {
        return this->properties[1].v.f / 1.2f;
    }
}

void
proximitysensor::on_slider_change(int s, float value)
{
    if (s == 0) {
        float v = (value * 17.f) + 1.f;
        this->properties[0].v.f = v;
        G->show_numfeed(v);
    } else {
        float v = value * 1.2f;
        this->properties[1].v.f = v;
        G->show_numfeed(v);
    }
    this->calculate_sensor();
}

void
proximitysensor::on_load(bool created, bool has_state)
{
    this->calculate_sensor();
}

void
proximitysensor::add_to_world()
{
    b2BodyDef bd;
    bd.type = this->get_dynamic_type();
    bd.position = _pos;
    bd.angle = _angle;

    b2PolygonShape box;
    box.SetAsBox(.5f, .125f);

    b2FixtureDef fd;
    fd.shape = &box;
    fd.density = this->get_material()->density; /* XXX */
    fd.friction = this->get_material()->friction;
    fd.restitution = this->get_material()->restitution;
    fd.filter = world::get_filter_for_layer(this->get_layer(), 15);

    b2Body *b = W->b2->CreateBody(&bd);
    (b->CreateFixture(&fd))->SetUserData(this);

    calculate_sensor();

    if (!W->is_paused()) {
        b2FixtureDef sfd;
        sfd.shape = &this->sensor_shape;
        sfd.density = .0f;
        sfd.isSensor = true;
        sfd.filter = world::get_filter_for_layer(this->get_layer(), 15);

        (b->CreateFixture(&sfd))->SetUserData(this);
    }

    this->body = b;
}

void
proximitysensor::on_touch(b2Fixture *my, b2Fixture *other)
{
    if (!other->IsSensor()) {
        tms_infof("prox %p touched %p", this, other);
        objects_detected.push_back(other);
    } else {
        tms_infof("prox %p touched %p which is a sensor, why", this, other);
    }
}

void
proximitysensor::on_untouch(b2Fixture *my, b2Fixture *other)
{
    if (!other->IsSensor()) {
        tms_infof("prox %p untouched %p", this, other);
        objects_detected.remove(other);
        other->m_isHighlighted = false;
    }
}

edevice*
proximitysensor::solve_electronics()
{
    float closest = .0f;

    float max = (this->properties[0].v.f);

    for (std::list<b2Fixture*>::iterator it = objects_detected.begin(); it != objects_detected.end(); ++it) {
        float dist = INFINITY;
        float norm = INFINITY;

        b2Vec2 query_point = this->get_position();
        const b2Shape *shape = (*it)->GetShape();
        if (!shape) continue;
        switch (shape->GetType()) {
            case b2Shape::e_circle:
                {
                b2Vec2 p = (*it)->GetBody()->GetWorldPoint(((b2CircleShape*)(*it)->GetShape())->m_p);
                p = query_point - p;
                dist = p.Length() - ((b2CircleShape*)(*it)->GetShape())->m_radius;
                }
                break;

            case b2Shape::e_polygon:;
                {
                b2Vec2 *ov = (((b2PolygonShape*)(*it)->GetShape())->m_vertices);
                int num_verts = ((b2PolygonShape*)(*it)->GetShape())->m_count;

                tvec2 verts[num_verts];
                for (int x=0; x<num_verts; x++) {
                    b2Vec2 tv = (*it)->GetBody()->GetWorldPoint(ov[x]);
                    verts[x].x = tv.x;
                    verts[x].y = tv.y;
                }

                dist = tintersect_point_poly_distance((tvec2*)&(query_point), verts, num_verts);
                }
                break;

            default:
                tms_warnf("Unknown b2 shape! %d", shape->GetType());
                break;
        }

        norm = 1.f-(tclampf(std::abs(dist)/max, 0.f, 1.f));

        if (norm > closest) {
            closest = norm;
        }
    }

    this->s_out[0].write(closest);

    return 0;
}

void
proximitysensor::calculate_sensor()
{
    static float eps = .05f;
    tvec2 a1, a2, b1, b2, point;
    tvec2 vec = tvec2f(
        cos(M_PI+(M_PI/4)+(M_PI/4)*(1.f-this->properties[1].v.f)),
        sin(M_PI+(M_PI/4)+(M_PI/4)*(1.f-this->properties[1].v.f))
    );
    float h = this->properties[0].v.f;

    b2Vec2 vertices[4];
    vertices[0] = b2Vec2(-(this->width)+eps, -.125f);

    if (this->properties[1].v.f < .05f) {
        vertices[1] = b2Vec2(-(this->width)+eps, -.125f-h);
    } else {
        a1 = (tvec2){vertices[0].x, vertices[0].y};
        a2 = (tvec2){a1.x+vec.x, a1.y+vec.y};
        b1 = (tvec2){0.f, a1.x-h};
        b2 = (tvec2){1.f, a1.x-h};
        tintersect_lines(a1, a2, b1, b2, &point);
        vertices[1] = b2Vec2(point.x, point.y);
    }

    vertices[2] = b2Vec2(+(this->width)-eps, -.125f);

    if (this->properties[1].v.f < .05f) {
        vertices[3] = b2Vec2((this->width)-eps, -.125f-h);
    } else {
        a1 = (tvec2){vertices[0].x, vertices[0].y};
        a2 = (tvec2){a1.x+vec.x, a1.y+vec.y};
        b1 = (tvec2){0.f, a1.x-h};
        b2 = (tvec2){1.f, a1.x-h};
        tintersect_lines(a1, a2, b1, b2, &point);
        vertices[3] = b2Vec2(-point.x, point.y);
    }

    this->sensor_shape.Set(vertices, 4);

    /* debugging angle
    float dx,dy;
    dx = std::abs(vertices[0].x-vertices[1].x);
    dy = std::abs(vertices[0].y-vertices[1].y);
    tms_infof("dx: %.2f    dy: %.2f", dx, dy);
    dx = std::abs(vertices[2].x-vertices[3].x);
    dy = std::abs(vertices[2].y-vertices[3].y);
    tms_infof("dx: %.2f    dy: %.2f", dx, dy);
    */
}
