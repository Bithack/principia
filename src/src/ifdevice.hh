#pragma once

struct iffeed
{
    float speed;
    float torque;
    float error;
    float angle;
};

class ifdevice
{
  public:
    virtual void ifstep(
            float voltage,
            float ctl_speed,
            float ctl_angle,
            float ctl_tradeoff,
            bool enable_angle,
            bool enable_tradeoff
            )=0;
    virtual void ifget(iffeed *feed)=0;
};
