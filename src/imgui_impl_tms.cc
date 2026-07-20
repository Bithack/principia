#include "imgui.h"
#include "settings.hh"
#include <tms/cpp.hh>
#include <SDL3/SDL.h>

#if defined(PRINCIPIA_BACKEND_IMGUI)

// This is a modified version of the SDL3 platform implementation for Dear Imgui,
// which rewrites some things (notably events) to work more tightly with TMS.

enum ImGui_ImplSDL3_GamepadMode { ImGui_ImplSDL3_GamepadMode_AutoFirst, ImGui_ImplSDL3_GamepadMode_AutoAll, ImGui_ImplSDL3_GamepadMode_Manual };
enum ImGui_ImplSDL3_MouseCaptureMode { ImGui_ImplSDL3_MouseCaptureMode_Enabled, ImGui_ImplSDL3_MouseCaptureMode_EnabledAfterDrag, ImGui_ImplSDL3_MouseCaptureMode_Disabled };

// Clang warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"                 // warning: use of old-style cast
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#endif

#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
#define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE    1
#else
#define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE    0
#endif

// SDL Data
struct ImGui_ImplSDL3_Data
{
    SDL_Window*             Window;
    SDL_WindowID            WindowID;
    Uint64                  Time;
    char*                   ClipboardTextData;

    // IME handling
    SDL_Window*             ImeWindow;
    ImGuiPlatformImeData    ImeData;
    bool                    ImeDirty;

    // Mouse handling
    Uint32                  MouseWindowID;
    int                     MouseButtonsDown;
    SDL_Cursor*             MouseCursors[ImGuiMouseCursor_COUNT];
    SDL_Cursor*             MouseLastCursor;
    int                     MousePendingLeaveFrame;
    bool                    MouseCanUseGlobalState;
    ImGui_ImplSDL3_MouseCaptureMode MouseCaptureMode;

    // Gamepad handling
    ImVector<SDL_Gamepad*>      Gamepads;
    ImGui_ImplSDL3_GamepadMode  GamepadMode;
    bool                        WantUpdateGamepadsList;

    ImGui_ImplSDL3_Data()   { memset((void*)this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
// FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using multi-context.
static ImGui_ImplSDL3_Data* ImGui_ImplSDL3_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplSDL3_Data*)ImGui::GetIO().BackendPlatformUserData : nullptr;
}

// Forward Declarations
static void ImGui_ImplSDL3_UpdateIme();

// Functions
static const char* ImGui_ImplSDL3_GetClipboardText(ImGuiContext*)
{
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();
    if (bd->ClipboardTextData)
        SDL_free(bd->ClipboardTextData);
    if (SDL_HasClipboardText())
        bd->ClipboardTextData = SDL_GetClipboardText();
    else
        bd->ClipboardTextData = nullptr;
    return bd->ClipboardTextData;
}

static void ImGui_ImplSDL3_SetClipboardText(ImGuiContext*, const char* text)
{
    SDL_SetClipboardText(text);
}

static ImGuiViewport* ImGui_ImplSDL3_GetViewportForWindowID(SDL_WindowID window_id)
{
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();
    return (window_id == bd->WindowID) ? ImGui::GetMainViewport() : nullptr;
}

static void ImGui_ImplSDL3_PlatformSetImeData(ImGuiContext*, ImGuiViewport*, ImGuiPlatformImeData* data)
{
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();
    bd->ImeData = *data;
    bd->ImeDirty = true;
    ImGui_ImplSDL3_UpdateIme();
}

// We discard viewport passed via ImGuiPlatformImeData and always call SDL_StartTextInput() on SDL_GetKeyboardFocus().
static void ImGui_ImplSDL3_UpdateIme()
{
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();
    ImGuiPlatformImeData* data = &bd->ImeData;
    SDL_Window* window = SDL_GetKeyboardFocus();

    // Stop previous input
    if ((!(data->WantVisible || data->WantTextInput) || bd->ImeWindow != window) && bd->ImeWindow != nullptr)
    {
        SDL_StopTextInput(bd->ImeWindow);
        bd->ImeWindow = nullptr;
    }
    if ((!bd->ImeDirty && bd->ImeWindow == window) || (window == nullptr))
        return;

    // Start/update current input
    bd->ImeDirty = false;
    if (data->WantVisible)
    {
        SDL_Rect r;
        r.x = (int)data->InputPos.x;
        r.y = (int)data->InputPos.y;
        r.w = 1;
        r.h = (int)data->InputLineHeight;
        SDL_SetTextInputArea(window, &r, 0);
        bd->ImeWindow = window;
    }
    if (!SDL_TextInputActive(window) && (data->WantVisible || data->WantTextInput))
        SDL_StartTextInput(window);
}

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
    bool touch_controls = settings["touch_controls"]->v.b;
    switch (event->type) {
        //TODO handle touch events
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
            int mouse_button = event->data.button.pointer_id;
            if (mouse_button >= 0 && mouse_button < 5) {
                io.AddMouseSourceEvent(touch_controls ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
                io.AddMousePosEvent(event->data.button.x, io.DisplaySize.y - event->data.button.y);
                io.AddMouseButtonEvent(mouse_button, event->type == TMS_EV_POINTER_DOWN);
            }
            // bd->MouseButtonsDown = (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? (bd->MouseButtonsDown | (1 << mouse_button)) : (bd->MouseButtonsDown & ~(1 << mouse_button));
            return io.WantCaptureMouse ? T_OK : T_CONT;
        }
        case TMS_EV_POINTER_DRAG:
        case TMS_EV_POINTER_MOVE: {
            io.AddMouseSourceEvent(touch_controls ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
            // why the fuck is the tms y axis upside-down
            io.AddMousePosEvent(event->data.motion.x, io.DisplaySize.y - event->data.motion.y);
            return io.WantCaptureMouse ? T_OK : T_CONT;
        }
        case TMS_EV_POINTER_SCROLL: {
            io.AddMouseSourceEvent(touch_controls ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
            io.AddMouseWheelEvent(event->data.scroll.x, event->data.scroll.y);
            return io.WantCaptureMouse ? T_OK : T_CONT;
        }
        case TMS_EV_TEXT_INPUT: {
            io.AddInputCharactersUTF8(event->data.text.text);
            return io.WantCaptureKeyboard ? T_OK : T_CONT;
        }

#if 0
        case SDL_EVENT_GAMEPAD_ADDED:
        case SDL_EVENT_GAMEPAD_REMOVED: {
            bd->WantUpdateGamepadsList = true;
            return true;
        }
#endif
    }
    return T_CONT;
}

static void ImGui_ImplSDL3_SetupPlatformHandles(ImGuiViewport* viewport, SDL_Window* window)
{
}

bool ImGui_ImplSDL3_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");
    //SDL_SetHint(SDL_HINT_EVENT_LOGGING, "2");

    const int ver_linked = SDL_GetVersion();

    // Setup backend capabilities flags
    ImGui_ImplSDL3_Data* bd = IM_NEW(ImGui_ImplSDL3_Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "tms (SDL3)";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;           // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;            // We can honor io.WantSetMousePos requests (optional, rarely used)

    bd->Window = _tms._window;
    bd->WindowID = SDL_GetWindowID(_tms._window);

    // Check and store if we are on a SDL backend that supports SDL_GetGlobalMouseState() and SDL_CaptureMouse()
    // ("wayland" and "rpi" don't support it, but we chose to use a white-list instead of a black-list)
    bd->MouseCanUseGlobalState = false;
    bd->MouseCaptureMode = ImGui_ImplSDL3_MouseCaptureMode_Disabled;
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
    const char* sdl_backend = SDL_GetCurrentVideoDriver();
    const char* capture_and_global_state_whitelist[] = { "windows", "cocoa", "x11", "DIVE", "VMAN" };
    for (const char* item : capture_and_global_state_whitelist)
        if (strncmp(sdl_backend, item, strlen(item)) == 0)
        {
            bd->MouseCanUseGlobalState = true;
            bd->MouseCaptureMode = (strcmp(item, "x11") == 0) ? ImGui_ImplSDL3_MouseCaptureMode_EnabledAfterDrag : ImGui_ImplSDL3_MouseCaptureMode_Enabled;
        }
#endif

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_SetClipboardTextFn = ImGui_ImplSDL3_SetClipboardText;
    platform_io.Platform_GetClipboardTextFn = ImGui_ImplSDL3_GetClipboardText;
    platform_io.Platform_SetImeDataFn = ImGui_ImplSDL3_PlatformSetImeData;
    platform_io.Platform_OpenInShellFn = [](ImGuiContext*, const char* url) { return SDL_OpenURL(url); };

    // Gamepad handling
    bd->GamepadMode = ImGui_ImplSDL3_GamepadMode_AutoFirst;
    bd->WantUpdateGamepadsList = true;

    // Load mouse cursors
    bd->MouseCursors[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    bd->MouseCursors[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
    bd->MouseCursors[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
    bd->MouseCursors[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NESW_RESIZE);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE);
    bd->MouseCursors[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    bd->MouseCursors[ImGuiMouseCursor_Wait] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
    bd->MouseCursors[ImGuiMouseCursor_Progress] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_PROGRESS);
    bd->MouseCursors[ImGuiMouseCursor_NotAllowed] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NOT_ALLOWED);

    // Set platform dependent data in viewport
    // Our mouse update function expect PlatformHandle to be filled for the main viewport
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = (void*)(intptr_t)SDL_GetWindowID(_tms._window);
    // PlatformHandleRaw is never used by imgui, so don't do anything with it

    // From 2.0.5: Set SDL hint to receive mouse click events on window focus, otherwise SDL doesn't emit the event.
    // Without this, when clicking to gain focus, our widgets wouldn't activate even though they showed as hovered.
    // (This is unfortunately a global SDL setting, so enabling it might have a side-effect on your application.
    // It is unlikely to make a difference, but if your app absolutely needs to ignore the initial on-focus click:
    // you can ignore SDL_EVENT_MOUSE_BUTTON_DOWN events coming right after a SDL_EVENT_WINDOW_FOCUS_GAINED)
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

    // From 2.0.22: Disable auto-capture, this is preventing drag and drop across multiple windows (see #5710)
    SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");

    tms_event_register_raw(&event_handler);

    return true;
}

static void ImGui_ImplSDL3_CloseGamepads();

void ImGui_ImplSDL3_Shutdown()
{
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();
    IM_ASSERT(bd != nullptr && "No platform backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    if (bd->ClipboardTextData)
        SDL_free(bd->ClipboardTextData);
    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
        SDL_DestroyCursor(bd->MouseCursors[cursor_n]);
    ImGui_ImplSDL3_CloseGamepads();

    io.BackendPlatformUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos | ImGuiBackendFlags_HasGamepad);
    platform_io.ClearPlatformHandlers();
    IM_DELETE(bd);
}

void ImGui_ImplSDL3_SetMouseCaptureMode(ImGui_ImplSDL3_MouseCaptureMode mode)
{
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();
    if (mode == ImGui_ImplSDL3_MouseCaptureMode_Disabled && bd->MouseCaptureMode != ImGui_ImplSDL3_MouseCaptureMode_Disabled)
        SDL_CaptureMouse(false);
    bd->MouseCaptureMode = mode;
}

static void ImGui_ImplSDL3_UpdateMouseData()
{
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();
    ImGuiIO& io = ImGui::GetIO();

    // We forward mouse input when hovered or captured (via SDL_EVENT_MOUSE_MOTION) or when focused (below)
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
    // - SDL_CaptureMouse() let the OS know e.g. that our drags can extend outside of parent boundaries (we want updated position) and shouldn't trigger other operations outside.
    // - Debuggers under Linux tends to leave captured mouse on break, which may be very inconvenient, so to mitigate the issue on X11 we we wait until mouse has moved to begin capture.
    if (bd->MouseCaptureMode == ImGui_ImplSDL3_MouseCaptureMode_Enabled)
    {
        SDL_CaptureMouse(bd->MouseButtonsDown != 0);
    }
    else if (bd->MouseCaptureMode == ImGui_ImplSDL3_MouseCaptureMode_EnabledAfterDrag)
    {
        bool want_capture = false;
        for (int button_n = 0; button_n < ImGuiMouseButton_COUNT && !want_capture; button_n++)
            if (ImGui::IsMouseDragging(button_n, 1.0f))
                want_capture = true;
        SDL_CaptureMouse(want_capture);
    }

    SDL_Window* focused_window = SDL_GetKeyboardFocus();
    const bool is_app_focused = (bd->Window == focused_window);
#else
    SDL_Window* focused_window = bd->Window;
    const bool is_app_focused = (SDL_GetWindowFlags(bd->Window) & SDL_WINDOW_INPUT_FOCUS) != 0; // SDL 2.0.3 and non-windowed systems: single-viewport only
#endif
    if (is_app_focused)
    {
        // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when io.ConfigNavMoveSetMousePos is enabled by user)
        if (io.WantSetMousePos)
            SDL_WarpMouseInWindow(bd->Window, io.MousePos.x, io.MousePos.y);

        // (Optional) Fallback to provide unclamped mouse position when focused but not hovered (SDL_EVENT_MOUSE_MOTION already provides this when hovered or captured)
        // Note that SDL_GetGlobalMouseState() is in theory slow on X11, but this only runs on rather specific cases. If a problem we may provide a way to opt-out this feature.
        SDL_Window* hovered_window = SDL_GetMouseFocus();
        const bool is_relative_mouse_mode = SDL_GetWindowRelativeMouseMode(bd->Window);
        if (hovered_window == nullptr && bd->MouseCanUseGlobalState && bd->MouseButtonsDown == 0 && !is_relative_mouse_mode)
        {
            // Single-viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window)
            float mouse_x, mouse_y;
            int window_x, window_y;
            SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
            SDL_GetWindowPosition(focused_window, &window_x, &window_y);
            mouse_x -= (float)window_x;
            mouse_y -= (float)window_y;
            io.AddMousePosEvent(mouse_x, mouse_y);
        }
    }
}

static void ImGui_ImplSDL3_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        SDL_HideCursor();
    }
    else
    {
        // Show OS mouse cursor
        SDL_Cursor* expected_cursor = bd->MouseCursors[imgui_cursor] ? bd->MouseCursors[imgui_cursor] : bd->MouseCursors[ImGuiMouseCursor_Arrow];
        if (bd->MouseLastCursor != expected_cursor)
        {
            SDL_SetCursor(expected_cursor); // SDL function doesn't have an early out (see #6113)
            bd->MouseLastCursor = expected_cursor;
        }
        SDL_ShowCursor();
    }
}

static void ImGui_ImplSDL3_CloseGamepads()
{
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();
    if (bd->GamepadMode != ImGui_ImplSDL3_GamepadMode_Manual)
        for (SDL_Gamepad* gamepad : bd->Gamepads)
            SDL_CloseGamepad(gamepad);
    bd->Gamepads.resize(0);
}

void ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode mode, SDL_Gamepad** manual_gamepads_array, int manual_gamepads_count)
{
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();
    ImGui_ImplSDL3_CloseGamepads();
    if (mode == ImGui_ImplSDL3_GamepadMode_Manual)
    {
        IM_ASSERT(manual_gamepads_array != nullptr || manual_gamepads_count <= 0);
        for (int n = 0; n < manual_gamepads_count; n++)
            bd->Gamepads.push_back(manual_gamepads_array[n]);
    }
    else
    {
        IM_ASSERT(manual_gamepads_array == nullptr && manual_gamepads_count <= 0);
        bd->WantUpdateGamepadsList = true;
    }
    bd->GamepadMode = mode;
}

static void ImGui_ImplSDL3_UpdateGamepadButton(ImGui_ImplSDL3_Data* bd, ImGuiIO& io, ImGuiKey key, SDL_GamepadButton button_no)
{
    bool merged_value = false;
    for (SDL_Gamepad* gamepad : bd->Gamepads)
        merged_value |= SDL_GetGamepadButton(gamepad, button_no) != 0;
    io.AddKeyEvent(key, merged_value);
}

static inline float Saturate(float v) { return v < 0.0f ? 0.0f : v  > 1.0f ? 1.0f : v; }
static void ImGui_ImplSDL3_UpdateGamepadAnalog(ImGui_ImplSDL3_Data* bd, ImGuiIO& io, ImGuiKey key, SDL_GamepadAxis axis_no, float v0, float v1)
{
    float merged_value = 0.0f;
    for (SDL_Gamepad* gamepad : bd->Gamepads)
    {
        float vn = Saturate((float)(SDL_GetGamepadAxis(gamepad, axis_no) - v0) / (float)(v1 - v0));
        if (merged_value < vn)
            merged_value = vn;
    }
    io.AddKeyAnalogEvent(key, merged_value > 0.1f, merged_value);
}

static void ImGui_ImplSDL3_UpdateGamepads()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();

    // Update list of gamepads to use
    if (bd->WantUpdateGamepadsList && bd->GamepadMode != ImGui_ImplSDL3_GamepadMode_Manual)
    {
        ImGui_ImplSDL3_CloseGamepads();
        int sdl_gamepads_count = 0;
        SDL_JoystickID* sdl_gamepads = SDL_GetGamepads(&sdl_gamepads_count);
        for (int n = 0; n < sdl_gamepads_count; n++)
            if (SDL_Gamepad* gamepad = SDL_OpenGamepad(sdl_gamepads[n]))
            {
                bd->Gamepads.push_back(gamepad);
                if (bd->GamepadMode == ImGui_ImplSDL3_GamepadMode_AutoFirst)
                    break;
            }
        bd->WantUpdateGamepadsList = false;
        SDL_free(sdl_gamepads);
    }

    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    if (bd->Gamepads.Size == 0)
        return;
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

    // Update gamepad inputs
    const int thumb_dead_zone = 8000;           // SDL_gamepad.h suggests using this value.
    ImGui_ImplSDL3_UpdateGamepadButton(bd, io, ImGuiKey_GamepadStart,       SDL_GAMEPAD_BUTTON_START);
    ImGui_ImplSDL3_UpdateGamepadButton(bd, io, ImGuiKey_GamepadBack,        SDL_GAMEPAD_BUTTON_BACK);
    ImGui_ImplSDL3_UpdateGamepadButton(bd, io, ImGuiKey_GamepadFaceLeft,    SDL_GAMEPAD_BUTTON_WEST);           // Xbox X, PS Square
    ImGui_ImplSDL3_UpdateGamepadButton(bd, io, ImGuiKey_GamepadFaceRight,   SDL_GAMEPAD_BUTTON_EAST);           // Xbox B, PS Circle
    ImGui_ImplSDL3_UpdateGamepadButton(bd, io, ImGuiKey_GamepadFaceUp,      SDL_GAMEPAD_BUTTON_NORTH);          // Xbox Y, PS Triangle
    ImGui_ImplSDL3_UpdateGamepadButton(bd, io, ImGuiKey_GamepadFaceDown,    SDL_GAMEPAD_BUTTON_SOUTH);          // Xbox A, PS Cross
    ImGui_ImplSDL3_UpdateGamepadButton(bd, io, ImGuiKey_GamepadDpadLeft,    SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    ImGui_ImplSDL3_UpdateGamepadButton(bd, io, ImGuiKey_GamepadDpadRight,   SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    ImGui_ImplSDL3_UpdateGamepadButton(bd, io, ImGuiKey_GamepadDpadUp,      SDL_GAMEPAD_BUTTON_DPAD_UP);
    ImGui_ImplSDL3_UpdateGamepadButton(bd, io, ImGuiKey_GamepadDpadDown,    SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    ImGui_ImplSDL3_UpdateGamepadButton(bd, io, ImGuiKey_GamepadL1,          SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    ImGui_ImplSDL3_UpdateGamepadButton(bd, io, ImGuiKey_GamepadR1,          SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
    ImGui_ImplSDL3_UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadL2,          SDL_GAMEPAD_AXIS_LEFT_TRIGGER,  0.0f, 32767);
    ImGui_ImplSDL3_UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadR2,          SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0.0f, 32767);
    ImGui_ImplSDL3_UpdateGamepadButton(bd, io, ImGuiKey_GamepadL3,          SDL_GAMEPAD_BUTTON_LEFT_STICK);
    ImGui_ImplSDL3_UpdateGamepadButton(bd, io, ImGuiKey_GamepadR3,          SDL_GAMEPAD_BUTTON_RIGHT_STICK);
    ImGui_ImplSDL3_UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadLStickLeft,  SDL_GAMEPAD_AXIS_LEFTX,  -thumb_dead_zone, -32768);
    ImGui_ImplSDL3_UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadLStickRight, SDL_GAMEPAD_AXIS_LEFTX,  +thumb_dead_zone, +32767);
    ImGui_ImplSDL3_UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadLStickUp,    SDL_GAMEPAD_AXIS_LEFTY,  -thumb_dead_zone, -32768);
    ImGui_ImplSDL3_UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadLStickDown,  SDL_GAMEPAD_AXIS_LEFTY,  +thumb_dead_zone, +32767);
    ImGui_ImplSDL3_UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadRStickLeft,  SDL_GAMEPAD_AXIS_RIGHTX, -thumb_dead_zone, -32768);
    ImGui_ImplSDL3_UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadRStickRight, SDL_GAMEPAD_AXIS_RIGHTX, +thumb_dead_zone, +32767);
    ImGui_ImplSDL3_UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadRStickUp,    SDL_GAMEPAD_AXIS_RIGHTY, -thumb_dead_zone, -32768);
    ImGui_ImplSDL3_UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadRStickDown,  SDL_GAMEPAD_AXIS_RIGHTY, +thumb_dead_zone, +32767);
}

void ImGui_ImplSDL3_NewFrame()
{
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplSDL3_Init()?");
    ImGuiIO& io = ImGui::GetIO();

    // Setup main viewport size (every frame to accommodate for window resizing)
    // TMS: Use window_width and window_height instead
    io.DisplaySize = ImVec2((float) _tms.window_width, (float) _tms.window_height);
    // TMS: What is this?
    io.DisplayFramebufferScale = ImVec2(1., 1.);

    // Setup time step (we could also use SDL_GetTicksNS() available since SDL3)
    // (Accept SDL_GetPerformanceCounter() not returning a monotonically increasing value. Happens in VMs and Emscripten, see #6189, #6114, #3644)
    static Uint64 frequency = SDL_GetPerformanceFrequency();
    Uint64 current_time = SDL_GetPerformanceCounter();
    if (current_time <= bd->Time)
        current_time = bd->Time + 1;
    io.DeltaTime = bd->Time > 0 ? (float)((double)(current_time - bd->Time) / (double)frequency) : (float)(1.0f / 60.0f);
    bd->Time = current_time;

    if (bd->MousePendingLeaveFrame && bd->MousePendingLeaveFrame >= ImGui::GetFrameCount() && bd->MouseButtonsDown == 0)
    {
        bd->MouseWindowID = 0;
        bd->MousePendingLeaveFrame = 0;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }

    ImGui_ImplSDL3_UpdateMouseData();
    ImGui_ImplSDL3_UpdateMouseCursor();
    ImGui_ImplSDL3_UpdateIme();

    // Update game controllers (if enabled and available)
    ImGui_ImplSDL3_UpdateGamepads();
}

//-----------------------------------------------------------------------------

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
