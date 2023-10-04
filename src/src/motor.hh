#pragma once

#include "edevice.hh"

#define MOTOR_DIR_LEFT  0
#define MOTOR_DIR_RIGHT 1

#define MOTOR_TYPE_DEFAULT 0
#define MOTOR_TYPE_SERVO   1
#define MOTOR_TYPE_SIMPLE  2

class motor : public ecomp, public b2QueryCallback, public ifdevice
{
  private:
    connection c;
    connection c_side[4];
    b2Vec2 q_point;
    entity *q_result;
    b2Fixture *q_fx;
    uint8_t q_frame;
    int mtype;

  public:
    motor(int mtype);
    const char *get_name(){
        switch (this->mtype) {
            case MOTOR_TYPE_SERVO: return "Servo Motor";
            case MOTOR_TYPE_SIMPLE: return "Simple Motor";
            default: case MOTOR_TYPE_DEFAULT: return "DC Motor";
        }
    };
    void on_load(bool created, bool has_state);
    void toggle_axis_rot();
    bool allow_connection(entity *asker, uint8_t fr, b2Vec2 p);
    void find_pairs();
    void connection_create_joint(connection *c);
    bool ReportFixture(b2Fixture *f);
    connection * load_connection(connection &conn);

    struct tms_sprite* get_axis_rot_sprite();
    const char* get_axis_rot_tooltip();

    float get_slider_snap(int s){return s==0?.05f:.1f;};
    float get_slider_value(int s){return this->properties[s*3].v.f;};
    void on_slider_change(int s, float value);
    const char * get_slider_label(int s){if (s==0)return "Speed vs Torque";else return "Speed Cap";};

    edevice* solve_electronics();
    void ifstep(float v, float ctl_speed, float ctl_angle, float ctl_tradeoff, bool enable_angle, bool enable_tradeoff);
    void ifget(iffeed *feed);
    ifdevice *get_ifdevice(){return static_cast<ifdevice*>(this);};

    bool get_dir(){return this->properties[2].v.i == 0;};
    void toggle_dir(){this->properties[2].v.i = !this->properties[2].v.i;};
};
