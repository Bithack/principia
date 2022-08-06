#include "composable.hh"
#include "group.hh"
#include "world.hh"
#include "game.hh"

void
composable::set_position(float x, float y, uint8_t frame/*=0*/)
{
    //tms_debugf("composable::set_position(%.2f, %.2f)", x, y);

    if (!this->flag_active(ENTITY_CAN_MOVE))
        return;

    if (this->gr) {
        /* we're attached to a group, move the group body */
        b2Vec2 offs = this->get_position() - this->gr->body->GetPosition();

        this->gr->set_position(x - offs.x, y - offs.y);
        return;
    }

    if (this->body) {
        /*
        b2BodyType bkp = this->body->GetType();
        this->body->SetType(b2_staticBody);
        this->body->SetActive(true);
        this->body->SetAwake(true);
        */
        this->body->SetTransform(b2Vec2(x,y), this->body->GetAngle());

        /*for (b2Fixture *f = this->body->GetFixtureList(); f; f=f->GetNext())
            f->Refilter();*/

        //this->body->SetType(bkp);
    } else {
        this->_pos = b2Vec2(x,y);
    }
}

b2Vec2
composable::get_position()
{
    if (this->body) {
        return this->body->GetPosition();
    } else if (this->gr && this->gr->body) {
        return this->gr->body->GetWorldPoint(this->_pos);
    } else {
        return this->_pos;
    }
}

float
composable::get_angle()
{
    if (this->body) {
        return this->body->GetAngle();
    } else if (this->gr && this->gr->get_body(0)) { /* XXX: All groups should have bodies! */
        return this->gr->body->GetAngle()+this->_angle;
    } else
        return this->_angle;
}

void
composable::set_angle(float a)
{
    entity::set_angle(a);
}

b2Vec2
composable::local_vector_to_body(b2Vec2 p, uint8_t frame)
{
    if (this->gr) {
        float c,s;
        tmath_sincos(this->_angle, &s, &c);

        b2Vec2 p2;

        p2.x = p.x*c - p.y*s;
        p2.y = p.x*s + p.y*c;

        return p2;
    }

    return entity::local_to_body(p,frame);
}

b2Vec2
composable::local_to_body(b2Vec2 p, uint8_t frame)
{
    if (this->gr) {
        float c,s;
        tmath_sincos(this->_angle, &s, &c);

        b2Vec2 p2;

        p2.x = p.x*c - p.y*s;
        p2.y = p.x*s + p.y*c;

        p2 += this->_pos;

        return p2;
    }

    return entity::local_to_body(p,frame);
}

b2Vec2
composable::local_to_world(b2Vec2 p, uint8_t frame)
{
    if (this->gr) {
        if (!this->gr->body) {
            //tms_debugf("Group %p has no body!!!!!!!!!!", this->gr);
            /* XXX: Is this unintended? */
            return b2Vec2(0,0);
        }
        b2Vec2 pos = this->gr->body->GetWorldPoint(this->_pos);

        float c,s;
        tmath_sincos(this->_angle + this->gr->body->GetAngle(), &s, &c);

        pos.x += p.x*c - p.y*s;
        pos.y += p.x*s + p.y*c;

        return pos;
    }

    return entity::local_to_world(p, frame);
}

b2Body *
composable::get_body(uint8_t frame)
{
    return this->body ? this->body : (this->gr ? this->gr->body : 0);
}

b2Vec2
composable::world_to_local(b2Vec2 p, uint8_t frame)
{
    if (this->gr) {
        b2Vec2 pos = p;

        b2Vec2 l = this->gr->body->GetLocalPoint(p);

        l -= this->_pos;

        float c,s;
        tmath_sincos(-this->_angle, &s, &c);

        b2Vec2 wp;
        wp.x = l.x*c - l.y*s;
        wp.y = l.x*s + l.y*c;

        return wp;
    }

    return entity::world_to_local(p, frame);
}

void
composable::refresh_poly_shape()
{
    this->active.poly.shape = this->orig.poly.shape;

    this->active.poly.shape.Scale(this->get_scale());

    fd.shape = &this->active.poly.shape;
    fd.density = this->get_material()->density * this->get_density_scale();
    fd.friction = this->get_material()->friction;
    fd.restitution = this->get_material()->restitution;
}

void
composable::refresh_circle_shape()
{
    this->active.circle.shape = this->orig.circle.shape;

    this->active.circle.shape.Scale(this->get_scale());

    fd.shape = &this->active.circle.shape;
    fd.density = this->get_material()->density * this->get_density_scale();
    fd.friction = this->get_material()->friction;
    fd.restitution = this->get_material()->restitution;
}

void
composable::set_as_circle(float r)
{
    this->orig.circle.shape = b2CircleShape();
    this->orig.circle.shape.m_radius = r;
    this->orig.circle.shape.m_p = b2Vec2(0.f, 0.f);

    this->width = r;
    this->height = r;

    this->refresh_circle_shape();
}

void
composable::set_as_poly(b2Vec2 *verts, int num_verts)
{
    this->orig.poly.shape = b2PolygonShape();
    this->orig.poly.shape.Set(verts, num_verts, true);

    this->width = 1.f; /* XXX TODO estimate width and height */
    this->height = 1.f;

    this->refresh_poly_shape();
}

void
composable::set_as_rect(float width, float height)
{
    this->orig.poly.shape = b2PolygonShape();
    this->orig.poly.shape.SetAsBox(width, height);

    this->width = width;
    this->height = height;

    this->refresh_poly_shape();
}

void
composable::set_as_tri(float width, float height)
{
    b2Vec2 verts[3] = {
        b2Vec2(width/2.f, height/2.f),
        b2Vec2(-width/2.f, -height/2.f),
        b2Vec2(width/2.f, -height/2.f),
    };

    this->orig.poly.shape = b2PolygonShape();
    this->orig.poly.shape.Set(verts, 3);

    this->width = width;
    this->height = height;

    this->refresh_poly_shape();
}

void
composable::recreate_fixtures(bool initial)
{

}

void
composable::add_to_world()
{
    b2BodyDef bd;
    bd.type = this->get_dynamic_type();
    bd.position = this->_pos;
    bd.angle = this->_angle;

    this->body = W->b2->CreateBody(&bd);
    this->fd.filter = world::get_filter_for_layer(this->get_layer(), this->layer_mask);
    (this->fx = this->body->CreateFixture(&this->fd))->SetUserData(this);

    this->create_sensor();
}

void
composable::remove_from_world()
{
    if (this->gr) {
        if (this->gr->body && this->fx_sensor) {
            this->gr->body->DestroyFixture(this->fx_sensor);
        }
    } else {
        if (this->body && this->fx_sensor) {
            this->body->DestroyFixture(this->fx_sensor);
        }
    }

    this->fx_sensor = 0;

    entity::remove_from_world();
}

void
composable::create_sensor()
{
    if (W->is_paused()) return;

    float r = this->get_sensor_radius();
    b2Body *b = this->gr ? this->gr->body : this->body;

    if (b && r > 0.f) {
        b2Vec2 p = this->local_to_body(this->get_sensor_offset(), 0);

        b2CircleShape c;
        c.m_radius = r;
        c.m_p = p;

        b2FixtureDef fd;
        fd.isSensor = true;
        fd.restitution = 0.f;
        fd.friction = FLT_EPSILON;
        fd.density = 0.00001f;
        fd.filter = world::get_filter_for_layer(this->get_layer(), this->layer_mask);
        fd.shape = &c;

        (this->fx_sensor = b->CreateFixture(&fd))->SetUserData(this);
    }
}

b2Vec2 ComputeCentroid(const b2Vec2* vs, int32 count);

void
composable::update_shape(b2Vec2 local_pos, float local_angle)
{
    if (this->fd.shape->m_type == b2Shape::e_circle) {
        ((b2CircleShape*)this->fd.shape)->m_p = local_pos;
    } else if (this->fd.shape->m_type == b2Shape::e_polygon) {
        b2PolygonShape *ps = (&this->orig.poly.shape);
        b2PolygonShape *p = ((b2PolygonShape*)this->fd.shape);

        if (p->GetVertexCount() == 3) {
            b2Vec2 verts[3] = {
                b2Vec2(width/2.f, height/2.f),
                b2Vec2(-width/2.f, -height/2.f),
                b2Vec2(width/2.f, -height/2.f),
            };

            b2Vec2 centroid = ComputeCentroid(verts, 3);

            verts[0]-=centroid;
            verts[1]-=centroid;
            verts[2]-=centroid;

            b2Transform xf;
            xf.p = local_pos+centroid;
            xf.q.Set(local_angle);

            verts[0] = b2Mul(xf, verts[0]);
            verts[1] = b2Mul(xf, verts[1]);
            verts[2] = b2Mul(xf, verts[2]);

            p->Set(verts, 3);
        } else {
            p->Set(ps->m_vertices, ps->GetVertexCount());
            p->m_centroid = local_pos;

            b2Transform xf;
            xf.p = local_pos;
            xf.q.Set(local_angle);

            // Transform vertices and normals.
            for (int32 i = 0; i < ps->GetVertexCount(); ++i)
            {
                p->m_vertices[i] = b2Mul(xf, p->m_vertices[i]);
                p->m_normals[i] = b2Mul(xf.q, p->m_normals[i]);
            }

            //p->SetAsBox(this->width, this->height, local_pos, local_angle);
        }

    }
}

void
composable::recreate_shape()
{
    if (this->body) {
        this->body->DestroyFixture(&this->body->GetFixtureList()[0]);
        this->fd.filter = world::get_filter_for_layer(this->get_layer(), this->layer_mask);
        (this->fx = this->body->CreateFixture(&this->fd))->SetUserData(this);
    }
}

connection *
composable_simpleconnect::load_connection(connection &conn)
{
    this->c = conn;
    return &this->c;
}

float32
composable_simpleconnect::ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nor, float32 fraction)
{
    if (f->IsSensor()) {
        return -1.f;
    }

    b2Body *b = f->GetBody();
    entity *e = (entity*)f->GetUserData();

    if (e && e != this && e->allow_connections() && e->get_layer() == this->get_layer() && (e->layer_mask & 6) != 0) {
        this->query_result = e;
        this->query_fx = f;
        this->query_frame = VOID_TO_UINT8(b->GetUserData());
    }

    return -1;
}

void
composable_simpleconnect::find_pairs()
{
    if (this->c.pending) {
        this->query_result = 0;

        W->raycast(this, this->local_to_world(this->query_pt, 0), this->local_to_world(this->query_vec+this->query_pt, 0));

        if (this->query_result) {
            this->c.o = this->query_result;
            this->c.f[1] = this->query_frame;
            this->c.angle = atan2f(this->query_vec.y, this->query_vec.x);
            this->c.o_data = this->c.o->get_fixture_connection_data(this->query_fx);
            b2Vec2 vv = this->query_vec;
            vv*=.5f;
            vv += this->query_pt;
            this->c.p = this->local_to_world(vv, 0);
            G->add_pair(this, this->query_result, &this->c);
        }
    }
}

connection *
composable_multiconnect::load_connection(connection &conn)
{
    this->c_side[conn.o_index] = conn;
    this->c_side[conn.o_index].render_type = CONN_RENDER_SMALL;
    return &this->c_side[conn.o_index];
}

void
composable_multiconnect::find_pairs()
{
    this->sidecheck4(this->c_side);
}

void
composable_multiconnect::set_as_rect(float width, float height)
{
    composable::set_as_rect(width, height);

    const float qw = width/2.f+0.15f;
    const float qh = height/2.f+0.15f;
    this->query_sides[0].Set(0.f,  qh); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, -qh); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */
}
