#include "ui.hh"
#include <SDL.h>

#include "main.hh"
#include "game.hh"
#include "menu_main.hh"
#include "menu-play.hh"
#include "loading_screen.hh"
#include "game-message.hh"
#include "beam.hh"
#include "wheel.hh"
#include "pixel.hh"
#include "command.hh"
#include "i1o1gate.hh"
#include "pkgman.hh"
#include "object_factory.hh"
#include "box.hh"
#include "settings.hh"
#include "fxemitter.hh"
#include "i0o1gate.hh"
#include "i2o0gate.hh"
#include "display.hh"
#include "prompt.hh"
#include "robot_base.hh"
#include "adventure.hh"
#include "speaker.hh"
#include "timer.hh"
#include "jumper.hh"
#include "item.hh"
#include "escript.hh"
#include "tpixel.hh"
#include "factory.hh"
#include "faction.hh"
#include "anchor.hh"
#include "resource.hh"
#include "animal.hh"
#include "simplebg.hh"
#include "soundman.hh"
#include "polygon.hh"
#include "treasure_chest.hh"
#include "decorations.hh"
#include "sequencer.hh"
#include "sfxemitter.hh"
#include "key_listener.hh"
#include "soundmanager.hh"

#include <tms/core/tms.h>
#ifdef BUILD_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include <sstream>

#define SAVE_REGULAR 0
#define SAVE_COPY 1

#define MAX_GRAVITY 75.f

const char *tips[] = {
#ifdef TMS_BACKEND_PC
"Double-click"
#else
"Double-tap"
#endif
" and drag from an electronic object to another electronic object to"
" automatically create and add an appropriate cable between them.",

#ifdef TMS_BACKEND_PC
"Press space to quickadd objects by typing parts of their name. For example, press space, type 'cy' and press Enter to add a cylinder. Double-press space to add the last added object.",
#endif

    "You can copy an object by selecting it and then adding a new object of the same type. All properties, the rotation and the layer will be copied to the new object. For example, select a rotated plank and then add a new plank. The new plank will have the same size and rotation as the previously selected plank. Create another plank and it too will get the same properties.",

    "When you publish a level to the community website, a screenshot will be taken at the current position of the camera. Use a Cam Marker to specify an exact location where the screenshot should be taken. Use many Cam Markers to give your level multiple screenshots.",

    "If you want to automatically activate an RC when the level is started, use the RC Activator object.",

    "Building something mechanically advanced? If it gets unstable or wobbly, try increasing physics iterations count in the Level Properties dialog. The velocity iterations number will affect joint parts (motors, linear motors, etc), while position iterations affects at what precision objects collide and interact, roughly speaking."
};

const int num_tips = sizeof(tips)/sizeof(char*);
int ctip = -1;
int ui::next_action = ACTION_IGNORE;

void
ui::message(const char *msg, bool long_duration)
{
#ifndef NO_UI
    pscreen::message->show(msg, long_duration ? 5.0 : 2.5);
#endif
}

/* always assume short duration */
void
ui::messagef(const char *format, ...)
{
    va_list vl;
    va_start(vl, format);

    char short_msg[256];
    const size_t sz = vsnprintf(short_msg, sizeof short_msg, format, vl) + 1;
    if (sz <= sizeof short_msg) {
        ui::message(short_msg, false);
    } else {
        char *long_msg = (char*)malloc(sz);
        vsnprintf(long_msg, sz, format, vl);
        ui::message(long_msg, false);
    }
}

#if !defined(PRINCIPIA_BACKEND_IMGUI)
void ui::render(){};
#endif

#if defined(NO_UI) || defined(TMS_BACKEND_EMSCRIPTEN)

int prompt_is_open = 0;
void ui::init(){};
void ui::open_dialog(int num, void *data/*=0*/){}
void ui::open_sandbox_tips(){};
void ui::open_url(const char *url){};
void ui::open_help_dialog(const char*, const char*){};
void ui::emit_signal(int num, void *data/*=0*/){};
void ui::quit(){};
void ui::set_next_action(int action_id){};
void ui::open_error_dialog(const char *error_msg){};
void
ui::confirm(const char *text,
        const char *button1, principia_action action1,
        const char *button2, principia_action action2,
        const char *button3/*=0*/, principia_action action3/*=ACTION_IGNORE*/,
        struct confirm_data _confirm_data/*=none*/
        )
{
    P.add_action(action1.action_id, 0);
}
void ui::alert(const char*, uint8_t/*=ALERT_INFORMATION*/) {};

#elif defined(PRINCIPIA_BACKEND_IMGUI)

#include "ui_imgui.hh"

#elif defined(TMS_BACKEND_IOS)

#include "ui_ios.hh"

#elif defined(TMS_BACKEND_ANDROID)

#include "ui_android.hh"

#elif defined(TMS_BACKEND_PC)

#include "ui_gtk3.hh"

#else

#error "No dialog functions, to compile without please define NO_UI"

#endif
