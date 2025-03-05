#pragma once

#include "edevice.hh"
#include "model.hh"

class robot_base;

class i2o1gate : public brcomp_multiconnect
{
  public:
    i2o1gate();
};

class i2o1gate_empty : public i2o1gate
{
  public:
    i2o1gate_empty()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_EMPTY));
    }
};

class xorgate : public i2o1gate
{
  public:
    xorgate()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_XOR));
        this->s_in[0].tag = SOCK_TAG_GENERIC_BOOL;
        this->s_in[1].tag = SOCK_TAG_GENERIC_BOOL;
        this->s_out[0].tag = SOCK_TAG_GENERIC_BOOL;
    }
    edevice* solve_electronics();
    const char* get_name(){return "XOR gate";}
};

class orgate : public i2o1gate
{
  public:
    orgate()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_OR));
        this->s_in[0].tag = SOCK_TAG_GENERIC_BOOL;
        this->s_in[1].tag = SOCK_TAG_GENERIC_BOOL;
        this->s_out[0].tag = SOCK_TAG_GENERIC_BOOL;
    }
    edevice* solve_electronics();
    const char* get_name(){return "OR gate";}
};

class andgate : public i2o1gate
{
  public:
    andgate()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_AND));
        this->s_in[0].tag = SOCK_TAG_GENERIC_BOOL;
        this->s_in[1].tag = SOCK_TAG_GENERIC_BOOL;
        this->s_out[0].tag = SOCK_TAG_GENERIC_BOOL;
    }
    edevice* solve_electronics();
    const char* get_name(){return "AND gate";}
};

class nandgate : public i2o1gate
{
  public:
    nandgate()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_NAND));
        this->s_in[0].tag = SOCK_TAG_GENERIC_BOOL;
        this->s_in[1].tag = SOCK_TAG_GENERIC_BOOL;
        this->s_out[0].tag = SOCK_TAG_GENERIC_BOOL;
    }
    edevice* solve_electronics();
    const char* get_name(){return "NAND gate";}
};

class ifgate : public i2o1gate_empty
{
  public:
    ifgate()
    {
        this->s_in[0].tag = SOCK_TAG_VALUE;
        this->s_in[1].tag = SOCK_TAG_GENERIC_BOOL;
        this->s_out[0].tag = SOCK_TAG_VALUE;
    }
    edevice* solve_electronics();
    const char* get_name(){return "IF gate";}
};

class memory : public i2o1gate_empty
{
  private:
    float store;
  public:
    memory()
        : store(0.f)
    {
        this->s_in[0].tag = SOCK_TAG_SET_ENABLE;
        this->s_in[1].tag = SOCK_TAG_VALUE;
        this->s_out[0].tag = SOCK_TAG_VALUE;
    }
    edevice* solve_electronics();
    const char* get_name(){return "Memory module";}

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl,lb);
        lb->w_s_float(this->store);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl,lb);
        this->store = lb->r_float();
    }
};

class halfpack : public i2o1gate_empty
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "Half pack";}
};

class sum : public i2o1gate
{
  public:
    sum()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_SUM));
    }
    edevice* solve_electronics();
    const char* get_name(){return "Sum";}
};

class emul : public i2o1gate_empty
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "Mul";}
};

class avg : public i2o1gate_empty
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "Avg";}
};

class emin : public i2o1gate_empty
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "Min";}
};

class emax : public i2o1gate_empty
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "Max";}
};

class hp_control : public i2o1gate_empty
{
  public:
    hp_control()
        : target(0)
    {
        this->set_num_properties(1);
        this->properties[0].type = P_INT;
        this->set_flag(ENTITY_HAS_TRACKER, true);
        this->properties[0].v.i = 0;
    }

    void setup();
    void restore();
    edevice* solve_electronics();
    const char* get_name(){return "HP Control";}

    robot_base *target;
};

class condenser : public i2o1gate_empty
{
  protected:
    float value;

  public:
    condenser();
    edevice* solve_electronics();
    const char* get_name(){return "Condenser";}

    void setup();
    const char *get_slider_label(int s) {
        if (s == 0)
            return "Max value";
        else // s == 1
            return "Initial fraction";
    }
    float get_slider_snap(int s) {
        if (s == 0)
            return 1/31.f;
        else // s == 1
            return 1/20.f;
    }
    float get_slider_value(int s);
    void on_slider_change(int s, float value);

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl,lb);
        lb->w_s_float(this->value);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl,lb);
        this->value = lb->r_float();
    }
};

class wrapcondenser : public condenser
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "Wrap condenser";}
};

class wrapadd : public i2o1gate
{
  public:
    wrapadd()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_WRAP_ADD));
    }
    edevice* solve_electronics();
    const char* get_name(){return "Wrap add";}
};

class wrapsub : public i2o1gate
{
  public:
    wrapsub()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_WRAP_SUB));
    }
    edevice* solve_electronics();
    const char* get_name(){return "Wrap sub";}
};

class ewrapdist : public i2o1gate_empty
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "Wrap distance";}
};

/* master cmp interface */
class cmp : public i2o1gate
{
  public:
    cmp()
    {
        this->s_in[0].tag = SOCK_TAG_VALUE;
        this->s_in[1].tag = SOCK_TAG_VALUE;
        this->s_out[0].tag = SOCK_TAG_GENERIC_BOOL;
    }
    virtual edevice* solve_electronics() = 0;
};

class cmpe : public cmp
{
  public:
    cmpe()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_EQUAL));
    }
    const char* get_name(){return "cmp-e";} /* == */
    edevice* solve_electronics();
};

class cmpl : public cmp
{
  public:
    cmpl()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_LESS));
    }
    const char* get_name(){return "cmp-l";} /* < */
    edevice* solve_electronics();
};

class cmple : public cmp
{
  public:
    cmple()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I2O1_LESS_EQUAL));
    }
    const char* get_name(){return "cmp-le";} /* <= */
    edevice* solve_electronics();
};
