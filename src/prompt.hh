#pragma once

#include "iomiscgate.hh"
#include "const.hh"

class base_prompt
{
  private:
    uint8_t response;
  public:
    char **message;
    struct prompt_button {
        char     **buf;
        uint32_t *len;

        prompt_button()
            : buf(0)
            , len(0)
        { }
    } buttons[3];

    inline void set_response(uint8_t new_response)
    {
        this->response = new_response;
    }

    inline uint8_t get_response()
    {
        return this->response;
    }

    base_prompt()
        : response(PROMPT_RESPONSE_NONE)
        , message(0)
    { }
};

class prompt : public i1o3gate, public base_prompt
{
  bool last;

  public:
    prompt();
    const char *get_name(){return "Prompt";};

    int get_num_buttons()
    {
        int num_buttons = 0;

        if (strlen(this->properties[1].v.s.buf) > 0)
            num_buttons ++;
        if (strlen(this->properties[2].v.s.buf) > 0)
            num_buttons ++;
        if (strlen(this->properties[3].v.s.buf) > 0)
            num_buttons ++;

        return num_buttons;
    }

    edevice* solve_electronics();

    void write_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::write_state(lvl, lb);

        lb->w_s_uint8(this->get_response());
        lb->w_s_bool(this->last);
    }

    void read_state(lvlinfo *lvl, lvlbuf *lb)
    {
        entity::read_state(lvl, lb);

        this->set_response(lb->r_uint8());
        this->last = lb->r_bool();
    }

    base_prompt *get_base_prompt() { return this; }
};
