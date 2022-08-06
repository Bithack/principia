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

#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h> /* For INT_MAX */

#include "SDL_x11video.h"
#include "SDL_x11touch.h"
#include "SDL_x11xinput2.h"
#include "../../events/SDL_events_c.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/SDL_touch_c.h"

#include "SDL_timer.h"
#include "SDL_syswm.h"

#include <stdio.h>

#ifdef SDL_INPUT_LINUXEV
//Touch Input/event* includes
#include <linux/input.h>
#include <fcntl.h>
#endif

/* Check to see if this is a repeated key.
   (idea shamelessly lifted from GII -- thanks guys! :)
 */
static SDL_bool X11_KeyRepeat(Display *display, XEvent *event)
{
    XEvent peekevent;

    if (XPending(display)) {
        XPeekEvent(display, &peekevent);
        if ((peekevent.type == KeyPress) &&
            (peekevent.xkey.keycode == event->xkey.keycode) &&
            ((peekevent.xkey.time-event->xkey.time) < 2)) {
            return SDL_TRUE;
        }
    }
    return SDL_FALSE;
}

static SDL_bool X11_IsWheelEvent(Display * display,XEvent * event,int * ticks)
{
    XEvent peekevent;
    if (XPending(display)) {
        /* according to the xlib docs, no specific mouse wheel events exist.
           however, mouse wheel events trigger a button press and a button release
           immediately. thus, checking if the same button was released at the same
           time as it was pressed, should be an adequate hack to derive a mouse 
           wheel event. */
        XPeekEvent(display,&peekevent);
        if ((peekevent.type           == ButtonRelease) &&
            (peekevent.xbutton.button == event->xbutton.button) &&
            (peekevent.xbutton.time   == event->xbutton.time)) {

            /* by default, X11 only knows 5 buttons. on most 3 button + wheel mouse,
               Button4 maps to wheel up, Button5 maps to wheel down. */
            if (event->xbutton.button == Button4) {
                *ticks = 1;
            }
            else if (event->xbutton.button == Button5) {
                *ticks = -1;
            }

            /* remove the following release event, as this is now a wheel event */
            XNextEvent(display,&peekevent);
            return SDL_TRUE;
        }
    }
    return SDL_FALSE;
}


#if SDL_VIDEO_DRIVER_X11_SUPPORTS_GENERIC_EVENTS
static void X11_HandleGenericEvent(SDL_VideoData *videodata,XEvent event)
{
    XGenericEventCookie *cookie = &event.xcookie;
    XGetEventData(videodata->display, cookie);
    X11_HandleXinput2Event(videodata,cookie);
    XFreeEventData(videodata->display,cookie);
}
#endif /* SDL_VIDEO_DRIVER_X11_SUPPORTS_GENERIC_EVENTS */



static void
X11_DispatchEvent(_THIS)
{
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
    Display *display = videodata->display;
    SDL_WindowData *data;
    XEvent xevent, discard;
    int i;

    SDL_zero(xevent);           /* valgrind fix. --ryan. */
    XNextEvent(display, &xevent);

    /* filter events catchs XIM events and sends them to the correct
       handler */
    if (XFilterEvent(&xevent, None) == True) {
#if 0
        printf("Filtered event type = %d display = %d window = %d\n",
               xevent.type, xevent.xany.display, xevent.xany.window);
#endif
        return;
    }

    /* Send a SDL_SYSWMEVENT if the application wants them */
    if (SDL_GetEventState(SDL_SYSWMEVENT) == SDL_ENABLE) {
        SDL_SysWMmsg wmmsg;

        SDL_VERSION(&wmmsg.version);
        wmmsg.subsystem = SDL_SYSWM_X11;
        wmmsg.msg.x11.event = xevent;
        SDL_SendSysWMEvent(&wmmsg);
    }

#if SDL_VIDEO_DRIVER_X11_SUPPORTS_GENERIC_EVENTS
    if(xevent.type == GenericEvent) {
        X11_HandleGenericEvent(videodata,xevent);
        return;
    }
#endif

    data = NULL;
    if (videodata && videodata->windowlist) {
        for (i = 0; i < videodata->numwindows; ++i) {
            if ((videodata->windowlist[i] != NULL) &&
                (videodata->windowlist[i]->xwindow == xevent.xany.window)) {
                data = videodata->windowlist[i];
                break;
            }
        }
    }
    if (!data) {
        return;
    }

#if 0
    printf("type = %d display = %d window = %d\n",
           xevent.type, xevent.xany.display, xevent.xany.window);
#endif
    switch (xevent.type) {

        /* Gaining mouse coverage? */
    case EnterNotify:{
#ifdef DEBUG_XEVENTS
            printf("EnterNotify! (%d,%d,%d)\n", 
                   xevent.xcrossing.x,
                   xevent.xcrossing.y,
                   xevent.xcrossing.mode);
            if (xevent.xcrossing.mode == NotifyGrab)
                printf("Mode: NotifyGrab\n");
            if (xevent.xcrossing.mode == NotifyUngrab)
                printf("Mode: NotifyUngrab\n");
#endif
            SDL_SetMouseFocus(data->window);
        }
        break;
        /* Losing mouse coverage? */
    case LeaveNotify:{
#ifdef DEBUG_XEVENTS
            printf("LeaveNotify! (%d,%d,%d)\n", 
                   xevent.xcrossing.x,
                   xevent.xcrossing.y,
                   xevent.xcrossing.mode);
            if (xevent.xcrossing.mode == NotifyGrab)
                printf("Mode: NotifyGrab\n");
            if (xevent.xcrossing.mode == NotifyUngrab)
                printf("Mode: NotifyUngrab\n");
#endif
            if (xevent.xcrossing.mode != NotifyGrab &&
                xevent.xcrossing.mode != NotifyUngrab &&
                xevent.xcrossing.detail != NotifyInferior) {
                SDL_SetMouseFocus(NULL);
            }
        }
        break;

        /* Gaining input focus? */
    case FocusIn:{
#ifdef DEBUG_XEVENTS
            printf("FocusIn!\n");
#endif
            SDL_SetKeyboardFocus(data->window);
#ifdef X_HAVE_UTF8_STRING
            if (data->ic) {
                XSetICFocus(data->ic);
            }
#endif
        }
        break;

        /* Losing input focus? */
    case FocusOut:{
#ifdef DEBUG_XEVENTS
            printf("FocusOut!\n");
#endif
            SDL_SetKeyboardFocus(NULL);
#ifdef X_HAVE_UTF8_STRING
            if (data->ic) {
                XUnsetICFocus(data->ic);
            }
#endif
        }
        break;

        /* Generated upon EnterWindow and FocusIn */
    case KeymapNotify:{
#ifdef DEBUG_XEVENTS
            printf("KeymapNotify!\n");
#endif
            /* FIXME:
               X11_SetKeyboardState(SDL_Display, xevent.xkeymap.key_vector);
             */
        }
        break;

        /* Has the keyboard layout changed? */
    case MappingNotify:{
#ifdef DEBUG_XEVENTS
            printf("MappingNotify!\n");
#endif
            X11_UpdateKeymap(_this);
        }
        break;

        /* Key press? */
    case KeyPress:{
            KeyCode keycode = xevent.xkey.keycode;
            KeySym keysym = NoSymbol;
            char text[SDL_TEXTINPUTEVENT_TEXT_SIZE];
            Status status = 0;

#ifdef DEBUG_XEVENTS
            printf("KeyPress (X11 keycode = 0x%X)\n", xevent.xkey.keycode);
#endif
            SDL_SendKeyboardKey(SDL_PRESSED, videodata->key_layout[keycode]);
#if 1
            if (videodata->key_layout[keycode] == SDL_SCANCODE_UNKNOWN) {
                int min_keycode, max_keycode;
                XDisplayKeycodes(display, &min_keycode, &max_keycode);
#if SDL_VIDEO_DRIVER_X11_HAS_XKBKEYCODETOKEYSYM
                keysym = XkbKeycodeToKeysym(display, keycode, 0, 0);
#else
                keysym = XKeycodeToKeysym(display, keycode, 0);
#endif
                fprintf(stderr,
                        "The key you just pressed is not recognized by SDL. To help get this fixed, please report this to the SDL mailing list <sdl@libsdl.org> X11 KeyCode %d (%d), X11 KeySym 0x%lX (%s).\n",
                        keycode, keycode - min_keycode, keysym,
                        XKeysymToString(keysym));
            }
#endif
            /* */
            SDL_zero(text);
#ifdef X_HAVE_UTF8_STRING
            if (data->ic) {
                Xutf8LookupString(data->ic, &xevent.xkey, text, sizeof(text),
                                  &keysym, &status);
            }
#else
            XLookupString(&xevent.xkey, text, sizeof(text), &keysym, NULL);
#endif
            if (*text) {
                SDL_SendKeyboardText(text);
            }
        }
        break;

        /* Key release? */
    case KeyRelease:{
            KeyCode keycode = xevent.xkey.keycode;

#ifdef DEBUG_XEVENTS
            printf("KeyRelease (X11 keycode = 0x%X)\n", xevent.xkey.keycode);
#endif
            if (X11_KeyRepeat(display, &xevent)) {
                /* We're about to get a repeated key down, ignore the key up */

                /* XXX TMS FIX XXX discard repeat */
                XNextEvent(display, &discard);
                break;
            }
            SDL_SendKeyboardKey(SDL_RELEASED, videodata->key_layout[keycode]);
        }
        break;

        /* Have we been iconified? */
    case UnmapNotify:{
#ifdef DEBUG_XEVENTS
            printf("UnmapNotify!\n");
#endif
            SDL_SendWindowEvent(data->window, SDL_WINDOWEVENT_HIDDEN, 0, 0);
            SDL_SendWindowEvent(data->window, SDL_WINDOWEVENT_MINIMIZED, 0, 0);
        }
        break;

        /* Have we been restored? */
    case MapNotify:{
#ifdef DEBUG_XEVENTS
            printf("MapNotify!\n");
#endif
            SDL_SendWindowEvent(data->window, SDL_WINDOWEVENT_SHOWN, 0, 0);
            SDL_SendWindowEvent(data->window, SDL_WINDOWEVENT_RESTORED, 0, 0);
        }
        break;

        /* Have we been resized or moved? */
    case ConfigureNotify:{
#ifdef DEBUG_XEVENTS
            printf("ConfigureNotify! (resize: %dx%d)\n",
                   xevent.xconfigure.width, xevent.xconfigure.height);
#endif
            SDL_SendWindowEvent(data->window, SDL_WINDOWEVENT_MOVED,
                                xevent.xconfigure.x, xevent.xconfigure.y);
            SDL_SendWindowEvent(data->window, SDL_WINDOWEVENT_RESIZED,
                                xevent.xconfigure.width,
                                xevent.xconfigure.height);
        }
        break;

        /* Have we been requested to quit (or another client message?) */
    case ClientMessage:{
            if ((xevent.xclient.format == 32) &&
                (xevent.xclient.data.l[0] == videodata->WM_DELETE_WINDOW)) {

                SDL_SendWindowEvent(data->window, SDL_WINDOWEVENT_CLOSE, 0, 0);
            }
        }
        break;

        /* Do we need to refresh ourselves? */
    case Expose:{
#ifdef DEBUG_XEVENTS
            printf("Expose (count = %d)\n", xevent.xexpose.count);
#endif
            SDL_SendWindowEvent(data->window, SDL_WINDOWEVENT_EXPOSED, 0, 0);
        }
        break;

    case MotionNotify:{
            SDL_Mouse *mouse = SDL_GetMouse();  
            if(!mouse->relative_mode) {
#ifdef DEBUG_MOTION
                printf("X11 motion: %d,%d\n", xevent.xmotion.x, xevent.xmotion.y);
#endif

                SDL_SendMouseMotion(data->window, 0, xevent.xmotion.x, xevent.xmotion.y);
            }
        }
        break;

    case ButtonPress:{
            int ticks = 0;
            if (X11_IsWheelEvent(display,&xevent,&ticks) == SDL_TRUE) {
                SDL_SendMouseWheel(data->window, 0, ticks);
            }
            else {
                SDL_SendMouseButton(data->window, SDL_PRESSED, xevent.xbutton.button);
            }
        }
        break;

    case ButtonRelease:{
            SDL_SendMouseButton(data->window, SDL_RELEASED, xevent.xbutton.button);
        }
        break;

    case PropertyNotify:{
#ifdef DEBUG_XEVENTS
            unsigned char *propdata;
            int status, real_format;
            Atom real_type;
            unsigned long items_read, items_left, i;

            char *name = XGetAtomName(display, xevent.xproperty.atom);
            if (name) {
                printf("PropertyNotify: %s %s\n", name, (xevent.xproperty.state == PropertyDelete) ? "deleted" : "changed");
                XFree(name);
            }

            status = XGetWindowProperty(display, data->xwindow, xevent.xproperty.atom, 0L, 8192L, False, AnyPropertyType, &real_type, &real_format, &items_read, &items_left, &propdata);
            if (status == Success && items_read > 0) {
                if (real_type == XA_INTEGER) {
                    int *values = (int *)propdata;

                    printf("{");
                    for (i = 0; i < items_read; i++) {
                        printf(" %d", values[i]);
                    }
                    printf(" }\n");
                } else if (real_type == XA_CARDINAL) {
                    if (real_format == 32) {
                        Uint32 *values = (Uint32 *)propdata;

                        printf("{");
                        for (i = 0; i < items_read; i++) {
                            printf(" %d", values[i]);
                        }
                        printf(" }\n");
                    } else if (real_format == 16) {
                        Uint16 *values = (Uint16 *)propdata;

                        printf("{");
                        for (i = 0; i < items_read; i++) {
                            printf(" %d", values[i]);
                        }
                        printf(" }\n");
                    } else if (real_format == 8) {
                        Uint8 *values = (Uint8 *)propdata;

                        printf("{");
                        for (i = 0; i < items_read; i++) {
                            printf(" %d", values[i]);
                        }
                        printf(" }\n");
                    }
                } else if (real_type == XA_STRING ||
                           real_type == videodata->UTF8_STRING) {
                    printf("{ \"%s\" }\n", propdata);
                } else if (real_type == XA_ATOM) {
                    Atom *atoms = (Atom *)propdata;

                    printf("{");
                    for (i = 0; i < items_read; i++) {
                        char *name = XGetAtomName(display, atoms[i]);
                        if (name) {
                            printf(" %s", name);
                            XFree(name);
                        }
                    }
                    printf(" }\n");
                } else {
                    char *name = XGetAtomName(display, real_type);
                    printf("Unknown type: %ld (%s)\n", real_type, name ? name : "UNKNOWN");
                    if (name) {
                        XFree(name);
                    }
                }
            }
#endif
        }
        break;

    /* Copy the selection from XA_CUT_BUFFER0 to the requested property */
    case SelectionRequest: {
            XSelectionRequestEvent *req;
            XEvent sevent;
            int seln_format;
            unsigned long nbytes;
            unsigned long overflow;
            unsigned char *seln_data;

            req = &xevent.xselectionrequest;
#ifdef DEBUG_XEVENTS
            printf("SelectionRequest (requestor = %ld, target = %ld)\n",
                req->requestor, req->target);
#endif

            SDL_zero(sevent);
            sevent.xany.type = SelectionNotify;
            sevent.xselection.selection = req->selection;
            sevent.xselection.target = None;
            sevent.xselection.property = None;
            sevent.xselection.requestor = req->requestor;
            sevent.xselection.time = req->time;
            if (XGetWindowProperty(display, DefaultRootWindow(display),
                    XA_CUT_BUFFER0, 0, INT_MAX/4, False, req->target,
                    &sevent.xselection.target, &seln_format, &nbytes,
                    &overflow, &seln_data) == Success) {
                if (sevent.xselection.target == req->target) {
                    XChangeProperty(display, req->requestor, req->property,
                        sevent.xselection.target, seln_format, PropModeReplace,
                        seln_data, nbytes);
                    sevent.xselection.property = req->property;
                }
                XFree(seln_data);
            }
            XSendEvent(display, req->requestor, False, 0, &sevent);
            XSync(display, False);
        }
        break;

    case SelectionNotify: {
#ifdef DEBUG_XEVENTS
            printf("SelectionNotify (requestor = %ld, target = %ld)\n",
                xevent.xselection.requestor, xevent.xselection.target);
#endif
            videodata->selection_waiting = SDL_FALSE;
        }
        break;

    default:{
#ifdef DEBUG_XEVENTS
            printf("Unhandled event %d\n", xevent.type);
#endif
        }
        break;
    }
}

/* Ack!  XPending() actually performs a blocking read if no events available */
static int
X11_Pending(Display * display)
{
    /* Flush the display connection and look to see if events are queued */
    XFlush(display);
    if (XEventsQueued(display, QueuedAlready)) {
        return (1);
    }

    /* More drastic measures are required -- see if X is ready to talk */
    {
        static struct timeval zero_time;        /* static == 0 */
        int x11_fd;
        fd_set fdset;

        x11_fd = ConnectionNumber(display);
        FD_ZERO(&fdset);
        FD_SET(x11_fd, &fdset);
        if (select(x11_fd + 1, &fdset, NULL, NULL, &zero_time) == 1) {
            return (XPending(display));
        }
    }

    /* Oh well, nothing is ready .. */
    return (0);
}


/* !!! FIXME: this should be exposed in a header, or something. */
int SDL_GetNumTouch(void);


void
X11_PumpEvents(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    /* Update activity every 30 seconds to prevent screensaver */
    if (_this->suspend_screensaver) {
        Uint32 now = SDL_GetTicks();
        if (!data->screensaver_activity ||
            (int) (now - data->screensaver_activity) >= 30000) {
            XResetScreenSaver(data->display);
            data->screensaver_activity = now;
        }
    }   

    /* Keep processing pending events */
    while (X11_Pending(data->display)) {
        X11_DispatchEvent(_this);
    }
    /*Dont process evtouch events if XInput2 multitouch is supported*/
    if(X11_Xinput2IsMultitouchSupported()) {
        return;
    }

#ifdef SDL_INPUT_LINUXEV
    /* Process Touch events*/
    int i = 0,rd;
    struct input_event ev[64];
    int size = sizeof (struct input_event);

/* !!! FIXME: clean the tabstops out of here. */
    for(i = 0;i < SDL_GetNumTouch();++i) {
	SDL_Touch* touch = SDL_GetTouchIndex(i);
	if(!touch) printf("Touch %i/%i DNE\n",i,SDL_GetNumTouch());
	EventTouchData* data;
	data = (EventTouchData*)(touch->driverdata);
	if(data == NULL) {
	  printf("No driver data\n");
	  continue;
	}
	if(data->eventStream <= 0) 
	    printf("Error: Couldn't open stream\n");
	rd = read(data->eventStream, ev, size * 64);
	if(rd >= size) {
	    for (i = 0; i < rd / sizeof(struct input_event); i++) {
		switch (ev[i].type) {
		case EV_ABS:
		    switch (ev[i].code) {
			case ABS_X:
			    data->x = ev[i].value;
			    break;
			case ABS_Y:
			    data->y = ev[i].value;
			    break;
			case ABS_PRESSURE:
			    data->pressure = ev[i].value;
			    if(data->pressure < 0) data->pressure = 0;
			    break;
			case ABS_MISC:
			    if(ev[i].value == 0)
			        data->up = SDL_TRUE;			    
			    break;
			}
		    break;
		case EV_MSC:
			if(ev[i].code == MSC_SERIAL)
				data->finger = ev[i].value;
			break;
		case EV_KEY:
			if(ev[i].code == BTN_TOUCH)
			    if(ev[i].value == 0)
			        data->up = SDL_TRUE;
			break;
		case EV_SYN:
		  if(!data->down) {
		      data->down = SDL_TRUE;
		      SDL_SendFingerDown(touch->id,data->finger,
		    		  data->down, data->x, data->y,
		    		  data->pressure);
		  }
		  else if(!data->up)
		    SDL_SendTouchMotion(touch->id,data->finger, 
					SDL_FALSE, data->x,data->y,
					data->pressure);
		  else
		  {
		      data->down = SDL_FALSE;
			  SDL_SendFingerDown(touch->id,data->finger,
					  data->down, data->x,data->y,
					  data->pressure);
			  data->x = -1;
			  data->y = -1;
			  data->pressure = -1;
			  data->finger = 0;
			  data->up = SDL_FALSE;
		  }
		  break;		
		}
	    }
	}
    }
#endif
}

/* This is so wrong it hurts */
#define GNOME_SCREENSAVER_HACK
#ifdef GNOME_SCREENSAVER_HACK
#include <unistd.h>
static pid_t screensaver_inhibit_pid;
static void
gnome_screensaver_disable()
{
    screensaver_inhibit_pid = fork();
    if (screensaver_inhibit_pid == 0) {
        close(0);
        close(1);
        close(2);
        execl("/usr/bin/gnome-screensaver-command",
              "gnome-screensaver-command",
              "--inhibit",
              "--reason",
              "GNOME screensaver doesn't respect MIT-SCREEN-SAVER", NULL);
        exit(2);
    }
}
static void
gnome_screensaver_enable()
{
    kill(screensaver_inhibit_pid, 15);
}
#endif

void
X11_SuspendScreenSaver(_THIS)
{
#if SDL_VIDEO_DRIVER_X11_XSCRNSAVER
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    int dummy;
    int major_version, minor_version;

    if (SDL_X11_HAVE_XSS) {
        /* XScreenSaverSuspend was introduced in MIT-SCREEN-SAVER 1.1 */
        if (!XScreenSaverQueryExtension(data->display, &dummy, &dummy) ||
            !XScreenSaverQueryVersion(data->display,
                                      &major_version, &minor_version) ||
            major_version < 1 || (major_version == 1 && minor_version < 1)) {
            return;
        }

        XScreenSaverSuspend(data->display, _this->suspend_screensaver);
        XResetScreenSaver(data->display);
    }
#endif

#ifdef GNOME_SCREENSAVER_HACK
    if (_this->suspend_screensaver) {
        gnome_screensaver_disable();
    } else {
        gnome_screensaver_enable();
    }
#endif
}

#endif /* SDL_VIDEO_DRIVER_X11 */

/* vi: set ts=4 sw=4 expandtab: */
