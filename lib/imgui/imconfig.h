// This file has been customised for Principia. Reset the changes in this file when updating Imgui.
#pragma once

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#ifndef DEBUG
    #define IMGUI_DISABLE_DEMO_WINDOWS
    #define IMGUI_DISABLE_DEBUG_TOOLS
#endif

// This stuff is provided by SDL
#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS   // [Win32] Don't implement default clipboard handler. Won't use and link with OpenClipboard/GetClipboardData/CloseClipboard etc. (user32.lib/.a, kernel32.lib/.a)
#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS         // [Win32] [Default with non-Visual Studio compilers] Don't implement default IME handler (won't require imm32.lib/.a)
#define IMGUI_DISABLE_WIN32_FUNCTIONS                     // [Win32] Won't use and link with any Win32 function (clipboard, IME).
#define IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS             // Don't implement default platform_io.Platform_OpenInShellFn() handler (Win32: ShellExecute(), require shell32.lib/.a, Mac/Linux: use system("")).
#define IMGUI_DISABLE_TIME_FUNCTIONS                      // Don't setup default platform_io.Platform_SessionDate value using time(), localtime_r().

// We use freetype with a custom font
#define IMGUI_DISABLE_DEFAULT_FONT

// Use imgui_freetype for font rendering
#define IMGUI_ENABLE_FREETYPE
