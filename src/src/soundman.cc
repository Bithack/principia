#include "soundman.hh"
#include "model.hh"
#include "game.hh"
#include "soundmanager.hh"
#include "ui.hh"

soundman::soundman()
    : volume(0.f)
    , busy(false)
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_SOUNDMAN;

    this->set_mesh(mesh_factory::get_mesh(MODEL_I1O1_EMPTY));

    this->set_num_properties(2);
    this->properties[0].type = P_INT; /* ID of sound we are intercepting */
    this->properties[0].v.i = 0;
    this->properties[1].type = P_INT8; /* Catch all */
    this->properties[1].v.i8 = 1;
}

void
soundman::init()
{
    W->add_soundman(this->properties[0].v.i, this);
    this->busy = false;
}

void
soundman::setup()
{
    entity::setup();

    this->volume = 0.f;
}

void
soundman::remove_from_world()
{
    entity::remove_from_world();

    this->busy = false;
    W->remove_soundman(this->properties[0].v.i, this);
}

edevice*
soundman::solve_electronics(void)
{
    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }

    this->volume = this->s_in[0].get_value();
    this->s_out[0].write(this->busy ? 1.f : 0.f);

    this->busy = false;

    return 0;
}

void*
soundman::translate(uint32_t sound_id)
{
    if (sound_id < SND__NUM) {
        return sm::get_sound_by_id(sound_id);
    }

    return 0;
}
