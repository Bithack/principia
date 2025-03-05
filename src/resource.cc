#include "resource.hh"
#include "model.hh"
#include "world.hh"
#include "game.hh"
#include "ui.hh"

struct resource_data resource_data[NUM_RESOURCES] = {
    { "Ruby", &m_colored, &mesh_factory::models[MODEL_RUBY].mesh, 1, RESOURCE_COLOR_RUBY },
    { "Sapphire", &m_colored, &mesh_factory::models[MODEL_RUBY].mesh, 1,  RESOURCE_COLOR_SAPPHIRE },
    { "Emerald", &m_colored, &mesh_factory::models[MODEL_RUBY].mesh, 1, RESOURCE_COLOR_EMERALD },
    { "Topaz", &m_colored, &mesh_factory::models[MODEL_RUBY].mesh, 1, RESOURCE_COLOR_TOPAZ },
    { "Diamond", &m_pv_colored, &mesh_factory::models[MODEL_DIAMOND].mesh, 1, RESOURCE_COLOR_DIAMOND },
    { "Copper", &m_colored, &mesh_factory::models[MODEL_STONE].mesh, 1, RESOURCE_COLOR_COPPER},
    { "Iron", &m_colored, &mesh_factory::models[MODEL_STONE].mesh, 1, RESOURCE_COLOR_IRON },
    { "Wood", &m_decoration, &mesh_factory::models[MODEL_WOOD].mesh, 0 },
    { "Aluminium", &m_colored, &mesh_factory::models[MODEL_STONE].mesh, 1, RESOURCE_COLOR_ALUMINIUM },
};

resource::resource()
{
    this->set_flag(ENTITY_HAS_CONFIG,           true);
    this->set_flag(ENTITY_DYNAMIC_UNLOADING,    true);

    this->dialog_id = DIALOG_RESOURCE;

    this->set_num_properties(2);
    this->properties[0].type = P_INT8; // resource type
    this->properties[1].type = P_INT; // amount
    this->properties[1].v.i = 1;

    this->width = .25f;
    this->amount = 1;

    this->num_sliders = 1;

    this->set_resource_type(RESOURCE_RUBY);
    this->set_amount(1);
}

void
resource::write_quickinfo(char *out)
{
    if (this->resource_type < NUM_RESOURCES)
        sprintf(out, "%s", resource_data[this->resource_type].name);
    else
        sprintf(out, "Resource");
}

void
resource::add_to_world()
{
    this->create_circle(this->get_dynamic_type(), .125f, this->material);

    this->body->GetFixtureList()->SetFilterData(world::get_filter_for_layer(this->prio, 2));
    this->body->SetFixedRotation(true);
}

void
resource::on_load(bool created, bool has_state)
{
    this->set_resource_type(this->properties[0].v.i8);
    this->set_amount(this->properties[1].v.i);
}

float
resource::get_slider_value(int s)
{
    return (this->properties[1].v.i-1) / 14.f;
}

void
resource::on_slider_change(int s, float value)
{
    this->set_amount(1+(uint32_t)roundf(value * 14.f));
    G->show_numfeed(this->amount);
}

void
resource::set_resource_type(uint8_t resource_type)
{
    if (resource_type >= NUM_RESOURCES) resource_type = NUM_RESOURCES-1;

    this->properties[0].v.i8 = resource_type;
    this->resource_type = resource_type;

    this->set_material(resource_data[resource_type].material);
    this->set_mesh(*resource_data[resource_type].mesh);
    if (resource_data[this->resource_type].set_uniform) {
        float amount_intensity = (this->amount-1) * 0.05f;
        this->set_uniform("~color",
                resource_data[this->resource_type].uniform.r+amount_intensity,
                resource_data[this->resource_type].uniform.g+amount_intensity,
                resource_data[this->resource_type].uniform.b+amount_intensity,
                resource_data[this->resource_type].uniform.a);
    }
}

void
resource::set_amount(uint32_t amount)
{
    this->amount = amount;
    this->properties[1].v.i = amount;

    if (resource_data[this->resource_type].set_uniform) {
        float amount_intensity = (this->amount-1) * 0.05f;
        this->set_uniform("~color",
                resource_data[this->resource_type].uniform.r+amount_intensity,
                resource_data[this->resource_type].uniform.g+amount_intensity,
                resource_data[this->resource_type].uniform.b+amount_intensity,
                resource_data[this->resource_type].uniform.a);
    }
}
