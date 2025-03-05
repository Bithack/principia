#include "decorations.hh"
#include "model.hh"
#include "ui.hh"
#include "world.hh"

struct decoration_info decorations[NUM_DECORATIONS] = {
    {
        "Stone 1",
        {.8f, .4f},
        {
            b2Vec2(0.72f, .48f), //top right
            b2Vec2(-0.72f, .16f), //top left
            b2Vec2(-0.68f, -.48f), //bottom left
            b2Vec2(0.72f, -.48f), //bottom right
        },
        4,
        &m_stone,
        &mesh_factory::models[MODEL_STONE1].mesh,
        false,
        false,
    },

    {
        "Stone 2",
        {.4f, .4f},
        {
            b2Vec2(0.54f, .45f), //top right
            b2Vec2(-0.53f, .45f), //top left
            b2Vec2(-0.52f, -.48f), //bottom left
            b2Vec2(0.54f, -.52f), //bottom right
        },
        4,
        &m_stone,
        &mesh_factory::models[MODEL_STONE2].mesh,
        false,
        false,
    },

    {
        "Stone 3",
        {.62f, .24f},
        {
            b2Vec2(0.54f, .32f), //top right
            b2Vec2(-0.5f, .28f), //top left
            b2Vec2(-0.45f, -.26f), //bottom left
            b2Vec2(0.42f, -.25f), //bottom right
        },
        4,
        &m_stone,
        &mesh_factory::models[MODEL_STONE3].mesh,
        false,
        false,
    },

    {
        "Stone 4",
        {.82f, .4f},
        {
            b2Vec2(0.9f, .17f), //top right
            b2Vec2(-0.85f, .32f), //top left
            b2Vec2(-0.58f, -.38f), //bottom left
            b2Vec2(0.64f, -.3f), //bottom right
        },
        4,
        &m_stone,
        &mesh_factory::models[MODEL_STONE4].mesh,
        false,
        true,
    },

    {
        "Stone 5",
        {.6f, .5f},
        {
            b2Vec2(0.46f, .38f), //top right
            b2Vec2(-0.43f, .43f), //top left
            b2Vec2(-0.35f, -.48f), //bottom left
            b2Vec2(0.64f, -.5f), //bottom right
        },
        4,
        &m_stone,
        &mesh_factory::models[MODEL_STONE5].mesh,
        false,
        true,
    },

    {
        "Stone 6",
        {.6f, .5f},
        {
            b2Vec2(0.67f, .26f), //top right
            b2Vec2(-0.5f, .37f), //top left
            b2Vec2(-0.55f, -.42f), //bottom left
            b2Vec2(0.57f, -.44f), //bottom right
        },
        4,
        &m_stone,
        &mesh_factory::models[MODEL_STONE6].mesh,
        false,
        false,
    },

    {
        "Mushroom 1",
        {.2f, .2f},
        {
            b2Vec2(0.62f, .65f), //top right
            b2Vec2(-0.62f, .65f), //top left
            b2Vec2(-0.62f, -.5f), //bottom left
            b2Vec2(0.62f, -.5f), //bottom right
        },
        4,
        &m_decoration,
        &mesh_factory::models[MODEL_MUSHROOM1].mesh,
        true,
        true,
    },

    {
        "Mushroom 2",
        {.2f, .2f},
        {
            b2Vec2(0.62f, .65f), //top right
            b2Vec2(-0.62f, .65f), //top left
            b2Vec2(-0.62f, -.5f), //bottom left
            b2Vec2(0.62f, -.5f), //bottom right
        },
        4,
        &m_decoration,
        &mesh_factory::models[MODEL_MUSHROOM2].mesh,
        true,
        true,
    },

    {
        "Mushroom 3",
        {.2f, .2f},
        {
            b2Vec2(0.62f, .65f), //top right
            b2Vec2(-0.62f, .65f), //top left
            b2Vec2(-0.62f, -.5f), //bottom left
            b2Vec2(0.62f, -.5f), //bottom right
        },
        4,
        &m_decoration,
        &mesh_factory::models[MODEL_MUSHROOM3].mesh,
        true,
        true,
    },

    {
        "Mushroom 4",
        {.35f, .32f},
        {
            b2Vec2(0.62f, .65f), //top right
            b2Vec2(-0.62f, .65f), //top left
            b2Vec2(-0.62f, -.5f), //bottom left
            b2Vec2(0.62f, -.5f), //bottom right
        },
        4,
        &m_decoration,
        &mesh_factory::models[MODEL_MUSHROOM4].mesh,
        true,
        true,
    },

    {
        "Mushroom 5",
        {.25f, .3f},
        {
            b2Vec2(0.62f, .65f), //top right
            b2Vec2(-0.62f, .65f), //top left
            b2Vec2(-0.62f, -.5f), //bottom left
            b2Vec2(0.62f, -.5f), //bottom right
        },
        4,
        &m_decoration,
        &mesh_factory::models[MODEL_MUSHROOM5].mesh,
        true,
        true,
    },

    {
        "Mushroom 6",
        {.2f, .2f},
        {
            b2Vec2(0.62f, .65f), //top right
            b2Vec2(-0.62f, .65f), //top left
            b2Vec2(-0.62f, -.5f), //bottom left
            b2Vec2(0.62f, -.5f), //bottom right
        },
        4,
        &m_decoration,
        &mesh_factory::models[MODEL_MUSHROOM6].mesh,
        true,
        true,
    },

    {
        "Sign 1",
        {.6f, .66f},
        {
            b2Vec2(0.62f, .54f), //top right
            b2Vec2(-0.6f, .62f), //top left
            b2Vec2(-0.64f, .1f), //bottom left
            b2Vec2(-.06f, -.65f), // middle left
            b2Vec2(.1f, -.65f), // middle right
            b2Vec2(0.54f, .07f), //bottom right
        },
        6,
        &m_decoration,
        &mesh_factory::models[MODEL_SIGN1].mesh,
        false,
        false,
    },

    {
        "Sign 2",
        {.6f, .66f},
        {
            b2Vec2(0.62f, .54f), //top right
            b2Vec2(-0.6f, .62f), //top left
            b2Vec2(-0.64f, .1f), //bottom left
            b2Vec2(-.06f, -.65f), // middle left
            b2Vec2(.1f, -.65f), // middle right
            b2Vec2(0.54f, .07f), //bottom right
        },
        6,
        &m_decoration,
        &mesh_factory::models[MODEL_SIGN2].mesh,
        false,
        false,
    },

    {
        "Sign 3",
        {.6f, .66f},
        {
            b2Vec2(0.62f, .54f), //top right
            b2Vec2(-0.6f, .62f), //top left
            b2Vec2(-0.64f, .1f), //bottom left
            b2Vec2(-.06f, -.65f), // middle left
            b2Vec2(.1f, -.65f), // middle right
            b2Vec2(0.54f, .07f), //bottom right
        },
        6,
        &m_decoration,
        &mesh_factory::models[MODEL_SIGN3].mesh,
        false,
        false,
    },

    {
        "Sign 4",
        {.6f, .66f},
        {
            b2Vec2(0.62f, .54f), //top right
            b2Vec2(-0.6f, .62f), //top left
            b2Vec2(-0.64f, .1f), //bottom left
            b2Vec2(-.06f, -.65f), // middle left
            b2Vec2(.1f, -.65f), // middle right
            b2Vec2(0.54f, .07f), //bottom right
        },
        6,
        &m_decoration,
        &mesh_factory::models[MODEL_SIGN4].mesh,
        false,
        false,
    },

    {
        "Wood Fence",
        {.99f, .54},
        {
            b2Vec2(0.62f, .65f), //top right
            b2Vec2(-0.62f, .65f), //top left
            b2Vec2(-0.62f, -.5f), //bottom left
            b2Vec2(0.62f, -.5f), //bottom right
        },
        4,
        &m_decoration,
        &mesh_factory::models[MODEL_FENCE_WOOD].mesh,
        true,
        false,
    },

    {
        "Stone head",
        {.6f, .82f},
        {
            b2Vec2(0.56f, .74f), //top right
            b2Vec2(-0.54f, .63f), //top left
            b2Vec2(-0.48f, -.18f), //bottom left
            b2Vec2(-.12f, -.56f), // middle left
            b2Vec2(.5f, -.56f), // middle right
            b2Vec2(0.63f, -.13f), //bottom right
        },
        6,
        &m_stone,
        &mesh_factory::models[MODEL_STATUE_HEAD].mesh,
        false,
        false,
    },

    {
        "Ground plant 1",
        {.11f, .11f},
        {
            b2Vec2(0.62f, .65f), //top right
            b2Vec2(-0.62f, .65f), //top left
            b2Vec2(-0.62f, -.5f), //bottom left
            b2Vec2(0.62f, -.5f), //bottom right
        },
        4,
        &m_decoration,
        &mesh_factory::models[MODEL_PLANT1].mesh,
        true,
        true,
    },

    {
        "Ground plant 2",
        {.02f, .3f},
        {
            b2Vec2(0.62f, .65f), //top right
            b2Vec2(-0.62f, .65f), //top left
            b2Vec2(-0.62f, -.5f), //bottom left
            b2Vec2(0.62f, -.5f), //bottom right
        },
        4,
        &m_decoration,
        &mesh_factory::models[MODEL_PLANT2].mesh,
        true,
        false,
    },

    {
        "Ground plant 3",
        {.02f, .3f},
        {
            b2Vec2(0.62f, .65f), //top right
            b2Vec2(-0.62f, .65f), //top left
            b2Vec2(-0.62f, -.5f), //bottom left
            b2Vec2(0.62f, -.5f), //bottom right
        },
        4,
        &m_decoration,
        &mesh_factory::models[MODEL_PLANT3].mesh,
        true,
        false,
    },

    {
        "Ground plant 4",
        {.02f, .3f},
        {
            b2Vec2(0.62f, .65f), //top right
            b2Vec2(-0.62f, .65f), //top left
            b2Vec2(-0.62f, -.5f), //bottom left
            b2Vec2(0.62f, -.5f), //bottom right
        },
        4,
        &m_decoration,
        &mesh_factory::models[MODEL_PLANT4].mesh,
        true,
        false,
    },

    /*{
        "Banner",
        {.4f, 1.2f},
        {
            b2Vec2(0.62f, .65f), //top right
            b2Vec2(-0.62f, .65f), //top left
            b2Vec2(-0.62f, -.5f), //bottom left
            b2Vec2(0.62f, -.5f), //bottom right
        },
        4,
        &m_decoration,
        &mesh_factory::models[MODEL_BANNER].mesh,
        true,
        false,
    },*/

};

decoration::decoration()
{
    this->dialog_id = DIALOG_DECORATION;
    this->set_flag(ENTITY_ALLOW_CONNECTIONS,    false);
    this->set_flag(ENTITY_HAS_CONFIG,           true);
    this->set_flag(ENTITY_DO_TICK,              true);
    this->set_flag(ENTITY_DYNAMIC_UNLOADING,    true);

    this->update_method = ENTITY_UPDATE_CUSTOM;

    this->set_num_properties(2);
    tmat4_load_identity(this->M);
    tmat3_load_identity(this->N);
    this->properties[0].type = P_INT; // decoration type
    this->properties[1].type = P_FLT; // rotation
    this->properties[1].v.f = .5f;
    this->set_decoration_type(0);
    this->do_recreate_shape = false;
}

void
decoration::tick()
{
    if (this->do_recreate_shape) {
        this->recreate_shape();
        this->do_recreate_shape = false;
    }
}

void
decoration::init()
{
    this->set_decoration_type(this->properties[0].v.i);
}

void
decoration::on_load(bool created, bool has_state)
{
    this->set_decoration_type(this->properties[0].v.i);
}

void
decoration::set_decoration_type(uint32_t t)
{
    if (t >= NUM_DECORATIONS) t = NUM_DECORATIONS-1;

    this->properties[0].v.i = t;

    const struct decoration_info &di = decorations[t];

    if (di.can_rotate) {
        this->num_sliders = 1;
    } else {
        this->num_sliders = 0;
    }
}

void
decoration::update()
{
    struct decoration_info &di = decorations[this->properties[0].v.i];

    if (di.can_rotate) {
        tmat4_load_identity(this->M);
        b2Vec2 p = this->get_position();
        tmat4_translate(this->M, p.x, p.y, this->get_layer() * LAYER_DEPTH);
        tmat4_rotate(this->M, this->get_angle() * (180.f/M_PI), 0, 0, -1);
        tmat4_rotate(this->M, (this->properties[1].v.f-.5f) * 180.f, 0, -1, 0);
        tmat3_copy_mat4_sub3x3(this->N, this->M);
        tmat4_scale(this->M, this->get_scale(), this->get_scale(), this->get_scale());
    } else {
        entity_fast_update(this);
    }
}

void
decoration::recreate_shape()
{
    b2Vec2 p = this->get_position();

    if (this->body) {
        this->body->GetWorld()->DestroyBody(this->body);
        this->body = 0;
    }

    struct decoration_info *i = &decorations[this->get_decoration_type()];

    this->set_material(i->material);
    this->set_mesh(*i->mesh);
    this->_pos = p;

    b2PolygonShape shape;

    if (i->is_rect) {
        shape.SetAsBox(i->size.x, i->size.y);
    } else {
        shape.Set(i->vertices, i->num_vertices);
    }

    b2FixtureDef fd;
    fd.shape = &shape;
    fd.density = this->get_material()->density;
    fd.friction = this->get_material()->friction;
    fd.restitution = this->get_material()->restitution;
    fd.filter = world::get_filter_for_layer(this->get_layer(), 6);

    b2BodyDef bd;
    bd.type = this->get_dynamic_type();
    bd.position = this->_pos;
    bd.angle = this->_angle;

    this->body = W->b2->CreateBody(&bd);
    (this->body->CreateFixture(&fd))->SetUserData(this);

}

void
decoration::add_to_world()
{
    this->recreate_shape();
}

void
decoration::write_quickinfo(char *out)
{
    sprintf(out, "%s", decorations[this->get_decoration_type()].name);
}
