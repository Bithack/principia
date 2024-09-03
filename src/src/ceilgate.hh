#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class ceilgate : public i1o1gate
{
  public:
    ceilgate() {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_INTEGER));
    }
    edevice* solve_electronics();
    const char* get_name(){return "Ceil";}
};

