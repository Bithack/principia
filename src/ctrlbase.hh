#pragma once

#include "edevice.hh"

class ctrlbase : public brcomp_multiconnect
{
  public:
    ctrlbase()
    {
        this->menu_scale = .75f;
        this->scaleselect = true;
    }
};

class ctrlmini : public ctrlbase
{
  public:
    ctrlmini();

    edevice* solve_electronics(void);
    const char *get_name(){return "CT Mini";};
};

class ctrlpass : public ctrlbase
{
  public:
    ctrlpass();

    edevice* solve_electronics(void);
    const char *get_name(){return "CT Pass";};
};

class ctrlservo : public ctrlbase
{
  public:
    ctrlservo();

    edevice* solve_electronics(void);
    const char *get_name(){return "CT Servo";};
};

class ctrlfplus : public ctrlbase
{
  public:
    ctrlfplus();

    edevice* solve_electronics(void);
    const char *get_name(){return "CT Feedback";};
};
