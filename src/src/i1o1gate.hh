#ifndef _I1O1GATE__H_
#define _I1O1GATE__H_

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

class invertergate : public i1o1gate
{
  public:
    invertergate()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_INVERT));
    }
    edevice* solve_electronics();
    const char* get_name(){return "Inverter";}
};

class integergate : public i1o1gate
{
  public:
    integergate()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_INTEGER));
    }
    edevice* solve_electronics();
    const char* get_name(){return "Floor";}
};

class snapgate : public i1o1gate_mini
{
  public:
    snapgate();
    edevice* solve_electronics();
    const char* get_name(){return "Snap";}

    const char *get_slider_label(int s) { return "Steps"; }
    float get_slider_snap(int s) { return 1.f/20.f; }
    float get_slider_value(int s) { return (this->properties[s].v.f-1.f) / 20.f; }
    void on_slider_change(int s, float value);
};

class ceilgate : public i1o1gate
{
  public:
    ceilgate() {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_INTEGER));
    }
    edevice* solve_electronics();
    const char* get_name(){return "Ceil";}
};

class squaregate : public i1o1gate
{
  public:
    squaregate()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_SQUARE));
    }
    edevice* solve_electronics();
    const char* get_name(){return "Square";}
};

class sqrtgate : public i1o1gate
{
  public:
    sqrtgate()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_SQRT));
    }
    edevice* solve_electronics();
    const char* get_name(){return "Sqrt";}
};

class sparsifier : public i1o1gate
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "Sparsifier";}
    void setup() {this->last = false;}

    sparsifier()
        : last(false)
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_SPARSIFY));
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb);
    void write_state(lvlinfo *lvl, lvlbuf *lb);

  private:
    bool last;
};

class besserwisser : public i1o1gate
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "Sparsifier+";}

    void setup(){this->last = false;};

    void read_state(lvlinfo *lvl, lvlbuf *lb);
    void write_state(lvlinfo *lvl, lvlbuf *lb);

    besserwisser()
        : last(false)
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_BESSERWISSER));
    }

  private:
    bool last;
};

class fifo : public i1o1gate_fifo
{
  private:
    float queue[8];
    int ptr;

  public:
    fifo();
    edevice* solve_electronics();
    void setup();
    const char* get_name(){return "FIFO queue";}
    void update_effects(void);

    void read_state(lvlinfo *lvl, lvlbuf *lb);
    void write_state(lvlinfo *lvl, lvlbuf *lb);
};

class mavg : public i1o1gate_fifo
{
  protected:
    float value;

  public:
    mavg();
    //float get_slider_snap(int s){return 0.f;};
    float get_slider_value(int s){return ((1.f-this->properties[0].v.f) * 8.f);};
    const char *get_slider_label(int s){return "Coefficient";};
    void on_slider_change(int s, float value){this->properties[0].v.f = 1.f-(value / 8.f);};
    edevice* solve_electronics();
    const char* get_name(){return "Moving Average";}
    void update_effects(void);
    void setup()
    {
        this->value = 0.f;
    };

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

class cavg : public mavg
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "0-Reset M. Avg";}
};

class epsilon : public i1o1gate
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "Epsilon";}

    epsilon()
    {
        this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_EPSILON));
    }

};

class clamp : public i1o1gate_mini
{
  private:
    float prev_value;

  public:
    edevice* solve_electronics();
    const char* get_name(){return "Clamp";}

    const char *get_slider_label(int s) { switch (s) { case 0: return "Min Value"; case 1: return "Max Value"; } return ""; };
    float get_slider_snap(int s) { return 0.05f; };
    float get_slider_value(int s) { return this->properties[s].v.f; }; 
    void on_slider_change(int s, float value);

    clamp();
};

class toggler : public i1o1gate_fifo
{
  private:
    bool value;

  public:
    edevice* solve_electronics();
    const char* get_name(){return "Toggler";}
    void update_effects(void);
    void setup();
    void on_pause();
    toggler();

    void on_slider_change(int s, float value);
    float get_slider_value(int s);
    float get_slider_snap(int s)
    {
        return 1.f;
    }
    const char *get_slider_label(int s)
    {
        return "Initial value";
    }

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl,lb);
        lb->w_s_uint8(this->value);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl,lb);
        this->value = lb->r_uint8();
    }
};

class valueshift : public i1o1gate
{
  public:
    valueshift();
    edevice* solve_electronics();
    const char* get_name(){return "Value shift";}

    float get_slider_value(int s)
    {
        return this->properties[s].v.f;
    };

    float get_slider_snap(int s)
    {
        return 0.05f;
    }
    const char *get_slider_label(int s)
    {
        return "Value";
    }
    void on_slider_change(int s, float value);
};

class muladd : public i1o1gate_mini
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "muladd";}

    const char *get_slider_label(int s) { switch (s) { case 0: return "multiply"; case 1: return "add"; } return ""; };
    float get_slider_snap(int s) { return 0.025f; };
    float get_slider_value(int s) {
        float v = this->properties[s].v.f;
        if (s == 0) { /* mul */
            v /= 2.0f;
        }
        return v;
    }; 
    void on_slider_change(int s, float value);

    muladd();
};

class esub : public i1o1gate_mini
{
  public:
    edevice* solve_electronics();
    const char* get_name(){return "sub";}
    void write_quickinfo(char *out);

    const char *get_slider_label(int s) { return "sub"; };
    float get_slider_snap(int s) { return 0.01f; };
    float get_slider_value(int s) { return this->properties[s].v.f; }; 
    void on_slider_change(int s, float value);

    esub();
};

class rcactivator : public i1o1gate_mini
{
  public:
    rcactivator();
    edevice* solve_electronics();
    const char* get_name(){return "RC Activator";}
};

class player_activator : public i1o1gate_mini
{
  public:
    player_activator();
    edevice* solve_electronics();
    const char* get_name(){return "Player Activator";}
};

class decay : public i1o1gate
{
  protected:
    float value;

  public:
    decay();
    edevice* solve_electronics();
    const char *get_name(){return "Decay";};

    const char *get_slider_label(int s) { return "Decay mul"; };
    float get_slider_snap(int s) { return 0.025f; };
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

class ldecay : public decay
{
  public:
    ldecay();
    edevice* solve_electronics();
    const char *get_name(){return "Linear Decay";};

    const char *get_slider_label(int s) { return "Decay rate"; };
    float get_slider_snap(int s) { return 0.02f; };
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
};

class boundary : public i1o1gate_mini
{
  protected:
    float value;

  public:
    boundary();
    edevice* solve_electronics();
    const char *get_name(){return "Boundary";};
    void setup() {this->value = this->properties[0].v.f;};
    void on_pause() {this->value = this->properties[0].v.f;};

    const char *get_slider_label(int s) { return "Initial value"; };
    float get_slider_snap(int s) { return .1f; };
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

class elimit : public i1o1gate
{
    int counter;
  public:
    elimit();
    edevice* solve_electronics();
    const char *get_name(){return "Limit";};
    void setup() {this->counter = 0;};
    void on_pause() {this->counter = 0;};

    const char *get_slider_label(int s) { return "Max ticks"; };
    float get_slider_snap(int s) { return 0.1f; };
    float get_slider_value(int s);
    void on_slider_change(int s, float value);

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl,lb);
        lb->w_s_uint32(this->counter);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl,lb);
        this->counter = lb->r_uint32();
    }
};

#define SEQUENCER_MAX_LENGTH   2048
#define SEQUENCER_MIN_TIME 16 /* in milliseconds, must be below 1000 */

class sequencer : public i1o1gate_mini
{
  private:
    uint32_t time;
    uint32_t cur_step;
    uint16_t num_steps;
    uint8_t sequence[SEQUENCER_MAX_LENGTH];
    bool started;

  public:
    sequencer();
    const char *get_name(){return "Sequencer";};

    void on_load(bool created, bool has_state);
    void on_pause() {this->setup();};
    void setup() {this->cur_step=0;this->time = 0; this->started = false;};

    inline uint16_t get_num_steps(){return this->num_steps;};

    void refresh_sequence();
    void step();
    edevice* solve_electronics();

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_uint32(this->time);
        lb->w_s_uint32(this->cur_step);
        lb->w_s_uint8(this->started ? 1 : 0);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->time = lb->r_uint32();
        this->cur_step = lb->r_uint32();
        this->started = (lb->r_uint8() != 0);
    }
};

#endif
