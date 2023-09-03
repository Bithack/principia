#include <unistd.h>
#include <sys/time.h>

#include <tms/core/project.h>
#include <tms/core/event.h>
#include <tms/core/tms.h>
#include <jni.h>
#include <tms/backend/opengl.h>

#include "SDL/src/video/android/SDL_androidvideo.h"
#include "SDL/src/core/android/SDL_android.h"

SDL_Window *_window;

int keys[235];
int mouse_down[64];

static int T_intercept_input(SDL_Event ev);

void tgen_init(void){};
void restore();

int
SDL_main(int argc, char **argv)
{
    SDL_Event  ev;
    int        done = 0;
    int do_step = 1;

    //Android_JNI_SetupThread();

    tms_init();

    if (tms.screen == 0)
        tms_fatalf("context has no initial screen, bailing out");

    do {
        int i;

        if (do_step) {
            for (i = 0; i < 235; ++i) {
                if (keys[i] == 1) {
                    struct tms_event spec;
                    spec.type = TMS_EV_KEY_DOWN;
                    spec.data.key.keycode = i;

                    tms_event_push(spec);
                }
            }
        }

        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_WINDOWEVENT:
                    {
                        switch (ev.window.event) {
                            /*
                            case SDL_WINDOWEVENT_FOCUS_LOST:
                                tms_infof("FOCUS LOST, HARD PAUSE");
                                tproject_pause();
                                do_step = 0;
                                break;
                                */

                            case SDL_WINDOWEVENT_MINIMIZED:
                                tms_infof("MINIMIZED, SOFT PAUSE");
                                tproject_soft_pause();
                                do_step = 0;
                                break;

                                /*
                            case SDL_WINDOWEVENT_FOCUS_GAINED:
                                tms_infof("FOCUS GAINED, HARD RESUME");
                                tproject_resume();
                                do_step = 1;
                                break;
                                */

                            case SDL_WINDOWEVENT_RESTORED:
                                tms_infof("WINDOW RESTORED, SOFT RESUME");
                                tproject_soft_resume();
                                do_step = 1;
                                break;
                        }
                    }
                    break;

                case SDL_QUIT:
                    tproject_quit();
                    tms.state = TMS_STATE_QUITTING;
                    tms_infof("QUIT  ---------------");
                    //done = 1;
                    break;

                case SDL_KEYDOWN:
                    T_intercept_input(ev);
                    keys[ev.key.keysym.scancode] = 1;
                    break;

                case SDL_KEYUP:
                    T_intercept_input(ev);
                    keys[ev.key.keysym.scancode] = 0;
                    break;

                case SDL_FINGERMOTION:
                case SDL_FINGERDOWN:
                case SDL_FINGERUP:
                    T_intercept_input(ev);
                    break;

                default:
                    //tms_debugf("Unhandled event: %d", ev.type);
                    break;
            }
        }

        if (_tms.is_paused == 0) {
            tms_step();
            tms_begin_frame();
            tms_render();
            SDL_GL_SwapWindow(_window);
            tms_end_frame();
        } else {
            SDL_Delay(100);
        }
    } while (tms.state != TMS_STATE_QUITTING);

    SDL_DestroyWindow(tms._window);

    return 0;
}

int
tbackend_init_surface()
{
    /* Set up SDL, create a window */
    tms_infof("Creating window...");

    SDL_Init(SDL_INIT_VIDEO);
    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(0, &mode);

    _window = SDL_CreateWindow("Principia", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            mode.w, mode.h,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);

    if (_window == NULL) {
        tms_fatalf("Could not create SDL Window: %s", SDL_GetError());
    }

    SDL_SetWindowFullscreen(_window, SDL_TRUE);
    SDL_GetWindowSize(_window, &_tms.window_width, &_tms.window_height);

    tms._window = _window;
    tms.xppcm = Android_ScreenDensityX / 2.54f;
    tms.yppcm = Android_ScreenDensityY / 2.54f;

    tms_infof("Device dimensions: %d %d", _tms.window_width, _tms.window_height);
    tms_infof("Device PPCM: %f %f", tms.xppcm, tms.yppcm);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    //SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    //
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    /*
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);
    */
    //SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    SDL_GL_CreateContext(_window);

    SDL_GL_SetSwapInterval(0);

    return T_OK;
}

int
T_intercept_input(SDL_Event ev)
{
    struct tms_event spec;
    spec.type = -1;

    switch (ev.type) {
        case SDL_KEYDOWN:
            if (keys[ev.key.keysym.scancode] == 1) {
                return T_OK;
            }

            spec.type = TMS_EV_KEY_PRESS;
            spec.data.key.keycode = ev.key.keysym.scancode;
            spec.data.key.mod = ev.key.keysym.mod;
            break;

        case SDL_KEYUP:
            spec.type = TMS_EV_KEY_UP;
            spec.data.key.keycode = ev.key.keysym.scancode;
            spec.data.key.mod = ev.key.keysym.mod;
            break;

        case SDL_FINGERDOWN:
            spec.type = TMS_EV_POINTER_DOWN;
            spec.data.button.pointer_id = ev.tfinger.fingerId;
            //tms_infof("event: %d %d", ev.tfinger.x, ev.tfinger.y);
            spec.data.button.x = ((float)ev.tfinger.x / 32768.f)*(float)_tms.window_width;
            spec.data.button.y = tms.window_height - ((float)ev.tfinger.y / 32768.f) * (float)_tms.window_height;
            //tms_infof("event: %f %f", spec.data.button.x, spec.data.button.y);
            break;

        case SDL_FINGERUP:
            spec.type = TMS_EV_POINTER_UP;
            spec.data.button.pointer_id = ev.tfinger.fingerId;
            spec.data.button.x = ((float)ev.tfinger.x / 32768.f)*(float)_tms.window_width;
            spec.data.button.y = tms.window_height - ((float)ev.tfinger.y / 32768.f) * (float)_tms.window_height;
            break;

        case SDL_FINGERMOTION:
            spec.type = TMS_EV_POINTER_DRAG;
            spec.data.button.pointer_id = ev.tfinger.fingerId;
            spec.data.motion.x = ((float)ev.tfinger.x / 32768.f)*(float)_tms.window_width;
            spec.data.motion.y = tms.window_height - ((float)ev.tfinger.y / 32768.f) * (float)_tms.window_height;
            break;
    }

    tms_event_push(spec);

    return T_OK;
}

static char storage_path[1024];
static const char*
_JNI_get_storage_path()
{
    JNIEnv *mEnv = Android_JNI_GetEnv();
    jclass cls = Android_JNI_GetActivityClass();

    jmethodID mid = (*mEnv)->GetStaticMethodID(mEnv, cls, "get_storage_path", "()Ljava/lang/String;");
    if (mid) {
        jstring s = (*mEnv)->CallStaticObjectMethod(mEnv, cls, mid);

        const char *tmp = (*mEnv)->GetStringUTFChars(mEnv, s, 0);

        tms_infof("Storage path: %s", tmp ? tmp : "<NULL>");

        strcpy(storage_path, tmp);

        (*mEnv)->ReleaseStringUTFChars(mEnv, s, tmp);
    } else {
        strcpy(storage_path, "");
    }

    return storage_path;
}

const char *tbackend_get_storage_path(void)
{
    return _JNI_get_storage_path();
}

void
tbackend_toggle_fullscreen(void)
{
}
