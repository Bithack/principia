#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class sqrtgate : public i1o1gate
{
  public:
    sqrtgate()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_SQRT));
    }
    edevice* solve_electronics();
    const char* get_name(){return "Sqrt";}
};
