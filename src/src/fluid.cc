#include "fluid.hh"
#include "material.hh"
#include "world.hh"
#include "fluidbuffer.hh"

fluid::fluid()
{
    this->set_flag(ENTITY_IS_BETA, true);
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);
    this->set_flag(ENTITY_IS_LOW_PRIO, true);
    this->set_mesh(const_cast<struct tms_mesh*>(tms_meshfactory_get_square()));
    this->set_material(&m_pv_rgba);
    this->set_uniform("~color", 0.f, 0.f, 0.f, .5f);

    this->set_num_properties(2);
    this->properties[0].type = P_FLT;
    this->properties[0].v.f = 1.f;
    this->properties[1].type = P_FLT;
    this->properties[1].v.f = 1.f;

    this->curr_update_method = this->update_method = ENTITY_UPDATE_STATIC_CUSTOM;

    this->num_sliders = 2;

    this->width = 1.f;
    this->height = 1.f;
}

void
fluid::update()
{
    /* only used in sandbox */
    if (this->body) {
        //tms_infof("rendering with body");
        b2Transform t;
        t = this->body->GetTransform();

        tmat4_load_identity(this->M);
        this->M[0] = t.q.c;
        this->M[1] = t.q.s;
        this->M[4] = -t.q.s;
        this->M[5] = t.q.c;
        this->M[12] = t.p.x;
        this->M[13] = t.p.y;
        this->M[14] = this->prio * LAYER_DEPTH;

        tmat4_scale(this->M, this->get_width()*2.f, this->get_height()*2.f, 1.f);

        this->width = this->get_width();
        this->height = this->get_height();

        tmat3_copy_mat4_sub3x3(this->N, this->M);
    } else {
        tmat4_load_identity(this->M);
        tmat4_translate(this->M, this->_pos.x, this->_pos.y, 0);
        tmat4_rotate(this->M, this->_angle * (180.f/M_PI), 0, 0, -1);
        tmat3_copy_mat4_sub3x3(this->N, this->M);
    }
}

float
fluid::get_slider_snap(int s)
{
    return .1f;
}

float
fluid::get_slider_value(int s)
{
    return this->properties[s].v.f / FLUID_MAX_SIZE;
}

void
fluid::on_slider_change(int s, float value)
{
    this->properties[s].v.f = value*FLUID_MAX_SIZE;

    if (this->body) {
        b2PolygonShape *sh = (b2PolygonShape*)(this->body->GetFixtureList()->GetShape());

        sh->SetAsBox(this->get_width(), this->get_height());
    }
}

void
fluid::add_to_world()
{
    if (!W->is_paused()) {
        b2ParticleGroupDef pd;
        W->b2->SetParticleRadius(.2f);
        W->b2->SetParticleDamping(.35f);
        W->b2->SetParticleDensity(1.0f);
        W->b2->SetParticleMaxCount((W->level.version <= LEVEL_VERSION_1_5_1) ? FLUIDBUFFER_MAX_1_5_1 : FLUIDBUFFER_MAX);
        pd.flags = ((2+4) << (16+4*this->get_layer()));
        //pd.flags = b2_waterParticle | (15 << (16+4*this->get_layer()));
        //pd.flags = b2_elasticParticle | (15 << (16+4*this->get_layer()));
        pd.userData = (void*)(uintptr_t)this->get_layer();

        b2PolygonShape shape;
        shape.SetAsBox(this->get_width(), this->get_height(), this->get_position(), this->get_angle());
        pd.shape = &shape;
        W->b2->CreateParticleGroup(pd);
    } else {
        this->create_rect(b2_dynamicBody, this->get_width(), this->get_height(), this->get_material(), 0);
        this->body->GetFixtureList()->SetSensor(true);
    }

    this->width = this->get_width();
    this->height = this->get_width();
}
