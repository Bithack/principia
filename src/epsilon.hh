#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class epsilon : public i1o1gate
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "Epsilon";}

    epsilon()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_EPSILON));
    }

};
