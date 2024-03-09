#include "simplebg.hh"
#include "model.hh"
#include "material.hh"

simplebg::simplebg()
{
    this->bottom_only = false;
    this->set_mesh(static_cast<tms::mesh*>(const_cast<tms_mesh*>(tms_meshfactory_get_square())));
    this->set_material(&m_bg);

    this->set_flag(ENTITY_DO_UPDATE, false);

    tmat4_load_identity(this->M);
    tmat4_translate(this->M, 0, 0, -1.f);
    tmat4_scale(this->M, 200, 200, .0f);
    this->prio = 0;

    for (int x=0; x<4; x++) {
        this->borders[x] = (tms::entity*)tms_entity_alloc();
        this->borders[x]->prio = 0;
        tms_entity_set_material(this->borders[x], &m_border);
        tms_entity_set_mesh(this->borders[x], mesh_factory::get_mesh(MODEL_BORDER));
        tms_entity_set_uniform4f(this->borders[x], "scale", (x&1) ? .0f : -1.f,  (x&1) ? 1.f : 0.f, 0.f, 0.f);
        tms_entity_add_child((struct tms_entity*)this, this->borders[x]);
    }

    tmat3_load_identity(this->N);
}

void
simplebg::set_repeating(bool repeat)
{
    if (repeat) {
        this->set_material(&m_bg);
    } else {
        this->set_material(&m_bg_fixed);
    }
}

bool
simplebg::set_level_size(uint16_t left, uint16_t right, uint16_t down, uint16_t up)
{
    float border_extra_span = 20.f;

    float w = (float)left+(float)right;
    float h = (float)down+(float)up;

    switch (material_factory::background_id) {
        case BG_OUTDOOR:
            {
                //this->set_mesh((struct tms_mesh*)0);
                this->set_material(&m_bg2);
                m_bg2.pipeline[0].texture[0] = m_breadboard.pipeline[0].texture[0];

                m_border.pipeline[0].texture[0] = m_bedrock.pipeline[0].texture[0];
                m_border.pipeline[1].program = 0;
                m_border.pipeline[3].program = 0;
            }
            break;

        case BG_COLORED:
            {
                this->set_material(&m_bg_colored);

                m_border.pipeline[0].texture[0] = m_border.pipeline[2].texture[0];
                m_border.pipeline[1].program = m_wood.pipeline[1].program;
                m_border.pipeline[3].program = m_bedrock.pipeline[3].program;
            }
            break;

        default:
            {
                this->set_material(&m_bg);

                m_border.pipeline[0].texture[0] = m_border.pipeline[2].texture[0];
                m_border.pipeline[1].program = m_wood.pipeline[1].program;
                m_border.pipeline[3].program = m_bedrock.pipeline[3].program;
            }
            break;
    }

    if (w < 5.f || h < 5.f) {
        tms_infof("invalid size %f %f %u %u %u %u", w, h, left, right, down, up);
        /*
        for (int x=0; x<4; x++) {
            if (this->borders[x]->parent)
                tms_entity_remove_child((struct tms_entity*)this, this->borders[x]);
            if (this->borders[x])
        }
        */

        return false;
    }

    if (this->bottom_only) {
        if (!this->borders[3]->parent)
            tms_entity_add_child((struct tms_entity*)this, this->borders[3]);
        for (int x=0; x<4; x++) {
            if (x == 3) continue;
            if (this->borders[x]->parent)
                tms_entity_remove_child((struct tms_entity*)this, this->borders[x]);
        }
    } else {
        for (int x=0; x<4; x++) {
            if (!this->borders[x]->parent)
                tms_entity_add_child((struct tms_entity*)this, this->borders[x]);
        }
    }

    float px = (float)right / 2.f - (float)left/2.f;
    float py = (float)up / 2.f - (float)down/2.f;

    b2Vec2 pos[4] = {
        b2Vec2(right+border_extra_span/2.f, py),
        b2Vec2(px, up+border_extra_span/2.f),
        b2Vec2(-left-border_extra_span/2.f, py),
        b2Vec2(px, -down-border_extra_span/2.f)
    };

    b2Vec2 size[4] = {
        b2Vec2(border_extra_span+1.f, h+border_extra_span*2.f),
        b2Vec2(w, border_extra_span+1.f),
        b2Vec2(border_extra_span+1.f, h+border_extra_span*2.f),
        b2Vec2(w, border_extra_span+1.f),
    };

    for (int x=0; x<4; x++) {
        float *M = this->borders[x]->M;
        float *N = this->borders[x]->N;
        tmat4_load_identity(M);

        tmat4_translate(M, pos[x].x, pos[x].y, (LAYER_DEPTH*3.f) / 2.f-LAYER_DEPTH/2.f);
        //tmat4_translate(M, pos[x].x, pos[x].y, 0.f);
        tmat3_copy_mat4_sub3x3(N, M);

        tmat4_scale(M, size[x].x-.01f, size[x].y-.01f, LAYER_DEPTH*3.f-.01f);
    }

    tmat4_load_identity(this->M);
    if (material_factory::background_id == BG_OUTDOOR) {
        tmat4_translate(this->M, 0, -100, -.5f);
        tmat4_scale(this->M, 1024, 200, 1.f);
    } else {
        tmat4_translate(this->M, px, py, -.5f);
        tmat4_scale(this->M, w, h, 1.f);
    }

    tmat3_load_identity(this->N);

    return true;
}


void
simplebg::set_color(tvec4 c)
{
    this->set_uniform("~color", TVEC4_INLINE(c));
}
