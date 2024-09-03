#pragma once

#include "edevice.hh"
#include "i0o1gate.hh"

class var_getter : public i0o1gate
{
  public:
    var_getter();
    edevice* solve_electronics(void);
    const char *get_name(){return "Var getter";};
    void write_quickinfo(char *out);
    bool compatible_with(entity *o);
};

