#include "ui.hh"
#include <SDL.h>

#include "main.hh"
#include "game.hh"
#include "game-message.hh"

#include <tms/core/tms.h>

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

bool prompt_is_open = false;

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

#ifndef TMS_BACKEND_ANDROID
void ui::open_url(const char *url)
{
    ui::messagef("Opening the page in your web browser...", url);
    SDL_OpenURL(url);
}
#endif
