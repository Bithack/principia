#include "dragfield.hh"
#include "material.hh"
#include "model.hh"
#include "spritebuffer.hh"
#include "world.hh"

dragfield::dragfield()
{
    this->num_sliders = 1;
    //this->menu_scale = .25f;
    this->update_method = ENTITY_UPDATE_CUSTOM;

    this->set_mesh(mesh_factory::get_mesh(MODEL_DRAGFIELD));
    this->set_material(&m_edev);

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->width = .75f/2.f;
    this->height = .75f/2.f;

    this->set_num_properties(1);
    this->properties[0].type = P_FLT;
    this->set_property(0, 1.f); /* radius */

    this->set_flag(ENTITY_DO_UPDATE_EFFECTS, true);

    this->c.init_owned(0, this);
    this->c.type = CONN_PLATE;

    /* create light entity */
    {
        struct tms_entity *e = tms_entity_alloc();
        tms_entity_set_mesh(e, mesh_factory::get_mesh(MODEL_CYLINDER05));
        tms_entity_set_uniform4f(e, "~color", 1.f, 1.f, 1.f, 1.0f);
        tms_entity_set_material(e, &m_field);
        tms_entity_add_child(this, e);
        //
        this->lamp = e;

        tmat3_load_identity(e->N);
    }

    float qw = this->width/2.f + 0.15f;
    float qh = this->height/2.f + 0.15f;
    this->query_sides[0].Set(0.f,  qh); /* up */
    this->query_sides[1].Set(-qw, 0.f); /* left */
    this->query_sides[2].Set(0.f, -qh); /* down */
    this->query_sides[3].Set( qw, 0.f); /* right */

    this->num_inside = 0;
}

dragfield::~dragfield()
{
    //tms_entity_free(lamp);
}

void
dragfield::init()
{
    this->num_inside = 0;
}

void
dragfield::update_effects()
{
    if (this->num_inside) {
        b2Vec2 p = this->get_position();
        spritebuffer::add(p.x, p.y, this->get_layer()*LAYER_DEPTH+.5f,
                0.8f, 1.2f, 0.8f, .225f,
                2,2, 2);
    }
}

void
dragfield::update()
{
    entity_fast_update(this);

    b2Vec2 p = this->get_position();
    tmat4_load_identity(lamp->M);
    tmat4_translate(lamp->M, p.x, p.y, this->get_layer()*LAYER_DEPTH+.05f);

    if (this->num_inside)
        tms_entity_set_uniform4f(this->lamp, "~color", .9f, 1.2f, 0.9f, 1.f);
    else
        tms_entity_set_uniform4f(this->lamp, "~color", 0.4f, 0.4f, 0.4f, 1.f);
}

float
dragfield::get_slider_value(int s)
{
    return this->properties[0].v.f / 5.f;
}

float
dragfield::get_slider_snap(int s)
{
    return .1f;
}

void
dragfield::on_slider_change(int s, float value)
{
    float radius = value * 5.f;

    this->set_size(radius);
}

void
dragfield::add_to_world()
{
    float radius = this->properties[0].v.f;

    this->set_size(radius);

    b2BodyDef bd;
    bd.type = this->get_dynamic_type();
    bd.position = _pos;
    bd.angle = _angle;
    b2Body *b = W->b2->CreateBody(&bd);
    this->body = b;

    b2PolygonShape box_shape;
    box_shape.SetAsBox(.75f/2.f, .75f/2.f);

    b2FixtureDef fd;
    fd.shape = &box_shape;
    fd.density = 1.f;
    fd.friction = .5f;
    fd.restitution = .3f;
    fd.filter = world::get_filter_for_layer(this->get_layer(), 6);

    (this->body->CreateFixture(&fd))->SetUserData(this);

    if (!W->is_paused()) {
        b2FixtureDef sfd;
        sfd.shape = &this->sensor_shape;
        sfd.density = .0f;
        sfd.isSensor = true;
        sfd.filter = world::get_filter_for_layer(this->get_layer(), 15);

        (this->body->CreateFixture(&sfd))->SetUserData(this);
    }
}

void
dragfield::on_touch(b2Fixture *my, b2Fixture *other)
{
    entity *e = static_cast<entity*>(other->GetUserData());

    if (e) {
        if (e->flag_active(ENTITY_IS_INTERACTIVE)) {
            tms_infof("touched an interactive entity %p", other);
            ++e->in_dragfield;

            this->num_inside ++;
        }
    }
}

void
dragfield::on_untouch(b2Fixture *my, b2Fixture *other)
{
    entity *e = static_cast<entity*>(other->GetUserData());

    if (e) {
        if (e->flag_active(ENTITY_IS_INTERACTIVE)) {
            tms_infof("untouched an interactive entity %p", other);
            --e->in_dragfield;
            this->num_inside --;
            if (this->num_inside < 0) this->num_inside = 0;
        }
    }
}

void
dragfield::set_size(float radius)
{
    if (radius>5.f) radius=5.f;
    if (radius<0.f) radius=0.f;

    this->set_property(0, radius);

    sensor_shape.m_radius = (radius+1.f);
}
