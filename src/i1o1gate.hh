#pragma once

#include "edevice.hh"
#include "model.hh"

class connection;

class i1o1gate : public brcomp_multiconnect
{
  public:
    i1o1gate();
};

class i1o1gate_mini : public brcomp_multiconnect
{
  public:
    i1o1gate_mini();
};

class i1o1gate_fifo : public brcomp_multiconnect
{
  public:
    i1o1gate_fifo();
};
