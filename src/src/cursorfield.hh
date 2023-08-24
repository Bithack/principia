#pragma once

#include "iomiscgate.hh"

class cursorfield : public i0o3gate
{
  public:
    int  pressed;
    int  dragged;
    int  hover;

    void init();

    cursorfield();
    edevice* solve_electronics(void);

    const char *get_name(){return "Cursor field";};
};
