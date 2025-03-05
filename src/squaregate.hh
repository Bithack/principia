#pragma once

#include "edevice.hh"
#include "i1o1gate.hh"

class squaregate : public i1o1gate
{
  public:
    squaregate()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_SQUARE));
    }
    edevice* solve_electronics();
    const char* get_name(){return "Square";}
};

