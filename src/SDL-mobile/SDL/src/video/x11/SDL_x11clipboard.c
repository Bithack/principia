/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "SDL_config.h"

#if SDL_VIDEO_DRIVER_X11

#include <limits.h> /* For INT_MAX */

#include "SDL_events.h"
#include "SDL_x11video.h"


/* If you don't support UTF-8, you might use XA_STRING here */
#ifdef X_HAVE_UTF8_STRING
#define TEXT_FORMAT XInternAtom(display, "UTF8_STRING", False)
#else
#define TEXT_FORMAT XA_STRING
#endif

/* Get any application owned window handle for clipboard association */
static Window
GetWindow(_THIS)
{
    SDL_Window *window;

    window = _this->windows;
    if (window) {
        return ((SDL_WindowData *) window->driverdata)->xwindow;
    }
    return None;
}

int
X11_SetClipboardText(_THIS, const char *text)
{
    Display *display = ((SDL_VideoData *) _this->driverdata)->display;
    Atom format;
    Window window;
    Atom XA_CLIPBOARD = XInternAtom(display, "CLIPBOARD", 0);

    /* Get the SDL window that will own the selection */
    window = GetWindow(_this);
    if (window == None) {
        SDL_SetError("Couldn't find a window to own the selection");
        return -1;
    }

    /* Save the selection on the root window */
    format = TEXT_FORMAT;
    XChangeProperty(display, DefaultRootWindow(display),
        XA_CUT_BUFFER0, format, 8, PropModeReplace,
        (const unsigned char *)text, SDL_strlen(text));

    if (XA_CLIPBOARD != None &&
        XGetSelectionOwner(display, XA_CLIPBOARD) != window) {
        XSetSelectionOwner(display, XA_CLIPBOARD, window, CurrentTime);
    }

    if (XGetSelectionOwner(display, XA_PRIMARY) != window) {
        XSetSelectionOwner(display, XA_PRIMARY, window, CurrentTime);
    }
    return 0;
}

char *
X11_GetClipboardText(_THIS)
{
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
    Display *display = videodata->display;
    Atom format;
    Window window;
    Window owner;
    Atom selection;
    Atom seln_type;
    int seln_format;
    unsigned long nbytes;
    unsigned long overflow;
    unsigned char *src;
    char *text;
    Atom XA_CLIPBOARD = XInternAtom(display, "CLIPBOARD", 0);
    if (XA_CLIPBOARD == None) {
        SDL_SetError("Couldn't access X clipboard");
        return NULL;
    }

    text = NULL;

    /* Get the window that holds the selection */
    window = GetWindow(_this);
    format = TEXT_FORMAT;
    owner = XGetSelectionOwner(display, XA_CLIPBOARD);
    if ((owner == None) || (owner == window)) {
        owner = DefaultRootWindow(display);
        selection = XA_CUT_BUFFER0;
    } else {
        /* Request that the selection owner copy the data to our window */
        owner = window;
        selection = XInternAtom(display, "SDL_SELECTION", False);
        XConvertSelection(display, XA_CLIPBOARD, format, selection, owner,
            CurrentTime);

        /* FIXME: Should we have a timeout here? */
        videodata->selection_waiting = SDL_TRUE;
        while (videodata->selection_waiting) {
            SDL_PumpEvents();
        }
    }

    if (XGetWindowProperty(display, owner, selection, 0, INT_MAX/4, False,
            format, &seln_type, &seln_format, &nbytes, &overflow, &src)
            == Success) {
        if (seln_type == format) {
            text = (char *)SDL_malloc(nbytes+1);
            if (text) {
                SDL_memcpy(text, src, nbytes);
                text[nbytes] = '\0';
            }
        }
        XFree(src);
    }

    if (!text) {
        text = SDL_strdup("");
    }
    
    return text;
}

SDL_bool
X11_HasClipboardText(_THIS)
{
    SDL_bool result = SDL_FALSE;
    char *text = X11_GetClipboardText(_this);
    if (text) {
        result = (SDL_strlen(text)>0) ? SDL_TRUE : SDL_FALSE;
        SDL_free(text);
    }    
    return result;
}

#endif /* SDL_VIDEO_DRIVER_X11 */

/* vi: set ts=4 sw=4 expandtab: */
