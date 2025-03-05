#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class integergate : public i1o1gate
{
  public:
    integergate()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_INTEGER));
    }
    edevice* solve_electronics();
    const char* get_name(){return "Floor";}
};
