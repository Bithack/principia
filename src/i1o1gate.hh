#pragma once

#include "edevice.hh"

/**
 * Generic class for a 1-input, 1-output gate.
 */
class i1o1gate : public brcomp_multiconnect {
  public:
    i1o1gate();
};

/**
 * Generic class for a compact 1-input, 1-output gate.
 */
class i1o1gate_mini : public brcomp_multiconnect {
  public:
    i1o1gate_mini();
};

/**
 * Generic class for a 1-input, 1-output gate with a FIFO buffer.
 */
class i1o1gate_fifo : public brcomp_multiconnect {
  public:
    i1o1gate_fifo();
};
