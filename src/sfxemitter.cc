#include "sfxemitter.hh"
#include "soundmanager.hh"
#include "ui.hh"

struct sfxemitter_option sfxemitter_options[NUM_SFXEMITTER_OPTIONS] = {
    {"Shoot", &sm::robot_shoot, -1},
    {"Click", &sm::click, -1},
    {"Bomb throw", &sm::robot_bomb, -1},
    {"Emit", &sm::emit, -1},
    {"Absorb", &sm::absorb, -1},
    {"Explosion", &sm::explosion, -1},
    {"Ding", &sm::ding, -1},
    {"Weird", &sm::weird, -1},
    {"Detect", &sm::detect, -1},
    {"Warning", &sm::warning, -1},
    {"8-bit drum 1",  &sm::drum1, 0},
    {"8-bit drum 2",  &sm::drum1, 1},
    {"8-bit drum 3",  &sm::drum1, 2},
    {"8-bit drum 4",  &sm::drum1, 3},
    {"8-bit drum 5",  &sm::drum1, 4},
    {"8-bit drum 6",  &sm::drum1, 5},
    {"8-bit drum 7",  &sm::drum1, 6},
    {"8-bit drum 8",  &sm::drum1, 7},
    {"8-bit drum 9",  &sm::drum2, 0},
    {"8-bit drum 10", &sm::drum2, 1},
    {"8-bit drum 11", &sm::drum2, 2},
    {"8-bit drum 12", &sm::drum2, 3},
};

sfxemitter::sfxemitter()
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_SFXEMITTER;

    this->set_num_properties(2);
    this->properties[0].type = P_INT;
    this->properties[0].v.i = 0;
    this->properties[1].type = P_INT8;
    this->properties[1].v.i8 = 0;
}

edevice*
sfxemitter::solve_electronics()
{
    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }
    if (!this->s_in[1].is_ready()) {
        return this->s_in[1].get_connected_edevice();
    }

    if ((bool)roundf(this->s_in[0].get_value())) {
        b2Vec2 p = this->get_position();

        if (this->properties[0].v.i < NUM_SFXEMITTER_OPTIONS) {
            sm_sound *snd = static_cast<sm_sound*>(sfxemitter_options[this->properties[0].v.i].sound);
            int chunk = sfxemitter_options[this->properties[0].v.i].chunk;
            sm::play(snd, p.x, p.y, chunk == -1 ? rand() : chunk, this->s_in[1].p ? this->s_in[1].get_value() : 1.f, false, 0, (1 == this->properties[1].v.i8));
        }
    }

    return 0;
}

/**
 * Old SFX emitter had:
 * 2 properties.
 * Property 1, int32: Sound ID
 * Property 2, uint8(bool): Global
 *
 * New SFX Emitter has:
 * 3 properties:
 * Property 1, int32: Sound ID
 * Property 2, uint8(bool): Global
 * Property 3, int32: chunk_id
 * Property 4, uint8(bool): Loop
 **/
sfxemitter_2::sfxemitter_2()
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_SFXEMITTER_2;

    this->set_num_properties(4);
    this->properties[0].type = P_INT;
    this->properties[0].v.i = 0;
    this->properties[1].type = P_INT8;
    this->properties[1].v.i8 = 0;
    this->properties[2].type = P_INT;
    this->properties[2].v.i = SFX_CHUNK_RANDOM;
    this->properties[3].type = P_INT8;
    this->properties[3].v.i8 = 0;
}

edevice*
sfxemitter_2::solve_electronics()
{
    if (!this->s_in[0].is_ready()) {
        return this->s_in[0].get_connected_edevice();
    }
    if (!this->s_in[1].is_ready()) {
        return this->s_in[1].get_connected_edevice();
    }

    if (this->properties[0].v.i < SND__NUM) {
        sm_sound *snd = sm::sound_lookup[this->properties[0].v.i];
        if ((bool)roundf(this->s_in[0].get_value())) {
            b2Vec2 p = this->get_position();

            uint32_t chunk = this->properties[2].v.i;

            sm::play(
                    snd,
                    p.x, p.y,
                    (chunk == SFX_CHUNK_RANDOM ? rand() : chunk),
                    this->s_in[1].p ? this->s_in[1].get_value() : 1.f,
                    (1 == this->properties[3].v.i8),
                    this,
                    (1 == this->properties[1].v.i8));
        } else {
            if (1 == this->properties[3].v.i8) {
                sm::stop(snd, this);
            }
        }
    }

    return 0;
}


