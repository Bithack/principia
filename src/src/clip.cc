#include "clip.hh"
#include "model.hh"
#include "material.hh"
#include "world.hh"

clip::clip(int _clip_type)
    : clip_type(_clip_type)
{
    this->set_flag(ENTITY_DO_UPDATE, false);
    this->set_flag(ENTITY_DO_STEP, true);

    this->num_s_in = 1;
    this->num_s_out = 1;

    this->s_in[0].lpos = b2Vec2(0.f, -.25f);
    this->s_out[0].lpos = b2Vec2(0.f, .25f);

    this->set_material(&m_iomisc);
    switch (this->clip_type) {
        case CLIP_INTERFACE:
            this->set_mesh(mesh_factory::get_mesh(MODEL_CLIP));

            this->s_in[0].ctype = CABLE_BLUE;
            this->s_out[0].ctype = CABLE_BLUE;
            this->s_in[0].angle =  -M_PI;
            this->s_out[0].angle = -M_PI;

            this->do_solve_electronics = false;
            this->set_as_rect( .25f/2.f, 1.2f/2.f);
            break;

        case CLIP_SIGNAL:
            this->set_mesh(mesh_factory::get_mesh(MODEL_CCLIP));

            this->s_in[0].ctype = CABLE_RED;
            this->s_in[0].lpos = b2Vec2(0.f, -.125f);
            this->s_out[0].ctype = CABLE_RED;
            this->s_out[0].lpos = b2Vec2(0.f, .125f);

            this->do_solve_electronics = true;
            this->set_as_rect( .25f/2.f, .5f/2.f);
            break;
    }

    this->menu_scale = 1.75f;

    if (W->level.version < LEVEL_VERSION_1_5_1) {
        this->layer_mask = 15;
    }

    this->c.init_owned(0, this);
    this->c.type = CONN_GROUP;
}

edevice*
clip::solve_electronics(void)
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();

    this->s_out[0].write(this->s_in[0].get_value());

    return 0;
}
