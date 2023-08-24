#pragma once

#include "edevice.hh"

class magsock;
class magplug;

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
    void step(void);
    edevice* solve_electronics();
    const char* get_name(){return "Magnetic socket";}

    friend class magplug;
};

class magplug : public ecomp_multiconnect
{
  private:
    magsock *sock;

  public:
    magplug();

    edevice* solve_electronics();
    const char* get_name(){return "Magnetic plug";}

    friend class magsock;
};
