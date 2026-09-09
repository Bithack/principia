#pragma once

#include "edevice.hh"

/**
 * Generic class for a 2-input, 1-output gate.
 */
class i2o1gate : public brcomp_multiconnect {
  public:
    i2o1gate();
};

/**
 * Generic class for a 2-input, 1-output gate with no symbol associated.
 */
class i2o1gate_empty : public i2o1gate {
  public:
    i2o1gate_empty();
};

/**
 * Class representing the XOR gate object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/XOR_gate
 */
class xorgate : public i2o1gate {
  public:
    xorgate();
    edevice* solve_electronics();
    const char* get_name() { return "XOR gate"; }
};

/**
 * Class representing the OR gate object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/OR_gate
 */
class orgate : public i2o1gate {
  public:
    orgate();
    edevice* solve_electronics();
    const char* get_name() { return "OR gate"; }
};

/**
 * Class representing the AND gate object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/AND_gate
 */
class andgate : public i2o1gate {
  public:
    andgate();
    edevice* solve_electronics();
    const char* get_name() { return "AND gate"; }
};

/**
 * Class representing the NAND gate object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/NAND_gate
 */
class nandgate : public i2o1gate {
  public:
    nandgate();
    edevice* solve_electronics();
    const char* get_name() { return "NAND gate"; }
};

/**
 * Class representing the IF gate object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/IF_gate
 */
class ifgate : public i2o1gate_empty {
  public:
    ifgate();
    edevice* solve_electronics();
    const char* get_name() { return "IF gate"; }
};

/**
 * Class representing the Memory module object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Memory_module
 */
class memory : public i2o1gate_empty {
  private:
    float store;
  public:
    memory();
    edevice* solve_electronics();
    const char* get_name() { return "Memory module"; }

    void write_state(lvlinfo *lvl, lvlbuf *lb) {
        entity::write_state(lvl,lb);
        lb->w_s_float(this->store);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb) {
        entity::read_state(lvl,lb);
        this->store = lb->r_float();
    }
};

/**
 * Class representing the Half pack object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Half_pack
 */
class halfpack : public i2o1gate_empty {
  public:
    edevice* solve_electronics();
    const char* get_name() { return "Half pack"; }
};

/**
 * Class representing the Sum object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Sum
 */
class sum : public i2o1gate {
  public:
    sum();
    edevice* solve_electronics();
    const char* get_name() { return "Sum"; }
};

/**
 * Class representing the Mul object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Mul
 */
class emul : public i2o1gate_empty {
  public:
    edevice* solve_electronics();
    const char* get_name() { return "Mul"; }
};

/**
 * Class representing the Avg object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Avg
 */
class avg : public i2o1gate_empty {
  public:
    edevice* solve_electronics();
    const char* get_name() { return "Avg"; }
};

/**
 * Class representing the Min object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Min
 */
class emin : public i2o1gate_empty {
  public:
    edevice* solve_electronics();
    const char* get_name() { return "Min"; }
};

/**
 * Class representing the Max object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Max
 */
class emax : public i2o1gate_empty {
  public:
    edevice* solve_electronics();
    const char* get_name() { return "Max"; }
};

/**
 * Class representing the Wrap add object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Wrap_add
 */
class wrapadd : public i2o1gate {
  public:
    wrapadd();
    edevice* solve_electronics();
    const char* get_name() { return "Wrap add"; }
};

/**
 * Class representing the Wrap sub object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Wrap_sub
 */
class wrapsub : public i2o1gate {
  public:
    wrapsub();
    edevice* solve_electronics();
    const char* get_name() { return "Wrap sub"; }
};

/**
 * Class representing the Wrap distance object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/Wrap_distance
 */
class ewrapdist : public i2o1gate_empty {
  public:
    edevice* solve_electronics();
    const char* get_name() { return "Wrap distance"; }
};

/**
 * Generic class for cmp objects
 */
class cmp : public i2o1gate {
  public:
    cmp() {
        this->s_in[0].tag = SOCK_TAG_VALUE;
        this->s_in[1].tag = SOCK_TAG_VALUE;
        this->s_out[0].tag = SOCK_TAG_GENERIC_BOOL;
    }
    virtual edevice* solve_electronics() = 0;
};

/**
 * Class representing the cmp-e object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/cmp-e
 */
class cmpe : public cmp {
  public:
    cmpe();
    const char* get_name() { return "cmp-e"; } /* == */
    edevice* solve_electronics();
};

/**
 * Class representing the cmp-l object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/cmp-l
 */
class cmpl : public cmp {
  public:
    cmpl();
    const char* get_name() { return "cmp-l"; } /* < */
    edevice* solve_electronics();
};

/**
 * Class representing the cmp-le object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/cmp-le
 */
class cmple : public cmp {
  public:
    cmple();
    const char* get_name() { return "cmp-le"; } /* <= */
    edevice* solve_electronics();
};
