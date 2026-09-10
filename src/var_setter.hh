#pragma once

#include "edevice.hh"
#include "i2o0gate.hh"

/**
 * Class representing the Var Setter object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Var_Setter
 */
class var_setter : public i2o0gate {
  public:
    var_setter();
    const char* get_name(){ return "Var Setter"; }

    edevice* solve_electronics();
    void write_quickinfo(char *out);
    bool compatible_with(entity *o);
};
