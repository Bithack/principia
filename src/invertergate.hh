#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class invertergate : public i1o1gate
{
  public:
    invertergate()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_INVERT));
    }
    edevice* solve_electronics();
    const char* get_name(){return "Inverter";}
};
