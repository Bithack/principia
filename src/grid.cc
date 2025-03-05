#include "grid.hh"
#include "material.hh"

grid::grid()
{
    this->set_mesh(static_cast<tms::mesh*>(const_cast<tms_mesh*>(tms_meshfactory_get_square())));
    this->set_material(&m_grid);

    this->set_flag(ENTITY_DO_UPDATE, false);

    tmat4_load_identity(this->M);
    tmat4_translate(this->M, 0, 0, -0.499f);
    tmat4_scale(this->M, 400, 400, .0f);
    this->prio = 0;

    tmat3_load_identity(this->N);
}
