#pragma once

#include "iomiscgate.hh"

/**
 * Class representing the Cursor Field object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Cursor_Field
 */
class cursorfield : public i0o3gate {
  public:
    int  pressed;
    int  dragged;
    int  hover;

    void init();

    cursorfield();
    edevice* solve_electronics();

    const char *get_name() { return "Cursor Field"; }
};
