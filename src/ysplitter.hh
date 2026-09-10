#pragma once

#include "iomiscgate.hh"

/**
 * Class representing the Y-splitter object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Y-splitter
 */
class ysplitter : public i1o2gate
{
  public:
    edevice* solve_electronics();
    const char *get_name() { return "Y-splitter"; }
};

/**
 * Class representing the 8-splitter object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/8-splitter
 */
class megasplitter : public i1o8gate
{
  public:
    edevice* solve_electronics();
    const char *get_name() { return "8-splitter"; }
};

/**
 * Class representing the Half-unpack object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Half-unpack
 */
class halfunpack : public i1o2gate {
  public:
    edevice* solve_electronics();
    const char *get_name() { return "Half-unpack"; }
};
