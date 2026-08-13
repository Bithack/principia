#pragma once

#include "edevice.hh"

class magsock;
class magplug;

/**
 * Class representing the Magnetic Socket object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Magnetic_Socket
 */
class magsock : public edev_multiconnect
{
  private:
    magplug *plug;
    b2Fixture *connected;
    b2Fixture *sensor;

  public:
    magsock();

    void add_to_world();
    void set_layer(int z);

    void on_touch(b2Fixture *a, b2Fixture *b);
    void on_untouch(b2Fixture *a, b2Fixture *b);
    void step();
    edevice* solve_electronics();
    const char* get_name() { return "Magnetic Socket"; }

    friend class magplug;
};

/**
 * Class representing the Magnetic Plug object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Magnetic_Plug
 */
class magplug : public ecomp_multiconnect
{
  private:
    magsock *sock;

  public:
    magplug();

    edevice* solve_electronics();
    const char* get_name() { return "Magnetic Plug"; }

    friend class magsock;
};
