#include "screenshot_marker.hh"
#include "world.hh"
#include "material.hh"
#include "model.hh"
#include "game.hh"
#include "ui.hh"

#define CABLE_Z .375f

screenshot_marker::screenshot_marker()
{
    this->set_flag(ENTITY_IS_HIGH_PRIO,         true);
    this->set_flag(ENTITY_ALLOW_ROTATION,       false);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_DISABLE_LAYERS,       true);
    this->set_flag(ENTITY_CUSTOM_GHOST_UPDATE,  true);
    this->set_flag(ENTITY_HAS_CONFIG,           true);

    this->set_mesh(mesh_factory::get_mesh(MODEL_SPHERE));
    this->set_material(&m_pv_colored);
    this->set_uniform("~color", .7f, .35f, .35f, 1.f);

    this->update_method = ENTITY_UPDATE_CUSTOM;

    /* XXX: A string property for the name of the camera? */
    this->set_num_properties(1);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 10.f;

    this->num_sliders = 1;

    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);

    this->hidden = false;
}

void
screenshot_marker::add_to_world()
{
    if (W->is_paused()) {
        b2BodyDef bd;
        bd.type = b2_staticBody;
        bd.position = _pos;
        bd.fixedRotation = true;
        bd.angle = 0.f;

        b2CircleShape shape;
        shape.m_radius = .125f;

        b2FixtureDef fd;
        fd.shape = &shape;
        fd.filter = world::get_filter_for_layer(this->get_layer(), 16); /* this should make it collide with nothing */
        //fd.isSensor = true;

        b2Body *b = W->b2->CreateBody(&bd);
        (b->CreateFixture(&fd))->SetUserData(this);

        this->body = b;
        this->set_layer(2);
    }

    W->cam_markers.insert(std::make_pair(this->id, this));
}

void
screenshot_marker::update()
{
    b2Vec2 p = this->get_position();
    float a = this->get_angle() + M_PI/2.f;

    float c,s;
    tmath_sincos(a, &s, &c);

    this->M[0] = c;
    this->M[1] = s;
    this->M[4] = -s;
    this->M[5] = c;
    this->M[12] = p.x;
    this->M[13] = p.y;
    this->M[14] = this->prio * LAYER_DEPTH + CABLE_Z;

    tmat3_copy_mat4_sub3x3(this->N, this->M);
}

void
screenshot_marker::setup()
{
    this->hide();
}

void
screenshot_marker::on_pause()
{
    this->show();
}

void
screenshot_marker::hide()
{
    if (this->is_hidden()) return;
    this->saved_position = get_position();
    this->set_position(HIDDEN_X, HIDDEN_Y);
    this->hidden = true;
}

void
screenshot_marker::show()
{
    if (!this->is_hidden()) return;
    this->set_position(this->saved_position.x, this->saved_position.y);
    this->hidden = false;
}

void
screenshot_marker::on_slider_change(int s, float value)
{
    float zoom = (value * 56.f) + 4.f;
    this->set_property(0, zoom);
    G->show_numfeed(zoom);
}

camtargeter::camtargeter()
{
    this->set_flag(ENTITY_HAS_TRACKER,  true);
    this->set_flag(ENTITY_HAS_CONFIG,   true);

    this->dialog_id = DIALOG_CAMTARGETER;

    this->set_num_properties(5);
    this->properties[0].type = P_ID; /* id of object */
    this->properties[0].v.i = 0;

    /**
     * set mode:
     *   0 = smoothly move camera to object
     *   1 = snap camera to object
     *   2 = follow object based on current camera position
     *   3 = linear follow
     **/
    this->properties[1].type = P_INT8;
    this->properties[1].v.i8 = 0;

    /**
     * offset type:
     *   0 = global offset
     *   1 = relative offset
     **/
    this->properties[2].type = P_INT8;
    this->properties[2].v.i8 = 0;

    /* x offset */
    this->properties[3].type = P_FLT;
    this->properties[3].v.f = 0.f;

    /* y offset */
    this->properties[4].type = P_FLT;
    this->properties[4].v.f = 0.f;
}

edevice*
camtargeter::solve_electronics()
{
    if (!this->s_in[0].is_ready()) return this->s_in[0].get_connected_edevice();

    if (this->s_in[0].p == 0 || (bool)roundf(this->s_in[0].get_value())) {
        if (this->properties[0].v.i != 0) {
            if ((G->follow_object && G->follow_object->id != this->properties[0].v.i)
                || !G->follow_object) {
                entity *e = W->get_entity_by_id(this->properties[0].v.i);
                if (e) {
                    bool snap = this->properties[1].v.i8 == 1 || this->properties[1].v.i8 == 3;
                    bool preserve_pos = this->properties[1].v.i8 == 2;

                    G->set_follow_object(e, snap, preserve_pos);

                    G->follow_options.linear = this->properties[1].v.i8 == 3;
                    G->follow_options.offset_mode = this->properties[2].v.i8;
                    G->follow_options.offset.x = this->properties[3].v.f;
                    G->follow_options.offset.y = this->properties[4].v.f;
                }
            }
        } else {
            G->set_follow_object(0, this->properties[1].v.i8 == 1, this->properties[1].v.i8 == 2);

            G->follow_options.linear = this->properties[1].v.i8 == 3;
            G->follow_options.offset_mode = this->properties[2].v.i8;
            G->follow_options.offset.x = this->properties[3].v.f;
            G->follow_options.offset.y = this->properties[4].v.f;
        }
    }

    return 0;
}

zoomer::zoomer()
{
    this->num_sliders = 2;

    this->set_num_properties(2);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 10.f;

    /**
     * interpolation coeffecient
     * 1 = snap
     **/
    this->properties[1].type = P_FLT;
    this->properties[1].v.f = 1.f;
}

edevice*
zoomer::solve_electronics()
{
    if (!this->s_in[0].is_ready()) return this->s_in[0].get_connected_edevice();

    if (this->s_in[0].p == 0 || (bool)roundf(this->s_in[0].get_value())) {
#ifndef SCREENSHOT_BUILD
        G->cam->_position.z = this->properties[0].v.f * (this->properties[1].v.f)
                                    + G->cam->_position.z * (1.f - this->properties[1].v.f);
#endif
    }

    return 0;
}

float zoomer::get_slider_value(int s)
{
    if (s == 0)
        return ((this->properties[0].v.f - 4.f) / 56.f);
    else
        return 1.f-(this->properties[1].v.f+.025f);
}

void
zoomer::on_slider_change(int s, float value)
{
    if (s == 0) {
        float zoom = (value * 56.f) + 4.f;
        this->set_property(0, zoom);
        G->show_numfeed(zoom);
    } else {
        this->properties[1].v.f = .025f + (1.f - value)*.875f;
        G->show_numfeed(this->properties[1].v.f);
    }
}

