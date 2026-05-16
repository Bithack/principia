#include "shape_extruder.hh"
#include "entity.hh"
#include "game.hh"
#include "model.hh"
#include "ui.hh"

shape_extruder::shape_extruder() {
    this->set_flag(ENTITY_IS_BETA,              true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_ALLOW_ROTATION,       false);
    this->set_flag(ENTITY_HAS_CONFIG,           true);

    this->dialog_id = DIALOG_SHAPEEXTRUDER;

    this->set_mesh(mesh_factory::get_mesh(MODEL_BOX0));
    this->set_material(&m_red);
    this->layer_mask = 1;
    this->update_method = ENTITY_UPDATE_CUSTOM;

    this->set_num_properties(4);

    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 0.f; /* extra right */

    this->properties[1].type = P_FLT;
    this->properties[1].v.f = 0.f; /* extra up */

    this->properties[2].type = P_FLT;
    this->properties[2].v.f = 0.f; /* extra left */

    this->properties[3].type = P_FLT;
    this->properties[3].v.f = 1.f; /* extra down */

    this->c.init_owned(0, this);
    this->c.type = CONN_CUSTOM;
}

connection *shape_extruder::load_connection(connection &conn) {
    this->c = conn;
    return &this->c;
}

void shape_extruder::update() {
    if (G && !W->is_paused()) {
		// Hide the object when in-game
        memset(this->M, 0, sizeof(float)*16);
		return;
    }

	entity *ths = static_cast<entity*>(this);

	if (ths->body) {
		b2Transform t;
		t = ths->body->GetTransform();

		tmat4_load_identity(ths->M);
		ths->M[0] = t.q.c;
		ths->M[1] = t.q.s;
		ths->M[4] = -t.q.s;
		ths->M[5] = t.q.c;
		ths->M[12] = t.p.x;
		ths->M[13] = t.p.y;
		ths->M[14] = ths->prio * LAYER_DEPTH;

		tmat3_copy_mat4_sub3x3(ths->N, ths->M);
	} else {
		//tms_infof("rendering with _pos");
		tmat4_load_identity(ths->M);
		tmat4_translate(ths->M, ths->_pos.x, ths->_pos.y, 0);
		tmat4_rotate(ths->M, ths->_angle * (180.f/M_PI), 0, 0, -1);
		tmat3_copy_mat4_sub3x3(ths->N, ths->M);
		/* XXX rotate */
	}
}

void shape_extruder::connection_create_joint(connection *c) {
    c->j = (b2Joint*)1;
    c->max_force = INFINITY;

    if (W->is_paused()) {
        b2WeldJointDef wjd;
        wjd.localAnchorA = c->e->local_to_body(b2Vec2(0.f, 0.f), c->f[0]);
        wjd.localAnchorB = c->o->local_to_body(b2Vec2(0.f, 0.f), c->f[1]);

        wjd.bodyA = c->e->get_body(c->f[0]);
        wjd.bodyB = c->o->get_body(c->f[1]);
        wjd.referenceAngle = c->o->get_body(c->f[1])->GetAngle() - c->e->get_body(c->f[0])->GetAngle();
        wjd.collideConnected = false;
        wjd.frequencyHz = 0.f;

        c->j = W->b2->CreateJoint(&wjd);
    }
}

bool shape_extruder::connection_destroy_joint(connection *c) {
    return !(c->j != (void*)1 && c->j != 0);
}

bool shape_extruder::ReportFixture(b2Fixture *f) {
    entity *e = static_cast<entity*>(f->GetUserData());

    if (e && e != this) {
        uint8_t frame = (uint8_t)(uintptr_t)(f->GetBody()->GetUserData());
        if (this->c.pending && frame == 0 && e->get_body(frame)->GetFixtureList()[0].TestPoint(this->get_position())
                    && e->flag_active(ENTITY_IS_COMPOSABLE)
            && e->get_layer() == this->get_layer()) {
            this->c.type = CONN_CUSTOM;
            this->c.o = e;
            this->c.p = e->get_position();
            this->c.o_data = e->get_fixture_connection_data(f);
            G->add_pair(this, e, &this->c);
        }
    }

    return true;
}

void shape_extruder::init() {
    //this->create_rect(w, b2_dynamicBody, .5f, .5, this->material);

    if (this->conn_ll && this->c.o && this->c.o->flag_active(ENTITY_IS_COMPOSABLE)) {
        composable *other = static_cast<composable*>(this->c.o);

        if (!other)
			return;

		float w = other->get_width();
		float h = other->height;

		b2Filter f = other->fx->GetFilterData();
		f.groupIndex = 1+this->get_layer();
		other->fx->SetFilterData(f);
		//other->fx->SetSensor(true);

		b2PolygonShape sh;

		b2Vec2 vertices[4] = {
			other->local_to_body(b2Vec2(w+this->properties[0].v.f, h+this->properties[1].v.f), 0),
			other->local_to_body(b2Vec2(-(w+this->properties[2].v.f), h+this->properties[1].v.f), 0),
			other->local_to_body(b2Vec2(-(w+this->properties[2].v.f), -(h+this->properties[3].v.f)), 0),
			other->local_to_body(b2Vec2(w+this->properties[0].v.f, -(h+this->properties[3].v.f)), 0)
		};

		sh.Set(vertices, 4);

		b2FixtureDef fd;
		fd.shape = &sh;
		fd.density = 0.00001f;
		fd.friction = other->fd.friction;
		fd.restitution = other->fd.restitution;
		fd.filter = other->fd.filter;
		fd.filter.groupIndex = -(1+this->get_layer());

		other->get_body(0)->CreateFixture(&fd)->SetUserData(other);
    }
}

void shape_extruder::find_pairs() {
    b2AABB aabb;
    b2Vec2 p = this->get_position();
    aabb.lowerBound = b2Vec2(p.x-.1f, p.y-.1f);
    aabb.upperBound = b2Vec2(p.x+.1f, p.y+.1f);
    W->b2->QueryAABB(this, aabb);
}

void shape_extruder::add_to_world() {
    if (W->is_paused()) {
        this->create_rect(b2_dynamicBody, .25f, .25f, this->material);
        this->body->GetFixtureList()[0].SetSensor(true);
    }
}
