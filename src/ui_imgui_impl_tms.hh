//ImGui Platform backend implementation for TMS+SDL2

#pragma once

#include <SDL.h>
#include "SDL_clipboard.h"
#include "SDL_stdinc.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "imgui_impl_opengl3.h"

#include "tms/core/tms.h"
#include "tms/core/event.h"
#include "tms/core/err.h"

static int tms_mouse_button_to_imgui(int btn) {
    switch (btn) {
        case 1: return 0;
        case 2: return 2;
        case 3: return 1;
        default: return -1;
    }
}

static ImGuiKey tms_key_to_imgui(int keycode) {
    switch (keycode) {
        case TMS_KEY_NONE: return ImGuiKey_None;
        case TMS_KEY_A: return ImGuiKey_A;
        case TMS_KEY_B: return ImGuiKey_B;
        case TMS_KEY_C: return ImGuiKey_C;
        case TMS_KEY_D: return ImGuiKey_D;
        case TMS_KEY_E: return ImGuiKey_E;
        case TMS_KEY_F: return ImGuiKey_F;
        case TMS_KEY_G: return ImGuiKey_G;
        case TMS_KEY_H: return ImGuiKey_H;
        case TMS_KEY_I: return ImGuiKey_I;
        case TMS_KEY_J: return ImGuiKey_J;
        case TMS_KEY_K: return ImGuiKey_K;
        case TMS_KEY_L: return ImGuiKey_L;
        case TMS_KEY_M: return ImGuiKey_M;
        case TMS_KEY_N: return ImGuiKey_N;
        case TMS_KEY_O: return ImGuiKey_O;
        case TMS_KEY_P: return ImGuiKey_P;
        case TMS_KEY_Q: return ImGuiKey_Q;
        case TMS_KEY_R: return ImGuiKey_R;
        case TMS_KEY_S: return ImGuiKey_S;
        case TMS_KEY_T: return ImGuiKey_T;
        case TMS_KEY_U: return ImGuiKey_U;
        case TMS_KEY_V: return ImGuiKey_V;
        case TMS_KEY_W: return ImGuiKey_W;
        case TMS_KEY_X: return ImGuiKey_X;
        case TMS_KEY_Y: return ImGuiKey_Y;
        case TMS_KEY_Z: return ImGuiKey_Z;
        case TMS_KEY_1: return ImGuiKey_1;
        case TMS_KEY_2: return ImGuiKey_2;
        case TMS_KEY_3: return ImGuiKey_3;
        case TMS_KEY_4: return ImGuiKey_4;
        case TMS_KEY_5: return ImGuiKey_5;
        case TMS_KEY_6: return ImGuiKey_6;
        case TMS_KEY_7: return ImGuiKey_7;
        case TMS_KEY_8: return ImGuiKey_8;
        case TMS_KEY_9: return ImGuiKey_9;
        case TMS_KEY_0: return ImGuiKey_0;
        case TMS_KEY_ENTER: return ImGuiKey_Enter;
        case TMS_KEY_ESC: return ImGuiKey_Escape;
        case TMS_KEY_BACKSPACE: return ImGuiKey_Backspace;
        case TMS_KEY_TAB: return ImGuiKey_Tab;
        case TMS_KEY_SPACE: return ImGuiKey_Space;
        case TMS_KEY_MINUS: return ImGuiKey_Minus;
        case TMS_KEY_EQUALS: return ImGuiKey_Equal;
        case TMS_KEY_LBRACKET: return ImGuiKey_LeftBracket;
        case TMS_KEY_RBRACKET: return ImGuiKey_RightBracket;
        case TMS_KEY_BACKSLASH: return ImGuiKey_Backslash;
        case TMS_KEY_SEMICOLON: return ImGuiKey_Semicolon;
        case TMS_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
        case TMS_KEY_GRAVE: return ImGuiKey_GraveAccent;
        case TMS_KEY_COMMA: return ImGuiKey_Comma;
        case TMS_KEY_PERIOD: return ImGuiKey_Period;
        case TMS_KEY_FRONTSLASH: return ImGuiKey_Slash;
        case TMS_KEY_F1: return ImGuiKey_F1;
        case TMS_KEY_F2: return ImGuiKey_F2;
        case TMS_KEY_F3: return ImGuiKey_F3;
        case TMS_KEY_F4: return ImGuiKey_F4;
        case TMS_KEY_F5: return ImGuiKey_F5;
        case TMS_KEY_F6: return ImGuiKey_F6;
        case TMS_KEY_F7: return ImGuiKey_F7;
        case TMS_KEY_F8: return ImGuiKey_F8;
        case TMS_KEY_F9: return ImGuiKey_F9;
        case TMS_KEY_F10: return ImGuiKey_F10;
        case TMS_KEY_F11: return ImGuiKey_F11;
        case TMS_KEY_F12: return ImGuiKey_F12;
        case TMS_KEY_SCROLLLOCK: return ImGuiKey_ScrollLock;
        case TMS_KEY_PAUSE: return ImGuiKey_Pause;
        case TMS_KEY_INSERT: return ImGuiKey_Insert;
        case TMS_KEY_HOME: return ImGuiKey_Home;
        case TMS_KEY_PAGEUP: return ImGuiKey_PageUp;
        case TMS_KEY_DELETE: return ImGuiKey_Delete;
        case TMS_KEY_END: return ImGuiKey_End;
        case TMS_KEY_PAGEDOWN: return ImGuiKey_PageDown;
        case TMS_KEY_RIGHT: return ImGuiKey_RightArrow;
        case TMS_KEY_LEFT: return ImGuiKey_LeftArrow;
        case TMS_KEY_DOWN: return ImGuiKey_DownArrow;
        case TMS_KEY_UP: return ImGuiKey_UpArrow;
        case TMS_KEY_MENU: return ImGuiKey_Menu;
        case TMS_KEY_LEFT_CTRL: return ImGuiKey_LeftCtrl;
        case TMS_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
        case TMS_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
        case TMS_KEY_LEFT_META: return ImGuiKey_LeftSuper;
        case TMS_KEY_RIGHT_CTRL: return ImGuiKey_RightCtrl;
        case TMS_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
        case TMS_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
        case TMS_KEY_RIGHT_META: return ImGuiKey_LeftSuper;
        default: return ImGuiKey_None;
    }
}

static int event_handler(tms_event *event) {
    ImGuiIO& io = ImGui::GetIO();
    switch (event->type) {
        //TODO handle touch events
        //Principia-specific bullshit events. Just block them if needed.
        case TMS_EV_KEY_DOWN:
        case TMS_EV_KEY_REPEAT: {
            return io.WantCaptureKeyboard ? T_OK : T_CONT;
        }
        case TMS_EV_KEY_PRESS:
        case TMS_EV_KEY_UP: {
            io.AddKeyEvent(ImGuiMod_Shift, (event->data.key.mod & TMS_MOD_SHIFT) != 0);
            io.AddKeyEvent(ImGuiMod_Ctrl, (event->data.key.mod & TMS_MOD_CTRL) != 0);
            io.AddKeyEvent(ImGuiMod_Alt, (event->data.key.mod & TMS_MOD_ALT) != 0);
            io.AddKeyEvent(ImGuiMod_Super, (event->data.key.mod & TMS_MOD_GUI) != 0);
            ImGuiKey keycode = tms_key_to_imgui(event->data.key.keycode);
            io.AddKeyEvent(keycode, (event->type & (TMS_EV_KEY_PRESS | TMS_EV_KEY_DOWN)) != 0);
            //XXX: we won't bother supporting SetKeyEventNativeData, as it's only used by legacy user code
            return io.WantCaptureKeyboard ? T_OK : T_CONT;
        }
        case TMS_EV_POINTER_DOWN:
        case TMS_EV_POINTER_UP: {
            int mouse_button = tms_mouse_button_to_imgui(event->data.button.button);
            if (mouse_button >= 0) {
                io.AddMouseButtonEvent(mouse_button, event->type == TMS_EV_POINTER_DOWN);
            }
            return io.WantCaptureMouse ? T_OK : T_CONT;
        }
        case TMS_EV_POINTER_DRAG:
        case TMS_EV_POINTER_MOVE: {
            // why the fuck is the tms y axis upside-down
            io.AddMousePosEvent(event->data.motion.x, io.DisplaySize.y - event->data.motion.y);
            return io.WantCaptureMouse ? T_OK : T_CONT;
        }
        case TMS_EV_POINTER_SCROLL: {
            io.AddMouseWheelEvent(event->data.scroll.x, event->data.scroll.y);
            return io.WantCaptureMouse ? T_OK : T_CONT;
        }
        case TMS_EV_TEXT_INPUT: {
            io.AddInputCharactersUTF8(event->data.text.text);
            return io.WantCaptureKeyboard ? T_OK : T_CONT;
        }
    }
    return T_CONT;
}

//SDL2 impls:
static const char* GetClipboardTextFn(void* _cbt) {
    char* cbt = (char*)_cbt;
    if (cbt) SDL_free(cbt);
    cbt = SDL_GetClipboardText();
    if (*cbt == 0) return nullptr;
    return cbt;
}
inline static void SetClipboardTextFn(void*, const char* text) {
    SDL_SetClipboardText(text);
}

inline const void init_io() {
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = "tms";
    io.ClipboardUserData = nullptr;
    io.GetClipboardTextFn = GetClipboardTextFn;
    io.SetClipboardTextFn = SetClipboardTextFn;

    //set PlatformHandleRaw
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandleRaw = nullptr;
#if defined(TMS_BACKEND_WINDOWS)
    SDL_SysWMinfo info;
    if (SDL_GetWindowWMInfo((SDL_Window*) _tms._window, &info)) {
        main_viewport->PlatformHandleRaw = (void*)info.info.win.window;
    }
#endif
}

inline int ImGui_ImplTMS_Init_Platform() {
    init_io();
    return tms_event_register_raw(&event_handler);
}

//TODO tms-specific opengl impl. While default opengl3 impl works just fine, it basically leads to code duplication.
// inline int ImGui_ImplTMS_Init_Gfx() {
// }

inline int ImGui_ImplTMS_Init() {
    int result = ImGui_ImplTMS_Init_Platform();
    if (result != T_OK) return result;
    // result = ImGui_ImplTMS_Init_Gfx();
    // if (result != T_OK) return result;
    return T_OK;
}

inline int ImGui_ImplTMS_Shutdown() {
    //currently a no-op, will be used by the gfx impl
    return T_OK;
}

///Call BEFORE ImGui_ImplOpenGL3_NewFrame
inline int ImGui_ImplTms_NewFrame() {
    ImGuiIO& io = ImGui::GetIO();

    //update window size
    if ((_tms.window_width > 0) && (_tms.window_height > 0)) {
        io.DisplaySize = ImVec2((float) _tms.window_width, (float) _tms.window_height);
        io.DisplayFramebufferScale = ImVec2(1., 1.);
    } else {
        tms_errorf("window size is 0");
        return -1;
    }

    return 1;
}
