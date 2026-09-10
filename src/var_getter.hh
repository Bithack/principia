#pragma once

#include "edevice.hh"
#include "i0o1gate.hh"

/**
 * Class representing the Var Getter object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Var_Getter
 */
class var_getter : public i0o1gate {
  public:
    var_getter();
    edevice* solve_electronics();
    const char *get_name() { return "Var Getter"; }
    void write_quickinfo(char *out);
    bool compatible_with(entity *o);
};
