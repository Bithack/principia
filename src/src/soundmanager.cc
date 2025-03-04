#include "soundmanager.hh"
#include "settings.hh"
#include "const.hh"
#include <tms/math/misc.h>
#include <Box2D/Box2D.h>

sm_sound*
sm::get_sound_by_id(uint32_t sound_id)
{
    if (sound_id < SND__NUM) {
        return sound_lookup[sound_id];
    }

    return 0;
}

#ifdef ENABLE_SOUND

static struct sm_load_data {
    uint32_t sound_id;
    const char *root_name;
    const char *chunk_name;
    sm_sound *sound_ptr;
    const char *path;
} load_data[] = {
    {
        SND_WOOD_METAL,
        "Wood on Metal",
        "Wood Metal 1",
        &sm::wood_metal,
        "data/sfx/wood_metal_1.wav"
    },
    {
        SND_WOOD_METAL, 0,
        "Wood Metal 2",
        &sm::wood_metal,
        "data/sfx/wood_metal_2.wav"
    },
    {
        SND_WOOD_METAL, 0,
        "Wood Metal 3",
        &sm::wood_metal,
        "data/sfx/wood_metal_3.wav"
    },

    {
        SND_WOOD_WOOD,
        "Wood on Wood",
        "Wood Wood 1",
        &sm::wood_wood,
        "data/sfx/lightwood_1.wav"
    },
    {
        SND_WOOD_WOOD, 0,
        "Wood Wood 2",
        &sm::wood_wood,
        "data/sfx/lightwood_2.wav"
    },
    {
        SND_WOOD_WOOD, 0,
        "Wood Wood 3",
        &sm::wood_wood,
        "data/sfx/lightwood_3.wav"
    },
    {
        SND_WOOD_WOOD, 0,
        "Wood Wood 4",
        &sm::wood_wood,
        "data/sfx/lightwood_4.wav"
    },

    {
        SND_WOOD_HOLLOWWOOD,
        "Wood on Hollow Wood",
        "Wood Hollow Wood 1",
        &sm::wood_hollowwood,
        "data/sfx/wood0.wav"
    },
    {
        SND_WOOD_HOLLOWWOOD, 0,
        "Wood Hollow Wood 2",
        &sm::wood_hollowwood,
        "data/sfx/wood1.wav"
    },
    {
        SND_WOOD_HOLLOWWOOD, 0,
        "Wood Hollow Wood 3",
        &sm::wood_hollowwood,
        "data/sfx/wood2.wav"
    },
    {
        SND_WOOD_HOLLOWWOOD, 0,
        "Wood Hollow Wood 4",
        &sm::wood_hollowwood,
        "data/sfx/wood3.wav"
    },
    {
        SND_WOOD_HOLLOWWOOD, 0,
        "Wood Hollow Wood 5",
        &sm::wood_hollowwood,
        "data/sfx/wood4.wav"
    },
    {
        SND_WOOD_HOLLOWWOOD, 0,
        "Wood Hollow Wood 6",
        &sm::wood_hollowwood,
        "data/sfx/wood5.wav"
    },

    {
        SND_CLICK,
        "Click",
        "Click",
        &sm::click,
        "data/sfx/click.wav"
    },

    {
        SND_DROP_ABSORB,
        "Drop absorb",
        "Drop absorb",
        &sm::drop_absorb,
        "data/sfx/mini_absorb.wav"
    },

    {
        SND_ROBOT,
        "Robot feet",
        "Robot feet",
        &sm::robot,
        "data/sfx/robot.wav"
    },

    {
        SND_ROBOT_SHOOT,
        "Arm cannon",
        "Arm cannon",
        &sm::robot_shoot,
        "data/sfx/shoot.wav"
    },

    {
        SND_SHOTGUN_SHOOT,
        "Shotgun",
        "Shotgun",
        &sm::shotgun_shoot,
        "data/sfx/shotgun_shoot.wav"
    },

    {
        SND_SHOTGUN_COCK,
        "Shotgun slide",
        "Shotgun slide",
        &sm::shotgun_cock,
        "data/sfx/shotgun_cock.wav"
    },

    {
        SND_RAILGUN_SHOOT,
        "Railgun",
        "Railgun",
        &sm::railgun_shoot,
        "data/sfx/railgun_shoot.wav"
    },

    {
        SND_ROBOT_BOMB,
        "Robot bomb",
        "Robot bomb",
        &sm::robot_bomb,
        "data/sfx/bomber.wav"
    },

    {
        SND_ROCKET,
        "Rocket",
        "Rocket",
        &sm::rocket,
        "data/sfx/rocket.wav"
    },

    {
        SND_THRUSTER,
        "Thruster",
        "Thruster",
        &sm::thruster,
        "data/sfx/thruster.wav"
    },

    {
        SND_EXPLOSION,
        "Explosion",
        "Explosion 1",
        &sm::explosion,
        "data/sfx/explosion_1.wav"
    },
    {
        SND_EXPLOSION, 0,
        "Explosion 2",
        &sm::explosion,
        "data/sfx/explosion_2.wav"
    },
    {
        SND_EXPLOSION, 0,
        "Explosion 3",
        &sm::explosion,
        "data/sfx/explosion_3.wav"
    },
    {
        SND_EXPLOSION, 0,
        "Explosion 4",
        &sm::explosion,
        "data/sfx/explosion_4.wav"
    },
    {
        SND_EXPLOSION, 0,
        "Explosion 5",
        &sm::explosion,
        "data/sfx/explosion_5.wav"
    },

    {
        SND_EXPLOSION_LIGHT,
        "Light explosion",
        "Light explosion",
        &sm::explosion_light,
        "data/sfx/explosion_light.wav"
    },

    {
        SND_SHEET_METAL,
        "Sheet metal",
        "Sheet metal 1",
        &sm::sheet_metal,
        "data/sfx/sheet_metal_1.wav"
    },
    {
        SND_SHEET_METAL, 0,
        "Sheet metal 2",
        &sm::sheet_metal,
        "data/sfx/sheet_metal_2.wav"
    },
    {
        SND_SHEET_METAL, 0,
        "Sheet metal 3",
        &sm::sheet_metal,
        "data/sfx/sheet_metal_3.wav"
    },
    {
        SND_SHEET_METAL, 0,
        "Sheet metal 4",
        &sm::sheet_metal,
        "data/sfx/sheet_metal_4.wav"
    },
    {
        SND_SHEET_METAL, 0,
        "Sheet metal 5",
        &sm::sheet_metal,
        "data/sfx/sheet_metal_5.wav"
    },
    {
        SND_SHEET_METAL, 0,
        "Sheet metal 6",
        &sm::sheet_metal,
        "data/sfx/sheet_metal_6.wav"
    },
    {
        SND_SHEET_METAL, 0,
        "Sheet metal 7",
        &sm::sheet_metal,
        "data/sfx/sheet_metal_7.wav"
    },

    {
        SND_RUBBER,
        "Rubber",
        "Rubber 1",
        &sm::rubber,
        "data/sfx/rubber_1.wav"
    },
    {
        SND_RUBBER, 0,
        "Rubber 2",
        &sm::rubber,
        "data/sfx/rubber_2.wav"
    },

    {
        SND_ABSORB,
        "Absorb",
        "Absorb",
        &sm::absorb,
        "data/sfx/absorb.wav"
    },

    {
        SND_EMIT,
        "Emit",
        "Emit",
        &sm::emit,
        "data/sfx/emit.wav"
    },

    {
        SND_METAL_METAL,
        "Metal on Metal 1",
        "Metal on Metal 1.1",
        &sm::metal_metal,
        "data/sfx/metal_metal_1.wav"
    },
    {
        SND_METAL_METAL, 0,
        "Metal on Metal 1.2",
        &sm::metal_metal,
        "data/sfx/metal_metal_2.wav"
    },
    {
        SND_METAL_METAL, 0,
        "Metal on Metal 1.3",
        &sm::metal_metal,
        "data/sfx/metal_metal_3.wav"
    },
    {
        SND_METAL_METAL, 0,
        "Metal on Metal 1.4",
        &sm::metal_metal,
        "data/sfx/metal_metal_4.wav"
    },
    {
        SND_METAL_METAL, 0,
        "Metal on Metal 1.5",
        &sm::metal_metal,
        "data/sfx/metal_metal_5.wav"
    },

    {
        SND_METAL_METAL2,
        "Metal on Metal 2",
        "Metal on Metal 2.1",
        &sm::metal_metal2,
        "data/sfx/metal_metal2_1.wav"
    },
    {
        SND_METAL_METAL2, 0,
        "Metal on Metal 2.2",
        &sm::metal_metal2,
        "data/sfx/metal_metal2_2.wav"
    },
    {
        SND_METAL_METAL2, 0,
        "Metal on Metal 2.3",
        &sm::metal_metal2,
        "data/sfx/metal_metal2_3.wav"
    },
    {
        SND_METAL_METAL2, 0,
        "Metal on Metal 2.4",
        &sm::metal_metal2,
        "data/sfx/metal_metal2_4.wav"
    },

    {
        SND_PLAYER_DEATH,
        "Player death",
        "Player death",
        &sm::lose,
        "data/sfx/lose.wav"
    },

    {
        SND_DING,
        "Ding",
        "Ding",
        &sm::ding,
        "data/sfx/ding_light.wav"
    },

    {
        SND_WEIRD,
        "Weird",
        "Weird",
        &sm::weird,
        "data/sfx/weird.wav"
    },

    {
        SND_DETECT,
        "Detect",
        "Detect",
        &sm::detect,
        "data/sfx/detect.wav"
    },

    {
        SND_WARNING,
        "Warning",
        "Warning",
        &sm::warning,
        "data/sfx/warning.wav"
    },

    {
        SND_DRUM1,
        "Drum 1",
        "Drum 1.1",
        &sm::drum1,
        "data/sfx/drum-0.wav"
    },
    {
        SND_DRUM1, 0,
        "Drum 1.2",
        &sm::drum1,
        "data/sfx/drum-1.wav"
    },
    {
        SND_DRUM1, 0,
        "Drum 1.3",
        &sm::drum1,
        "data/sfx/drum-2.wav"
    },
    {
        SND_DRUM1, 0,
        "Drum 1.4",
        &sm::drum1,
        "data/sfx/drum-3.wav"
    },
    {
        SND_DRUM1, 0,
        "Drum 1.5",
        &sm::drum1,
        "data/sfx/drum-4.wav"
    },
    {
        SND_DRUM1, 0,
        "Drum 1.6",
        &sm::drum1,
        "data/sfx/drum-5.wav"
    },
    {
        SND_DRUM1, 0,
        "Drum 1.7",
        &sm::drum1,
        "data/sfx/drum-6.wav"
    },
    {
        SND_DRUM1, 0,
        "Drum 1.8",
        &sm::drum1,
        "data/sfx/drum-7.wav"
    },

    {
        SND_DRUM2,
        "Drum 2",
        "Drum 2.1",
        &sm::drum2,
        "data/sfx/drum-8.wav"
    },
    {
        SND_DRUM2, 0,
        "Drum 2.2",
        &sm::drum2,
        "data/sfx/drum-9.wav"
    },
    {
        SND_DRUM2, 0,
        "Drum 2.3",
        &sm::drum2,
        "data/sfx/drum-10.wav"
    },
    {
        SND_DRUM2, 0,
        "Drum 2.4",
        &sm::drum2,
        "data/sfx/drum-11.wav"
    },

    {
        SND_BUBBLES,
        "Bubbles",
        "Bubbles",
        &sm::bubbles,
        "data/sfx/bubbles.wav"
    },

    {
        SND_ROCKET_LAUNCHER_SHOOT,
        "Rocket launcher",
        "Rocket launcher",
        &sm::rocket_launcher_shoot,
        "data/sfx/rocket_launcher_shoot.wav"
    },

    {
        SND_CASH_REGISTER,
        "Vendor",
        "Vendor",
        &sm::cash_register,
        "data/sfx/cash_register.wav"
    },

    {
        SND_DISCHARGE,
        "Discharge",
        "Discharge",
        &sm::discharge,
        "data/sfx/discharge.wav"
    },

    {
        SND_PLASMA_SHOOT,
        "Plasma gun",
        "Plasma gun",
        &sm::plasma_shoot,
        "data/sfx/plasma_shoot.wav"
    },

    {
        SND_BUSTER_SHOOT,
        "Mega buster shoot",
        "Mega buster shoot",
        &sm::buster_shoot,
        "data/sfx/buster_shoot.wav"
    },

    {
        SND_BUSTER_SHOOT_MAXCHARGE,
        "Mega buster shoot full charge",
        "Mega buster shoot full charge",
        &sm::buster_shoot_maxcharge,
        "data/sfx/buster_shoot_maxcharge.wav"
    },

    {
        SND_BUSTER_CHARGE,
        "Mega buster charge",
        "Mega buster charge 1",
        &sm::buster_charge,
        "data/sfx/buster_charge_1.wav"
    },
    {
        SND_BUSTER_CHARGE, 0,
        "Mega buster charge 2",
        &sm::buster_charge,
        "data/sfx/buster_charge_2.wav"
    },

    {
        SND_BUSTER_CHARGE, 0,
        "Mega buster charge 3",
        &sm::buster_charge,
        "data/sfx/buster_charge_3.wav"
    },


    {
        SND_STONE_STONE,
        "Stone on Stone",
        "Stone on Stone",
        &sm::stone_stone,
        "data/sfx/stone_stone.wav"
    },

    {
        SND_CHEST_OPEN,
        "Chest open",
        "Chest open",
        &sm::chest_open,
        "data/sfx/chest_open.wav"
    },

    {
        SND_CHEST_OPEN_RARE,
        "Chest open rare",
        "Chest open rare",
        &sm::chest_open_rare,
        "data/sfx/chest_open_rare.wav"
    },

    {
        SND_MINING_HIT_ORE,
        "Mining hit ore",
        "Mining hit ore",
        &sm::mining_hit_ore,
        "data/sfx/mining_hit_ore.wav"
    },

    {
        SND_ZAPPER,
        "Zapper",
        "Zapper",
        &sm::zapper,
        "data/sfx/zapper.wav"
    },

    {
        SND_COMPRESSOR,
        "Compressor",
        "Compressor",
        &sm::compressor,
        "data/sfx/compressor.wav"
    },

    {
        SND_COMPRESSOR_REVERSE,
        "Compressor reverse",
        "Compressor reverse",
        &sm::compressor_reverse,
        "data/sfx/compressor-reverse.wav"
    },

    {
        SND_SAW,
        "Saw",
        "Saw",
        &sm::saw_loop,
        "data/sfx/saw_loop.wav"
    },

    {
        SND_CHOP_WOOD,
        "Chop wood",
        "Chop wood 1",
        &sm::chop_wood,
        "data/sfx/chop_wood_1.wav"
    },
    {
        SND_CHOP_WOOD, 0,
        "Chop wood 2",
        &sm::chop_wood,
        "data/sfx/chop_wood_2.wav"
    },
    {
        SND_CHOP_WOOD, 0,
        "Chop wood 3",
        &sm::chop_wood,
        "data/sfx/chop_wood_3.wav"
    },
    {
        SND_CHOP_WOOD, 0,
        "Chop wood 4",
        &sm::chop_wood,
        "data/sfx/chop_wood_4.wav"
    },

    {
        SND_SWISH_HAMMER,
        "Swish Hammer",
        "Swish Hammer 1",
        &sm::swish_hammer,
        "data/sfx/swish_hammer_1.wav"
    },
    {
        SND_SWISH_HAMMER, 0,
        "Swish Hammer 2",
        &sm::swish_hammer,
        "data/sfx/swish_hammer_2.wav"
    },
    {
        SND_SWISH_HAMMER, 0,
        "Swish Hammer 3",
        &sm::swish_hammer,
        "data/sfx/swish_hammer_3.wav"
    },

    {
        SND_SWISH_BLADE,
        "Swish Blade",
        "Swish Blade 1",
        &sm::swish_blade,
        "data/sfx/swish_blade_1.wav"
    },
    {
        SND_SWISH_BLADE, 0,
        "Swish Blade 2",
        &sm::swish_blade,
        "data/sfx/swish_blade_2.wav"
    },
    {
        SND_SWISH_BLADE, 0,
        "Swish Blade 3",
        &sm::swish_blade,
        "data/sfx/swish_blade_3.wav"
    },

    {
        SND_SWISH_SPEAR,
        "Swish Spear",
        "Swish Spear 1",
        &sm::swish_spear,
        "data/sfx/swish_spear_1.wav"
    },
    {
        SND_SWISH_SPEAR, 0,
        "Swish Spear 2",
        &sm::swish_spear,
        "data/sfx/swish_spear_2.wav"
    },

    {
        SND_SWISH_AXE,
        "Swish Axe",
        "Swish Axe 1",
        &sm::swish_axe,
        "data/sfx/swish_axe_1.wav"
    },
    {
        SND_SWISH_AXE, 0,
        "Swish Axe 2",
        &sm::swish_axe,
        "data/sfx/swish_axe_2.wav"
    },
    {
        SND_SWISH_AXE, 0,
        "Swish Axe 3",
        &sm::swish_axe,
        "data/sfx/swish_axe_3.wav"
    },

    {
        SND_HAPPY,
        "Happy",
        "Happy",
        &sm::happy,
        "data/sfx/happy.wav"
    },

    {
        SND_TEST,
        "Test",
        "Test",
        &sm::test,
        "data/sfx/test.wav"
    },

    {
        SND_WIN,
        "Win",
        "Win",
        &sm::win,
        "data/sfx/win.wav"
    },

    {
        SND_MOTOR_STARTSTOP,
        "Motor Start/Stop",
        "Motor Start",
        &sm::motor_startstop,
        "data/sfx/motor_start.wav"
    },
    {
        SND_MOTOR_STARTSTOP, 0,
        "Motor Stop",
        &sm::motor_startstop,
        "data/sfx/motor_stop.wav"
    },
};

static size_t num_chunks_to_load = sizeof(load_data) / sizeof(load_data[0]);

#endif

static void channel_finished_cb(int channel);

bool      sm::enabled = true;
bool      sm::initialized = false;
tvec2     sm::position;
bool      sm::gen_started = false;

float   sm::volume = 1.f;

int      sm::tick_counter=0;
int      sm::remainder_counter=0;
uint64_t sm::read_counter=0;
uint64_t sm::write_counter=0;

sm_sound sm::test;
sm_sound sm::wood_metal;
sm_sound sm::wood_wood;
sm_sound sm::wood_hollowwood;
sm_sound sm::click;
sm_sound sm::drop_absorb;
sm_sound sm::robot;
sm_sound sm::robot_shoot;
sm_sound sm::shotgun_shoot;
sm_sound sm::shotgun_cock;
sm_sound sm::railgun_shoot;
sm_sound sm::robot_bomb;
sm_sound sm::rocket;
sm_sound sm::thruster;
sm_sound sm::explosion;
sm_sound sm::explosion_light;
sm_sound sm::sheet_metal;
sm_sound sm::rubber;
sm_sound sm::absorb;
sm_sound sm::emit;
sm_sound sm::metal_metal;
sm_sound sm::metal_metal2;
sm_sound sm::win;
sm_sound sm::lose;
sm_sound sm::motor_startstop;
sm_sound sm::ding;
sm_sound sm::weird;
sm_sound sm::detect;
sm_sound sm::warning;
sm_sound sm::drum1;
sm_sound sm::drum2;
sm_sound sm::bubbles;
sm_sound sm::rocket_launcher_shoot;
sm_sound sm::cash_register;
sm_sound sm::discharge;
sm_sound sm::plasma_shoot;
sm_sound sm::buster_shoot;
sm_sound sm::buster_shoot_maxcharge;
sm_sound sm::buster_charge;
sm_sound sm::stone_stone;
sm_sound sm::chest_open;
sm_sound sm::chest_open_rare;
sm_sound sm::mining_hit_ore;
sm_sound sm::zapper;
sm_sound sm::happy;
sm_sound sm::compressor;
sm_sound sm::compressor_reverse;
sm_sound sm::saw_loop;
sm_sound sm::chop_wood;
sm_sound sm::swish_hammer;
sm_sound sm::swish_blade;
sm_sound sm::swish_spear;
sm_sound sm::swish_axe;

sm_sound* sm::sound_lookup[SND__NUM];
genwave_data sm::generated[SM_MAX_CHANNELS];

#ifdef ENABLE_SOUND

Mix_Chunk *sm::genchunk;

sm_channel sm::channels[SM_MAX_CHANNELS];

void genwave(int chan, void *stream, int len, void *udata)
{
    Sint16* pstream = (Sint16*) stream;

    //int vol = roundf((65535./2. *.75) *data->volume);
    bool did_wait = false;

    for (int i = 0; i < len/2/2; i++)
    {
        int16_t v = 0;

        for (int chan=0; chan<SM_MAX_CHANNELS; chan++) {
            if (sm::generated[chan].available)
                continue;

            struct genwave_data *data = (struct genwave_data*)&sm::generated[chan];

            if (!data->started)
                continue;

            int vol = roundf((65535./2. *.125) * data->ticks[sm::read_counter%SM_GENWAVE_NUM_TICKS].volume) * sm::volume;

            if (data->bitcrushing > 0.f) {
                int b = (int)roundf(data->bitcrushing);

                data->bitcrush_counter = (data->bitcrush_counter+1)%b;

                if (data->bitcrush_counter == 0) {
                    data->phase += data->ticks[sm::read_counter%SM_GENWAVE_NUM_TICKS].freq/44100. * b;
                }
            } else {
                data->phase += data->ticks[sm::read_counter%SM_GENWAVE_NUM_TICKS].freq / 44100.;
            }

            if (data->freq_vibrato > 0.f && data->freq_vibrato_width > 0.f)
                data->phase += sin(M_PI*2. * _tms.last_time/1000000. * data->freq_vibrato) * data->freq_vibrato_width * data->ticks[sm::read_counter%SM_GENWAVE_NUM_TICKS].freq / 44100.;

            double phase = data->phase;

            double wave;

            if (data->waveform == 3)
                wave = fmod(phase*2., 2.) - 1.;
            else if (data->waveform == 4) {
                wave = fmod(phase, 1.);
                if (wave > 0.5) wave = .5-wave;
                wave = wave-.25 * 4.;
            } else
                wave = sin(M_PI*2. * phase);

            if (data->waveform == 1)
                wave = (wave >= 0.f ? 1.f : -1.f);
            else if (data->waveform == 2) {
                wave = (wave + 1.f) / 2.f;
                wave = (wave >= (1.f-data->pulse_width) ? 1.f : -1.f);
            }

            if (data->volume_vibrato > 0.f)
                wave *= .5-.5*sin(M_PI*2. * _tms.last_time/1000000. * data->volume_vibrato) * data->volume_vibrato_width;

            v += wave * vol;
        }

        *pstream = v;
        pstream++;
        *pstream = v; /* stereo */
        pstream++;

        sm::tick_counter ++;

        if ((sm::tick_counter >= 353)) {
            sm::read_counter ++;

            if (sm::read_counter >= sm::write_counter) {
                sm::read_counter = sm::write_counter-1;
                did_wait = true;
            } else {
                sm::tick_counter = (((int)sm::write_counter - (int)sm::read_counter) - SM_GENWAVE_PRETICKS)*4;
                //sm::tick_counter = 0;
                //sm::remainder_counter ++;

                //if (sm::remainder_counter == 5) {
                    //sm::remainder_counter = 0;
                    //sm::tick_counter = 1 + ((int)sm::write_counter - (int)sm::read_counter) - SM_GENWAVE_PRETICKS;
                //}
            }

            for (int chan=0; chan<SM_MAX_CHANNELS; chan++) {
                if (sm::generated[chan].available)
                    continue;
                struct genwave_data *data = (struct genwave_data*)&sm::generated[chan];
                if (!data->started) {
                    if (data->ticks[sm::read_counter%SM_GENWAVE_NUM_TICKS].command == SM_GENWAVE_START) {
                        data->started = true;
                        tms_debugf("STARTING %d, read_counter %" PRIu64 " write_counter %" PRIu64 " (%d)", chan, sm::read_counter, sm::write_counter, (int)sm::write_counter - (int)sm::read_counter);
                    } else {
                        //tms_debugf("not started");
                        continue;
                    }
                } else if (data->ticks[sm::read_counter%SM_GENWAVE_NUM_TICKS].command == SM_GENWAVE_STOP) {
                    tms_debugf("stopping %d, read_counter %" PRIu64 " write_counter %" PRIu64 " (%d)", chan, sm::read_counter, sm::write_counter, (int)sm::write_counter - (int)sm::read_counter);
                    for (int t=0; t<SM_GENWAVE_NUM_TICKS; t++) {
                        data->ticks[t].command = SM_GENWAVE_NO_COMMAND;
                    }
                    sm::generated[chan].available = true;
                    sm::generated[chan].started = false;
                    continue;
                }
            }
        }
    }

    if (did_wait) {
        //tms_debugf("how the fuck");
    }
}

void genwave_cleanup(int chan, void *udata)
{
  //free(udata);
}

static char genbuf[2048*20];

void
sm::play_gen(int x)
{
    if (!sm::gen_started) {
        sm::gen_started = true;
        sm::read_counter = 0;
        sm::write_counter = 0;
        Mix_PlayChannel(SM_MAX_CHANNELS, sm::genchunk, -1);
        static bool reg = false;
        Mix_RegisterEffect(SM_MAX_CHANNELS, genwave, genwave_cleanup, 0);

        for (int t=1; t<SM_GENWAVE_PRETICKS; t++) {
            sm::generated[x].ticks[t].freq = sm::generated[0].ticks[t].freq;
            sm::generated[x].ticks[t].volume = sm::generated[0].ticks[t].volume;
        }
        sm::write_counter = SM_GENWAVE_PRETICKS;
    }

    for (int t=0; t<SM_GENWAVE_NUM_TICKS; t++) {
        sm::generated[x].ticks[t].command = SM_GENWAVE_NO_COMMAND;
    }
    sm::generated[x].started = false;
    sm::generated[x].ticks[sm::write_counter%SM_GENWAVE_NUM_TICKS].command = SM_GENWAVE_START;
    sm::generated[x].available = false;
}

void
sm::load_settings()
{
    sm::volume = settings["volume"]->v.f;
    if (settings["muted"]->v.b) {
        sm::volume = 0.f;
    }
}

void
sm::init()
{
    tms_infof("Initializing audio device...");
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
        tms_infof("Error: %s\n", Mix_GetError());
        sm::initialized = false;
    } else {
        Mix_ChannelFinished(&channel_finished_cb);
        sm::initialized = true;

        /* verbose some info about the audio subsystem */
        tms_infof(">> Audio Device: %s", SDL_GetAudioDeviceName(0,0));
        tms_infof(">> Audio Driver: %s", SDL_GetAudioDriver(0));
    }

    /* sm must be initialized after settings has been initialized */
    sm::load_settings();

    Mix_AllocateChannels(SM_MAX_CHANNELS+1);
    {
        sm::genchunk = Mix_QuickLoad_RAW((uint8_t*)genbuf, 2048*20);

        int x;
        for (x=0; x<SM_MAX_CHANNELS; x++) {
            sm::generated[x].available = true;
        }

        Mix_Volume(x, MIX_MAX_VOLUME);
    }

    for (int x=0; x<SM_MAX_CHANNELS; x++) {
        sm::channels[x].playing = false;
    }

    /* load all sound effects */
    tms_infof("Initializing sound effects... ");

    sm::click.min_repeat_ms = 80;
    sm::robot.min_repeat_ms = 40;
    sm::buster_charge.min_repeat_ms = 30;

    for (int x=0; x<num_chunks_to_load; ++x) {
        struct sm_load_data *data = &load_data[x];

        data->sound_ptr->add_chunk(data->path, data->chunk_name);

        sm::sound_lookup[data->sound_id] = data->sound_ptr;

        if (data->root_name) {
            data->sound_ptr->name = data->root_name;
        }
    }
}

static void channel_finished_cb(int channel)
{
    //tms_debugf("finish");
    if (channel < SM_MAX_CHANNELS) {
        sm::channels[channel].playing = false;
    }
}

void
sm::play(sm_sound *snd, float x, float y, uint8_t random, float volume, bool loop/*=false*/, void *ident/*=0*/, bool global/*=false*/)
{
    volume *= sm::volume;
    if (volume <= SM_MIN_VOLUME) {
        return;
    }

    if (sm::initialized && sm::enabled) {
        if (snd->last_chan != -1 && loop) {

            b2Vec2 p = b2Vec2(sm::position.x, sm::position.y);
            b2Vec2 d1 = b2Vec2(x,y)-p;
            b2Vec2 d2 = b2Vec2(channels[snd->last_chan].position.x,channels[snd->last_chan].position.y)-p;

            if (sm::channels[snd->last_chan].ident == ident) {
                sm::channels[snd->last_chan].position = (tvec2){x,y};
                sm::channels[snd->last_chan].volume = volume;
            } else {
                if (d1.Length() < d2.Length()) {
                    sm::channels[snd->last_chan].ident = ident;
                    sm::channels[snd->last_chan].position = (tvec2){x,y};
                    sm::channels[snd->last_chan].volume = volume;
                }
            }
            return;
        }

        if ((_tms.last_time - snd->last_time) > snd->min_repeat_ms*1000) {
            if (snd->last_chan == -1 || !loop) {

                int chan = -1;
                for (int c=0; c<SM_MAX_CHANNELS; c++) {
                    if (!Mix_Playing(c)) {
                        chan = c;
                        break;
                    }
                }

                if (chan != -1 && chan < SM_MAX_CHANNELS) {
                    Mix_PlayChannel(chan, snd->chunks[random % snd->num_chunks].chunk, loop?-1:0);

                    channels[chan].chan = chan;
                    channels[chan].position = (tvec2){x,y};
                    channels[chan].ident = ident;
                    channels[chan].sound = snd;
                    channels[chan].volume = volume;
                    channels[chan].playing = true;
                    channels[chan].global = global;
                    channels[chan].update_position();

                    /* TODO: Make a priority list of sounds, prioritize sounds that are closeby
                     * instead of just omitting sounds with a large distance. */
                    if (channels[chan].distance > 200) {
                        Mix_HaltChannel(chan);
                        snd->reset();
                    } else {
                        snd->last_chan = chan;
                        snd->last_time = _tms.last_time;

                        Mix_Volume(chan, (int)(roundf(tclampf(volume, 0.f, 1.f)*MIX_MAX_VOLUME)));
                    }
                } else {

                }
            }
        }
    }
}

bool
sm::stop(sm_sound *snd, void *ident)
{
#ifdef DEBUG
    if (snd->last_chan != -1 && sm::channels[snd->last_chan].ident != ident) {
        tms_debugf("last chan was something, but it wasnt our ident! %p %p", sm::channels[snd->last_chan].ident, ident);
    }
#endif
    if (snd->last_chan != -1 && sm::channels[snd->last_chan].ident == ident) {
        Mix_HaltChannel(snd->last_chan);
        snd->reset();

        return true;
    }

    return false;
}

void
sm::pause_all(void)
{
    Mix_Pause(-1);

}

void
sm::resume_all(void)
{
    Mix_Resume(-1);
}

void
sm::stop_all(void)
{
    for (int x=0; x<SM_MAX_CHANNELS; x++) {
        Mix_HaltChannel(x);
    }

    Mix_HaltChannel(SM_MAX_CHANNELS); /* genwave channel */
    sm::gen_started = false;
    for (int x=0; x<SM_MAX_CHANNELS; x++) {
        sm::generated[x].available = true;
    }

    //sm::motor.reset();
    sm::rocket.reset();
    sm::thruster.reset();
    sm::wood_metal.reset();
    sm::wood_wood.reset();
    sm::click.reset();
    sm::robot.reset();
    sm::robot_shoot.reset();
    sm::explosion.reset();
    sm::explosion_light.reset();
    sm::sheet_metal.reset();
    sm::rubber.reset();
    sm::absorb.reset();
    sm::emit.reset();

    for (int x=0; x<SM_MAX_CHANNELS; x++) {
        sm::channels[x].playing = false;
    }
}

void
sm_sound::add_chunk(const char *filename, const char *chunk_name)
{
    if (this->num_chunks < SM_MAX_CHUNKS) {
        this->chunks[this->num_chunks].chunk = Mix_LoadWAV(filename);
        this->chunks[this->num_chunks].name = chunk_name;
        this->num_chunks ++;
    } else {
        tms_errorf("Unable to add chunk '%s', too many chunks loaded for this sound.", filename);
    }
}

void
sm::step(void)
{
    for (int x=0; x<SM_MAX_CHANNELS; x++) {
        if (sm::channels[x].playing) {
            Mix_Volume(x, (int)(roundf(tclampf(channels[x].volume, 0.f, 1.f)*MIX_MAX_VOLUME)));
            channels[x].update_position();
        }
    }
}

void
sm_channel::update_position()
{
    if (this->global) {
        Mix_SetPosition(this->chan, 0, 0);
    } else {
        tvec2 rel = tvec2_sub(this->position, sm::position);
        float dist = tvec2_magnitude(&rel);

        rel.y = SM_FRONT_BIAS;
        float angle = atan2(rel.y, rel.x);
        angle = fabsf(fmodf(angle, M_PI*2.f)) * 180./M_PI;

        Sint16 s_a = (Sint16)roundf(angle)+90;

        //Sint16 s_a = (Sint16)roundf(copysignf(90.f, rel.x))+180;
        Uint8  s_d = (Uint8)(roundf(tclampf(dist * SM_DIST_FACTOR, 0.f, 255.f)));
        this->distance = s_d;

        Mix_SetPosition(this->chan, s_a, s_d);
    }
}

#else

// Dummy functions when sound is disabled

void sm::play_gen(int x) { }
void sm::load_settings() { }
void sm::init() {}
void sm::play(sm_sound *snd, float x, float y, uint8_t random, float volume, bool loop, void *ident, bool global) { }
bool sm::stop(sm_sound *snd, void *ident) { return false; }
void sm::pause_all(void) { }
void sm::resume_all(void) { }
void sm::stop_all(void) { }
void sm::step(void) { }

#endif
