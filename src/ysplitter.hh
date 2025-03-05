#pragma once

#include "iomiscgate.hh"

class ysplitter : public i1o2gate
{
  public:
    edevice* solve_electronics();
    const char *get_name(void){return "Y-splitter";};
};

class megasplitter : public i1o8gate
{
  public:
    edevice* solve_electronics();
    const char *get_name(void){return "8-splitter";};
};

class halfunpack : public i1o2gate
{
  public:
    edevice* solve_electronics();
    const char *get_name(void){return "Half-unpack";};

};
