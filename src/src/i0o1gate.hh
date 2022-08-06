#ifndef _I0O1GATE__H_
#define _I0O1GATE__H_

#include "edevice.hh"

/* Makes sure the sawtooth wave behaves consistently */
#define PRECISE_SAWTOOTH

class i0o1gate : public brcomp_multiconnect
{
  public:
    i0o1gate();
};

class erandom : public i0o1gate
{
  public:
    edevice* solve_electronics(void);
    const char *get_name(){return "Random";};
};

class sinewave : public i0o1gate
{
  private:
    double elapsed;

  public:
    sinewave();
    const char *get_name(){return "Sine wave";};
    void setup();
    edevice* solve_electronics();

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        /* XXX XXX we're losing precision here, maybe we should store the tick count
         * instead and just multiply the tick count with 0.008 */
        entity::write_state(lvl,lb);
        lb->w_s_float((float)this->elapsed);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl,lb);
        this->elapsed = (double)lb->r_float();
    }

    float get_slider_value(int s)
    {
        if (s == 0)
            return this->properties[0].v.f / 4.f;
        else
            return this->properties[1].v.f;
    };

    float get_slider_snap(int s)
    {
        return .05f;
    };

    const char *get_slider_label(int s)
    {
        if (s == 0)
            return "Frequency Hz";
        else
            return "Phase";
    };
    void on_slider_change(int s, float value);
};

#ifndef PRECISE_SAWTOOTH
# error Hold it right htere! state saving is limited to PRECISE_SAWTOOTH
#endif

class sawtooth : public i0o1gate
{
#ifdef PRECISE_SAWTOOTH
    uint64_t elapsed;
#else
    double elapsed;
#endif

  public:
    sawtooth();
    const char *get_name(){return "Sawtooth";};
    void setup();
    edevice* solve_electronics();

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl,lb);
        lb->w_s_uint64(this->elapsed);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl,lb);
        this->elapsed = lb->r_uint64();
    }

    float get_slider_value(int s)
    {
        if (s == 0) {
            //return tmath_logstep_position(this->properties[0].v.f, 0.02, 10);
            return this->properties[0].v.f / 4.f;
        } else
            return this->properties[1].v.f;
    };

    float get_slider_snap(int s)
    {
        return .05f;
    };

    const char *get_slider_label(int s)
    {
        if (s == 0)
            return "Frequency Hz";
        else
            return "Phase";
    };
    void on_slider_change(int s, float value);
};

class eventlistener : public i0o1gate
{
  public:
    int triggered;
    int event_id;

    eventlistener();
    void setup();
    edevice* solve_electronics(void);
    const char *get_name(){return "Event Listener";};

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_uint32(this->triggered);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->triggered = (int)lb->r_uint32();
    }

    void restore();
};

class var_getter : public i0o1gate
{
  public:
    var_getter();
    edevice* solve_electronics(void);
    const char *get_name(){return "Var getter";};
    void write_quickinfo(char *out);
    bool compatible_with(entity *o);
};

static const char *key_names[TMS_KEY__NUM] = {
    NULL, NULL, NULL, NULL,
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "Return",
    "Escape",
    "Backspace",
    "Tab",
    "Space",
    "-",
    "=",
    "[",
    "]",
    "\\",
    "#",
    ";",
    "'",
    "`",
    ",",
    ".",
    "/",
    0, /*"CapsLock",*/
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    0, /*"PrintScreen",*/
    "ScrollLock",
    "Pause",
    "Insert",
    "Home",
    "PageUp",
    "Delete",
    "End",
    "PageDown",
    "Right",
    "Left",
    "Down",
    "Up",
    0, /* "Numlock",*/
    0, /* "Keypad /",*/
    0, /* "Keypad *",*/
    0, /* "Keypad -",*/
    0, /* "Keypad +",*/
    0, /* "Keypad Enter",*/
    0, /* "Keypad 1",*/
    0, /* "Keypad 2",*/
    0, /* "Keypad 3",*/
    0, /* "Keypad 4",*/
    0, /* "Keypad 5",*/
    0, /* "Keypad 6",*/
    0, /* "Keypad 7",*/
    0, /* "Keypad 8",*/
    0, /* "Keypad 9",*/
    0, /* "Keypad 0",*/
    0, /* "Keypad .",*/
    0, /* NULL,*/
    0, /* "Application",*/
    0, /* "Power",*/
    0, /* "Keypad =",*/
    0, /* "F13",*/
    0, /* "F14",*/
    0, /* "F15",*/
    0, /* "F16",*/
    0, /* "F17",*/
    0, /* "F18",*/
    0, /* "F19",*/
    0, /* "F20",*/
    0, /* "F21",*/
    0, /* "F22",*/
    0, /* "F23",*/
    0, /* "F24",*/
    0, /* "Execute",*/
    0, /* "Help",*/
    "Menu",
    0, /* "Select",*/
    0, /* "Stop",*/
    0, /* "Again",*/
    0, /* "Undo",*/
    0, /* "Cut",*/
    0, /* "Copy",*/
    0, /* "Paste",*/
    0, /* "Find",*/
    0, /* "Mute",*/
    0, /* "VolumeUp",*/
    0, /* "VolumeDown",*/
    NULL, NULL, NULL,
    0, /* "Keypad ,",*/
    0, /* "Keypad = (AS400)",*/
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL,
    0, /* "AltErase",*/
    0, /* "SysReq",*/
    0, /* "Cancel",*/
    0, /* "Clear",*/
    0, /* "Prior",*/
    0, /* "Return",*/
    0, /* "Separator",*/
    0, /* "Out",*/
    0, /* "Oper",*/
    0, /* "Clear / Again",*/
    0, /* "CrSel",*/
    0, /* "ExSel",*/
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    0, /* "Keypad 00",*/
    0, /* "Keypad 000",*/
    0, /* "ThousandsSeparator",*/
    0, /* "DecimalSeparator",*/
    0, /* "CurrencyUnit",*/
    0, /* "CurrencySubUnit",*/
    0, /* "Keypad (",*/
    0, /* "Keypad )",*/
    0, /* "Keypad {",*/
    0, /* "Keypad }",*/
    0, /* "Keypad Tab",*/
    0, /* "Keypad Backspace",*/
    0, /* "Keypad A",*/
    0, /* "Keypad B",*/
    0, /* "Keypad C",*/
    0, /* "Keypad D",*/
    0, /* "Keypad E",*/
    0, /* "Keypad F",*/
    0, /* "Keypad XOR",*/
    0, /* "Keypad ^",*/
    0, /* "Keypad %",*/
    0, /* "Keypad <",*/
    0, /* "Keypad >",*/
    0, /* "Keypad &",*/
    0, /* "Keypad &&",*/
    0, /* "Keypad |",*/
    0, /* "Keypad ||",*/
    0, /* "Keypad :",*/
    0, /* "Keypad #",*/
    0, /* "Keypad Space",*/
    0, /* "Keypad @",*/
    0, /* "Keypad !",*/
    0, /* "Keypad MemStore",*/
    0, /* "Keypad MemRecall",*/
    0, /* "Keypad MemClear",*/
    0, /* "Keypad MemAdd",*/
    0, /* "Keypad MemSubtract",*/
    0, /* "Keypad MemMultiply",*/
    0, /* "Keypad MemDivide",*/
    0, /* "Keypad +/-",*/
    0, /* "Keypad Clear",*/
    0, /* "Keypad ClearEntry",*/
    0, /* "Keypad Binary",*/
    0, /* "Keypad Octal",*/
    0, /* "Keypad Decimal",*/
    0, /* "Keypad Hexadecimal",*/
    NULL, NULL,
    "Left Ctrl",
    "Left Shift",
    "Left Alt",
    "Left Meta",
    "Right Ctrl",
    "Right Shift",
    "Right Alt",
    "Right Meta"
};

class key_listener : public i0o1gate
{
  public:
    key_listener();
    void setup();
    edevice* solve_electronics(void);
    const char *get_name(){return "Key Listener";};

    void restore()
    {
        entity::restore();

        this->active = false;
    }

    bool active;
};

#endif
