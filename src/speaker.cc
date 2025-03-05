#include "speaker.hh"
#include "model.hh"
#include "game.hh"
#include "soundmanager.hh"
#include "ui.hh"

const char *speaker_options[NUM_WAVEFORMS] = {
    "Sine",
    "Square",
    "Pulse",
    "Sawtooth",
    "Triangle",
};

speaker::speaker()
{
    this->set_flag(ENTITY_HAS_CONFIG, true);

    this->dialog_id = DIALOG_SYNTHESIZER;

    this->scaleselect = true;

    this->set_mesh(mesh_factory::get_mesh(MODEL_I2O0));

    this->set_num_properties(9);
    this->properties[0].type = P_FLT; /* Min frequency */
    this->properties[0].v.f = 440/2;
    this->properties[1].type = P_FLT; /* Max frequency */
    this->properties[1].v.f = 440;
    this->properties[2].type = P_INT; /* Waveform */
    this->properties[2].v.i = 1;

    this->properties[3].type = P_FLT; /* Bitcrushing */
    this->properties[3].v.f = 0.f;

    this->properties[4].type = P_FLT; /* Volume vibrato Hz */
    this->properties[4].v.f = 0.f;

    this->properties[5].type = P_FLT; /* Frequency vibrato Hz */
    this->properties[5].v.f = 0.f;

    this->properties[6].type = P_FLT; /* Volume vibrato extent */
    this->properties[6].v.f = 0.1f;

    this->properties[7].type = P_FLT; /* Frequency vibrato extent */
    this->properties[7].v.f = 0.1f;

    this->properties[8].type = P_FLT; /* Pulse width */
    this->properties[8].v.f = 0.25f;
}

edevice*
speaker::solve_electronics()
{
    if (!this->s_in[0].is_ready())
        return this->s_in[0].get_connected_edevice();
    if (!this->s_in[1].is_ready())
        return this->s_in[1].get_connected_edevice();

    int s = this->slot;

    float freq = this->properties[0].v.f + (this->properties[1].v.f - this->properties[0].v.f) * this->s_in[1].get_value();
    float volume = this->s_in[0].p ? this->s_in[0].get_value() : 1.f;

    if (s == -1) {
        if (volume < 0.001f) return 0;

        /* find an open slot */
        for (int x=0; x<SM_MAX_CHANNELS; x++) {
            if (!sm::generated[x].available) continue;

            s = (this->slot = x);

            sm::generated[s].phase = 0.;
            sm::generated[s].bitcrushing = this->properties[3].v.f;
            sm::generated[s].volume_vibrato = this->properties[4].v.f;
            sm::generated[s].freq_vibrato = this->properties[5].v.f;
            sm::generated[s].waveform = this->properties[2].v.i;
            sm::generated[s].freq_vibrato_width = this->properties[7].v.f;
            sm::generated[s].volume_vibrato_width = this->properties[6].v.f;
            sm::generated[s].pulse_width = this->properties[8].v.f;

            sm::generated[s].ticks[sm::write_counter%SM_GENWAVE_NUM_TICKS].freq = freq;
            sm::generated[s].ticks[sm::write_counter%SM_GENWAVE_NUM_TICKS].volume = volume;

            sm::play_gen(s);
            break;
        }
    } else {
        if (volume <= 0.001f) {
            sm::generated[s].ticks[sm::write_counter%SM_GENWAVE_NUM_TICKS].command = SM_GENWAVE_STOP;

            s = (this->slot = -1);
        } else {
            sm::generated[s].ticks[sm::write_counter%SM_GENWAVE_NUM_TICKS].freq = freq;
            sm::generated[s].ticks[sm::write_counter%SM_GENWAVE_NUM_TICKS].volume = volume;
        }
    }

    return 0;
}
