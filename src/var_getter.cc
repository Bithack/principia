#include "var_getter.hh"
#include "ui.hh"
#include "world.hh"

var_getter::var_getter()
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_VARIABLE;

    this->set_num_properties(1);
    this->properties[0].type = P_STR;
    this->set_property(0, "x");
}

/* TODO: on play, save an iterator to the correct variable */

edevice*
var_getter::solve_electronics(void)
{
    std::map<std::string, double>::iterator i = W->level_variables.find(this->properties[0].v.s.buf);
    if (i != W->level_variables.end()) {
        this->s_out[0].write(tclampf(i->second, 0.f, 1.f));
    } else {
        this->s_out[0].write(0.f);
    }

    return 0;
}

bool
var_getter::compatible_with(entity *o)
{
    return (this->num_properties == o->num_properties &&
            (o->g_id == O_VAR_GETTER || o->g_id == O_VAR_SETTER));
}

void
var_getter::write_quickinfo(char *out)
{
    snprintf(out, 255, "%s (%s)", this->get_name(), this->properties[0].v.s.buf);
}
