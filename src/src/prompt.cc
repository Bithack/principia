#include "prompt.hh"
#include "world.hh"
#include "game.hh"
#include "ui.hh"
#include "const.hh"

prompt::prompt()
    : last(false)
{
    this->set_flag(ENTITY_HAS_CONFIG, true);
    this->set_flag(ENTITY_IS_PROMPT,  true);

    this->dialog_id = DIALOG_PROMPT_SETTINGS;

    this->menu_scale = 1.5f;

    this->set_num_properties(4);

    this->properties[0].type = P_STR; /* Button #1 */
    this->set_property(0, "Yes");

    this->properties[1].type = P_STR; /* Button #2 */
    this->set_property(1, "No");

    this->properties[2].type = P_STR; /* Button #3 */
    this->set_property(2, "");

    this->properties[3].type = P_STR; /* Message */
    this->set_property(3, "Do you want to do something?");

    this->message = &this->properties[3].v.s.buf;
    this->buttons[0].buf = &this->properties[0].v.s.buf;
    this->buttons[0].len = &this->properties[0].v.s.len;

    this->buttons[1].buf = &this->properties[1].v.s.buf;
    this->buttons[1].len = &this->properties[1].v.s.len;

    this->buttons[2].buf = &this->properties[2].v.s.buf;
    this->buttons[2].len = &this->properties[2].v.s.len;
}

edevice*
prompt::solve_electronics()
{
    if (!this->s_out[0].written())
        this->s_out[0].write(this->get_response() == PROMPT_RESPONSE_A ? 1.f : 0.f);

    if (!this->s_out[1].written())
        this->s_out[1].write(this->get_response() == PROMPT_RESPONSE_B ? 1.f : 0.f);

    if (!this->s_out[2].written())
        this->s_out[2].write(this->get_response() == PROMPT_RESPONSE_C ? 1.f : 0.f);

    if (this->s_in[0].p) {
        if (!this->s_in[0].is_ready()) {
            return this->s_in[0].get_connected_edevice();
        }

        bool v = (bool)((int)roundf(this->s_in[0].get_value()));

        if (true == v && false == last && G->occupy_prompt_slot()) {
            /* TODO: Perform time checks here */
            this->set_response(PROMPT_RESPONSE_NONE);
            G->current_prompt = this;
            ui::open_dialog(DIALOG_PROMPT);
            last = v;
        } else if (false == v) {
            last = v;
        }
    }

    return 0;
}
