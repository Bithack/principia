#pragma once

#include "velmeter.hh"

/**
 * Class representing the Angular Vel. Meter object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Angular_Vel._Meter
 */
class angularvelmeter : public velmeter {
  public:
    angularvelmeter();
    const char *get_name(){return "Angular Vel. Meter";};

    edevice* solve_electronics();
};
