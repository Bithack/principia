#pragma once

#include "edevice.hh"
#include "ifdevice.hh"

class lmotor : public ecomp, public b2QueryCallback, public ifdevice
{
  private:
    connection c;
    connection c_side[4];
    entity *query_result;
    b2Fixture *query_result_fx;
    b2Vec2 query_point;
    struct tms_entity *jent;

    bool is_servo;

  public:
    lmotor(bool is_servo);

    void init(){};
    const char *get_name(){if (this->is_servo)return "Linear Servo"; else return "Linear Motor";};
    void find_pairs();
    void connection_create_joint(connection *c);
    connection * load_connection(connection &conn);

    bool allow_connection(entity *asker, uint8_t fr, b2Vec2 p);

    float get_slider_snap(int s);
    float get_slider_value(int s);
    void on_slider_change(int s, float value);
    const char *get_slider_label(int s){
        if (s == 0)
            return "Speed - Force";
        else
            return "Size";
    };

    void update(void);
    void ghost_update();
    void toggle_axis_rot();

    b2Vec2 get_joint_pos();
    void init_socks();

    inline float get_size(){
        return (this->properties[1].v.i == 0 ? .5f :
                (this->properties[1].v.i == 1 ? 1.f :
                (this->properties[1].v.i == 2 ? 2.f :
                 4.f)));
    };
    void recreate_shape();
    void construct();
    void on_load(bool created, bool has_state);
    bool ReportFixture(b2Fixture *f);

    void ifstep(float v, float ctl_speed, float ctl_angle, float ctl_tradeoff, bool enable_angle, bool enable_tradeoff);
    void ifget(iffeed *feed);
    ifdevice *get_ifdevice(){return static_cast<ifdevice*>(this);};

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_float(this->_speed);
        lb->w_s_float(this->_force);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb);
    void restore();

    float _speed;
    float _force;
};
