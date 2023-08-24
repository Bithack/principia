#pragma once

#include "edevice.hh"

class i0o2gate : public brcomp_multiconnect
{
  public:
    i0o2gate();
};

class i0o3gate : public brcomp_multiconnect
{
  public:
    i0o3gate();
};

class i1o2gate : public brcomp_multiconnect
{
  public:
    i1o2gate();
};

class i1o3gate : public brcomp_multiconnect
{
  public:
    i1o3gate();
};

class i1o4gate : public brcomp_multiconnect
{
  public:
    i1o4gate();
};

class i1o8gate : public brcomp_multiconnect
{
  public:
    i1o8gate();
};

class i2o2gate : public brcomp_multiconnect
{
  public:
    i2o2gate();
};

class i3o1gate : public brcomp_multiconnect
{
  public:
    i3o1gate();
};

class i4o1gate : public brcomp_multiconnect
{
  public:
    i4o1gate();
};

class ifselect : public i3o1gate
{
  public:
    const char *get_name(){return "IF-select";}
    edevice *solve_electronics();
};
