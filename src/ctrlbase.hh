#pragma once

#include "edevice.hh"

/**
 * Base class for all CT entities.
 */
class ctrlbase : public brcomp_multiconnect {
  public:
    ctrlbase() {
        this->menu_scale = .75f;
        this->scaleselect = true;
    }
};

/**
 * Class representing the CT Mini object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/CT_Mini
 */
class ctrlmini : public ctrlbase {
  public:
    ctrlmini();

    edevice* solve_electronics();
    const char *get_name() { return "CT Mini"; }
};

/**
 * Class representing the CT Pass object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/CT_Pass
 */
class ctrlpass : public ctrlbase {
  public:
    ctrlpass();

    edevice* solve_electronics();
    const char *get_name() { return "CT Pass"; }
};

/**
 * Class representing the CT Servo object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/CT_Servo
 */
class ctrlservo : public ctrlbase {
  public:
    ctrlservo();

    edevice* solve_electronics();
    const char *get_name() { return "CT Servo"; }
};

/**
 * Class representing the CT Feedback object.
 *
 * Player Wiki ref: https://principia-web.se/wiki/CT_Feedback
 */
class ctrlfeedback : public ctrlbase {
  public:
    ctrlfeedback();

    edevice* solve_electronics();
    const char *get_name() { return "CT Feedback"; }
};
