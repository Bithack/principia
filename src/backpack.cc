#include "backpack.hh"
#include "model.hh"
#include "material.hh"

backpack::backpack()
    : activator(ATTACHMENT_JOINT)
{
    this->set_flag(ENTITY_ALLOW_CONNECTIONS, false);
    this->set_flag(ENTITY_HAS_ACTIVATOR, true);

    this->set_mesh(mesh_factory::get_mesh(MODEL_BACKPACK));
    this->set_material(&m_edev);

    this->set_as_rect(.2f/2.f, .5f/2.f);

    this->query_vec = b2Vec2(-.15f, 0.f);
    this->query_pt = b2Vec2(-.125f, 0.f);
}

void
backpack::on_touch(b2Fixture *my, b2Fixture *other)
{
    if (my == this->fx_sensor) {
        this->activator_touched(other);
    }
}

void
backpack::on_untouch(b2Fixture *my, b2Fixture *other)
{
    if (my == this->fx_sensor) {
        this->activator_untouched(other);
    }
}
