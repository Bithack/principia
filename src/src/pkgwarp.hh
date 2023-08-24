#pragma once

#include "i1o0gate.hh"
#include "iomiscgate.hh"

class pkgwarp : public i1o0gate
{
  public:
    pkgwarp();
    const char *get_name(){return "Pkg Warp";};
    edevice* solve_electronics();
};

class pkgstatus : public i0o3gate
{
  public:
    float cached_percent;
    float cached_lock;

  public:
    pkgstatus();
    const char *get_name(){return "Pkg Status";};
    edevice* solve_electronics();
    void init();
};

