#include "var_setter.hh"
#include "ui.hh"
#include "world.hh"

var_setter::var_setter()
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_VARIABLE;

    this->s_in[0].tag = SOCK_TAG_SET_ENABLE;
    this->s_in[1].tag = SOCK_TAG_VALUE;

    this->set_num_properties(1);

    this->properties[0].type = P_STR;
    this->set_property(0, "x");

}

edevice*
var_setter::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    bool set = (bool)((int)roundf(this->s_in[0].get_value()));

    if (set) {
        std::pair<std::map<std::string, double>::iterator, bool> ret;
        double v = this->s_in[1].get_value();
        ret = W->level_variables.insert(std::pair<std::string, double>(this->properties[0].v.s.buf, v));

        if (!ret.second) {
            (ret.first)->second = v;
        }
    }
    return 0;
}

void
var_setter::write_quickinfo(char *out)
{
    snprintf(out, 255, "%s (%s)", this->get_name(), this->properties[0].v.s.buf);
}

bool
var_setter::compatible_with(entity *o)
{
    return (this->num_properties == o->num_properties &&
            (o->g_id == O_VAR_GETTER || o->g_id == O_VAR_SETTER));
}
