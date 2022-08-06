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

/* *INDENT-OFF* */

SDL_X11_MODULE(BASEXLIB)
SDL_X11_SYM(XSizeHints*,XAllocSizeHints,(void),(),return)
SDL_X11_SYM(int,XAutoRepeatOn,(Display* a),(a),return)
SDL_X11_SYM(int,XAutoRepeatOff,(Display* a),(a),return)
SDL_X11_SYM(int,XChangePointerControl,(Display* a,Bool b,Bool c,int d,int e,int f),(a,b,c,d,e,f),return)
SDL_X11_SYM(int,XChangeProperty,(Display* a,Window b,Atom c,Atom d,int e,int f,_Xconst unsigned char* g,int h),(a,b,c,d,e,f,g,h),return)
SDL_X11_SYM(Bool,XCheckIfEvent,(Display* a,XEvent *b,Bool (*c)(Display*,XEvent*,XPointer),XPointer d),(a,b,c,d),return)
SDL_X11_SYM(int,XCloseDisplay,(Display* a),(a),return)
SDL_X11_SYM(int,XConvertSelection,(Display* a,Atom b,Atom c,Atom d,Window e,Time f),(a,b,c,d,e,f),return)
SDL_X11_SYM(Pixmap,XCreateBitmapFromData,(Display *dpy,Drawable d,_Xconst char *data,unsigned int width,unsigned int height),(dpy,d,data,width,height),return)
SDL_X11_SYM(Colormap,XCreateColormap,(Display* a,Window b,Visual* c,int d),(a,b,c,d),return)
SDL_X11_SYM(Cursor,XCreatePixmapCursor,(Display* a,Pixmap b,Pixmap c,XColor* d,XColor* e,unsigned int f,unsigned int g),(a,b,c,d,e,f,g),return)
SDL_X11_SYM(GC,XCreateGC,(Display* a,Drawable b,unsigned long c,XGCValues* d),(a,b,c,d),return)
SDL_X11_SYM(XImage*,XCreateImage,(Display* a,Visual* b,unsigned int c,int d,int e,char* f,unsigned int g,unsigned int h,int i,int j),(a,b,c,d,e,f,g,h,i,j),return)
SDL_X11_SYM(Window,XCreateWindow,(Display* a,Window b,int c,int d,unsigned int e,unsigned int f,unsigned int g,int h,unsigned int i,Visual* j,unsigned long k,XSetWindowAttributes* l),(a,b,c,d,e,f,g,h,i,j,k,l),return)
SDL_X11_SYM(int,XDefineCursor,(Display* a,Window b,Cursor c),(a,b,c),return)
SDL_X11_SYM(int,XDeleteProperty,(Display* a,Window b,Atom c),(a,b,c),return)
SDL_X11_SYM(int,XDestroyWindow,(Display* a,Window b),(a,b),return)
SDL_X11_SYM(int,XDisplayKeycodes,(Display* a,int* b,int* c),(a,b,c),return)
SDL_X11_SYM(char*,XDisplayName,(_Xconst char* a),(a),return)
SDL_X11_SYM(int,XEventsQueued,(Display* a,int b),(a,b),return)
SDL_X11_SYM(Bool,XFilterEvent,(XEvent *event,Window w),(event,w),return)
SDL_X11_SYM(int,XFlush,(Display* a),(a),return)
SDL_X11_SYM(int,XFree,(void*a),(a),return)
SDL_X11_SYM(int,XFreeCursor,(Display* a,Cursor b),(a,b),return)
SDL_X11_SYM(int,XFreeGC,(Display* a,GC b),(a,b),return)
SDL_X11_SYM(int,XFreeModifiermap,(XModifierKeymap* a),(a),return)
SDL_X11_SYM(int,XFreePixmap,(Display* a,Pixmap b),(a,b),return)
SDL_X11_SYM(char*,XGetAtomName,(Display *a,Atom b),(a,b),return)
SDL_X11_SYM(int,XGetInputFocus,(Display *a,Window *b,int *c),(a,b,c),return)
SDL_X11_SYM(int,XGetErrorDatabaseText,(Display* a,_Xconst char* b,_Xconst char* c,_Xconst char* d,char* e,int f),(a,b,c,d,e,f),return)
SDL_X11_SYM(XModifierKeymap*,XGetModifierMapping,(Display* a),(a),return)
SDL_X11_SYM(int,XGetPointerControl,(Display* a,int* b,int* c,int* d),(a,b,c,d),return)
SDL_X11_SYM(Window,XGetSelectionOwner,(Display* a,Atom b),(a,b),return)
SDL_X11_SYM(XVisualInfo*,XGetVisualInfo,(Display* a,long b,XVisualInfo* c,int* d),(a,b,c,d),return)
SDL_X11_SYM(Status,XGetWindowAttributes,(Display* a,Window b,XWindowAttributes* c),(a,b,c),return)
SDL_X11_SYM(int,XGetWindowProperty,(Display* a,Window b,Atom c,long d,long e,Bool f,Atom g,Atom* h,int* i,unsigned long* j,unsigned long *k,unsigned char **l),(a,b,c,d,e,f,g,h,i,j,k,l),return)
SDL_X11_SYM(XWMHints*,XGetWMHints,(Display* a,Window b),(a,b),return)
SDL_X11_SYM(Status,XGetWMNormalHints,(Display *a,Window b, XSizeHints *c, long *d),(a,b,c,d),return)
SDL_X11_SYM(int,XIfEvent,(Display* a,XEvent *b,Bool (*c)(Display*,XEvent*,XPointer),XPointer d),(a,b,c,d),return)
SDL_X11_SYM(int,XGrabKeyboard,(Display* a,Window b,Bool c,int d,int e,Time f),(a,b,c,d,e,f),return)
SDL_X11_SYM(int,XGrabPointer,(Display* a,Window b,Bool c,unsigned int d,int e,int f,Window g,Cursor h,Time i),(a,b,c,d,e,f,g,h,i),return)
SDL_X11_SYM(int,XGrabServer,(Display* a),(a),return)
SDL_X11_SYM(Status,XIconifyWindow,(Display* a,Window b,int c),(a,b,c),return)
SDL_X11_SYM(KeyCode,XKeysymToKeycode,(Display* a,KeySym b),(a,b),return)
SDL_X11_SYM(char*,XKeysymToString,(KeySym a),(a),return)
SDL_X11_SYM(Atom,XInternAtom,(Display* a,_Xconst char* b,Bool c),(a,b,c),return)
SDL_X11_SYM(XPixmapFormatValues*,XListPixmapFormats,(Display* a,int* b),(a,b),return)
SDL_X11_SYM(KeySym,XLookupKeysym,(XKeyEvent* a,int b),(a,b),return)
SDL_X11_SYM(int,XLookupString,(XKeyEvent* a,char* b,int c,KeySym* d,XComposeStatus* e),(a,b,c,d,e),return)
SDL_X11_SYM(int,XMapRaised,(Display* a,Window b),(a,b),return)
SDL_X11_SYM(Status,XMatchVisualInfo,(Display* a,int b,int c,int d,XVisualInfo* e),(a,b,c,d,e),return)
SDL_X11_SYM(int,XMissingExtension,(Display* a,_Xconst char* b),(a,b),return)
SDL_X11_SYM(int,XMoveWindow,(Display* a,Window b,int c,int d),(a,b,c,d),return)
SDL_X11_SYM(int,XNextEvent,(Display* a,XEvent* b),(a,b),return)
SDL_X11_SYM(Display*,XOpenDisplay,(_Xconst char* a),(a),return)
SDL_X11_SYM(int,XPeekEvent,(Display* a,XEvent* b),(a,b),return)
SDL_X11_SYM(int,XPending,(Display* a),(a),return)
SDL_X11_SYM(int,XPutImage,(Display* a,Drawable b,GC c,XImage* d,int e,int f,int g,int h,unsigned int i,unsigned int j),(a,b,c,d,e,f,g,h,i,j),return)
SDL_X11_SYM(int,XQueryKeymap,(Display* a,char *b),(a,b),return)
SDL_X11_SYM(Bool,XQueryPointer,(Display* a,Window b,Window* c,Window* d,int* e,int* f,int* g,int* h,unsigned int* i),(a,b,c,d,e,f,g,h,i),return)
SDL_X11_SYM(int,XRaiseWindow,(Display* a,Window b),(a,b),return)
SDL_X11_SYM(int,XResetScreenSaver,(Display* a),(a),return)
SDL_X11_SYM(int,XResizeWindow,(Display* a,Window b,unsigned int c,unsigned int d),(a,b,c,d),return)
SDL_X11_SYM(int,XSelectInput,(Display* a,Window b,long c),(a,b,c),return)
SDL_X11_SYM(Status,XSendEvent,(Display* a,Window b,Bool c,long d,XEvent* e),(a,b,c,d,e),return)
SDL_X11_SYM(XErrorHandler,XSetErrorHandler,(XErrorHandler a),(a),return)
SDL_X11_SYM(XIOErrorHandler,XSetIOErrorHandler,(XIOErrorHandler a),(a),return)
SDL_X11_SYM(int,XSetInputFocus,(Display *a,Window b,int c,Time d),(a,b,c,d),return)
SDL_X11_SYM(int,XSetSelectionOwner,(Display* a,Atom b,Window c,Time d),(a,b,c,d),return)
SDL_X11_SYM(int,XSetTransientForHint,(Display* a,Window b,Window c),(a,b,c),return)
SDL_X11_SYM(void,XSetTextProperty,(Display* a,Window b,XTextProperty* c,Atom d),(a,b,c,d),)
SDL_X11_SYM(void,XSetWMProperties,(Display* a,Window b,XTextProperty* c,XTextProperty* d,char** e,int f,XSizeHints* g,XWMHints* h,XClassHint* i),(a,b,c,d,e,f,g,h,i),)
SDL_X11_SYM(void,XSetWMNormalHints,(Display* a,Window b,XSizeHints* c),(a,b,c),)
SDL_X11_SYM(Status,XSetWMProtocols,(Display* a,Window b,Atom* c,int d),(a,b,c,d),return)
SDL_X11_SYM(int,XStoreColors,(Display* a,Colormap b,XColor* c,int d),(a,b,c,d),return)
SDL_X11_SYM(Status,XStringListToTextProperty,(char** a,int b,XTextProperty* c),(a,b,c),return)
SDL_X11_SYM(int,XSync,(Display* a,Bool b),(a,b),return)
SDL_X11_SYM(int,XUndefineCursor,(Display* a,Window b),(a,b),return)
SDL_X11_SYM(int,XUngrabKeyboard,(Display* a,Time b),(a,b),return)
SDL_X11_SYM(int,XUngrabPointer,(Display* a,Time b),(a,b),return)
SDL_X11_SYM(int,XUngrabServer,(Display* a),(a),return)
SDL_X11_SYM(int,XUnmapWindow,(Display* a,Window b),(a,b),return)
SDL_X11_SYM(int,XWarpPointer,(Display* a,Window b,Window c,int d,int e,unsigned int f,unsigned int g,int h,int i),(a,b,c,d,e,f,g,h,i),return)
SDL_X11_SYM(VisualID,XVisualIDFromVisual,(Visual* a),(a),return)
#if SDL_VIDEO_DRIVER_X11_CONST_PARAM_XEXTADDDISPLAY
SDL_X11_SYM(XExtDisplayInfo*,XextAddDisplay,(XExtensionInfo* a,Display* b,_Xconst char* c,XExtensionHooks* d,int e,XPointer f),(a,b,c,d,e,f),return)
#else
SDL_X11_SYM(XExtDisplayInfo*,XextAddDisplay,(XExtensionInfo* a,Display* b,_Xconst char* c,XExtensionHooks* d,int e,XPointer f),(a,b,c,d,e,f),return)
#endif
SDL_X11_SYM(XExtensionInfo*,XextCreateExtension,(void),(),return)
SDL_X11_SYM(void,XextDestroyExtension,(XExtensionInfo* a),(a),)
SDL_X11_SYM(XExtDisplayInfo*,XextFindDisplay,(XExtensionInfo* a,Display* b),(a,b),return)
SDL_X11_SYM(int,XextRemoveDisplay,(XExtensionInfo* a,Display* b),(a,b),return)
SDL_X11_SYM(Bool,XQueryExtension,(Display* a,_Xconst char* b,int* c,int* d,int* e),(a,b,c,d,e),return)
SDL_X11_SYM(char *,XDisplayString,(Display* a),(a),return)
SDL_X11_SYM(int,XGetErrorText,(Display* a,int b,char* c,int d),(a,b,c,d),return)
SDL_X11_SYM(void,_XEatData,(Display* a,unsigned long b),(a,b),)
SDL_X11_SYM(void,_XFlush,(Display* a),(a),)
SDL_X11_SYM(void,_XFlushGCCache,(Display* a,GC b),(a,b),)
SDL_X11_SYM(int,_XRead,(Display* a,char* b,long c),(a,b,c),return)
SDL_X11_SYM(void,_XReadPad,(Display* a,char* b,long c),(a,b,c),)
SDL_X11_SYM(void,_XSend,(Display* a,_Xconst char* b,long c),(a,b,c),)
SDL_X11_SYM(Status,_XReply,(Display* a,xReply* b,int c,Bool d),(a,b,c,d),return)
SDL_X11_SYM(unsigned long,_XSetLastRequestRead,(Display* a,xGenericReply* b),(a,b),return)
SDL_X11_SYM(SDL_X11_XSynchronizeRetType,XSynchronize,(Display* a,Bool b),(a,b),return)
SDL_X11_SYM(SDL_X11_XESetWireToEventRetType,XESetWireToEvent,(Display* a,int b,SDL_X11_XESetWireToEventRetType c),(a,b,c),return)
SDL_X11_SYM(SDL_X11_XESetEventToWireRetType,XESetEventToWire,(Display* a,int b,SDL_X11_XESetEventToWireRetType c),(a,b,c),return)

#if SDL_VIDEO_DRIVER_X11_SUPPORTS_GENERIC_EVENTS
SDL_X11_SYM(Bool,XGetEventData,(Display* a,XGenericEventCookie* b),(a,b),return)
SDL_X11_SYM(void,XFreeEventData,(Display* a,XGenericEventCookie* b),(a,b),)    
#endif

#if SDL_VIDEO_DRIVER_X11_HAS_XKBKEYCODETOKEYSYM
#if NeedWidePrototypes
SDL_X11_SYM(KeySym,XkbKeycodeToKeysym,(Display* a,unsigned int b,int c,int d),(a,b,c,d),return)
#else
SDL_X11_SYM(KeySym,XkbKeycodeToKeysym,(Display* a,KeyCode b,int c,int d),(a,b,c,d),return)
#endif
#endif

#if NeedWidePrototypes
SDL_X11_SYM(KeySym,XKeycodeToKeysym,(Display* a,unsigned int b,int c),(a,b,c),return)
#else
SDL_X11_SYM(KeySym,XKeycodeToKeysym,(Display* a,KeyCode b,int c),(a,b,c),return)
#endif

#ifdef X_HAVE_UTF8_STRING
SDL_X11_MODULE(UTF8)
SDL_X11_SYM(int,Xutf8TextListToTextProperty,(Display* a,char** b,int c,XICCEncodingStyle d,XTextProperty* e),(a,b,c,d,e),return)
SDL_X11_SYM(int,Xutf8LookupString,(XIC a,XKeyPressedEvent* b,char* c,int d,KeySym* e,Status* f),(a,b,c,d,e,f),return)
/*SDL_X11_SYM(XIC,XCreateIC,(XIM, ...),return)  !!! ARGH! */
SDL_X11_SYM(void,XDestroyIC,(XIC a),(a),)
/*SDL_X11_SYM(char*,XGetICValues,(XIC, ...),return)  !!! ARGH! */
SDL_X11_SYM(void,XSetICFocus,(XIC a),(a),)
SDL_X11_SYM(void,XUnsetICFocus,(XIC a),(a),)
SDL_X11_SYM(XIM,XOpenIM,(Display* a,struct _XrmHashBucketRec* b,char* c,char* d),(a,b,c,d),return)
SDL_X11_SYM(Status,XCloseIM,(XIM a),(a),return)
#endif

#ifndef NO_SHARED_MEMORY
SDL_X11_MODULE(SHM)
SDL_X11_SYM(Status,XShmAttach,(Display* a,XShmSegmentInfo* b),(a,b),return)
SDL_X11_SYM(Status,XShmDetach,(Display* a,XShmSegmentInfo* b),(a,b),return)
SDL_X11_SYM(Status,XShmPutImage,(Display* a,Drawable b,GC c,XImage* d,int e,int f,int g,int h,unsigned int i,unsigned int j,Bool k),(a,b,c,d,e,f,g,h,i,j,k),return)
SDL_X11_SYM(XImage*,XShmCreateImage,(Display* a,Visual* b,unsigned int c,int d,char* e,XShmSegmentInfo* f,unsigned int g,unsigned int h),(a,b,c,d,e,f,g,h),return)
SDL_X11_SYM(Pixmap,XShmCreatePixmap,(Display *a,Drawable b,char* c,XShmSegmentInfo* d, unsigned int e, unsigned int f, unsigned int g),(a,b,c,d,e,f,g),return)
SDL_X11_SYM(Bool,XShmQueryExtension,(Display* a),(a),return)
#endif

/*
 * Not required...these only exist in code in headers on some 64-bit platforms,
 *  and are removed via macros elsewhere, so it's safe for them to be missing.
 */
#ifdef LONG64
SDL_X11_MODULE(IO_32BIT)
#if SDL_VIDEO_DRIVER_X11_CONST_PARAM_XDATA32
SDL_X11_SYM(int,_XData32,(Display *dpy,register _Xconst long *data,unsigned len),(dpy,data,len),return)
#else
SDL_X11_SYM(int,_XData32,(Display *dpy,register long *data,unsigned len),(dpy,data,len),return)
#endif
SDL_X11_SYM(void,_XRead32,(Display *dpy,register long *data,long len),(dpy,data,len),)
#endif

/*
 * These only show up on some variants of Unix.
 */
#if defined(__osf__)
SDL_X11_MODULE(OSF_ENTRY_POINTS)
SDL_X11_SYM(void,_SmtBufferOverflow,(Display *dpy,register smtDisplayPtr p),(dpy,p),)
SDL_X11_SYM(void,_SmtIpError,(Display *dpy,register smtDisplayPtr p,int i),(dpy,p,i),)
SDL_X11_SYM(int,ipAllocateData,(ChannelPtr a,IPCard b,IPDataPtr * c),(a,b,c),return)
SDL_X11_SYM(int,ipUnallocateAndSendData,(ChannelPtr a,IPCard b),(a,b),return)
#endif

/* XCursor support */
#if SDL_VIDEO_DRIVER_X11_XCURSOR
SDL_X11_MODULE(XCURSOR)
SDL_X11_SYM(XcursorImage*,XcursorImageCreate,(int a,int b),(a,b),return)
SDL_X11_SYM(void,XcursorImageDestroy,(XcursorImage *a),(a),)
SDL_X11_SYM(Cursor,XcursorImageLoadCursor,(Display *a,const XcursorImage *b),(a,b),return)
#endif

/* Xinerama support */
#if SDL_VIDEO_DRIVER_X11_XINERAMA
SDL_X11_MODULE(XINERAMA)
SDL_X11_SYM(Bool,XineramaIsActive,(Display *a),(a),return)
SDL_X11_SYM(Bool,XineramaQueryExtension,(Display *a,int *b,int *c),(a,b,c),return)
SDL_X11_SYM(Status,XineramaQueryVersion,(Display *a,int *b,int *c),(a,b,c),return)
SDL_X11_SYM(XineramaScreenInfo*,XineramaQueryScreens,(Display *a, int *b),(a,b),return)
#endif

/* XInput2 support for multiple mice, tablets, etc. */
#if SDL_VIDEO_DRIVER_X11_XINPUT2
SDL_X11_MODULE(XINPUT2)
SDL_X11_SYM(XIDeviceInfo*,XIQueryDevice,(Display *a,int b,int *c),(a,b,c),return)
SDL_X11_SYM(void,XIFreeDeviceInfo,(XIDeviceInfo *a),(a),)
SDL_X11_SYM(int,XISelectEvents,(Display *a,Window b,XIEventMask *c,int d),(a,b,c,d),return)
SDL_X11_SYM(Status,XIQueryVersion,(Display *a,int *b,int *c),(a,b,c),return)
SDL_X11_SYM(XIEventMask*,XIGetSelectedEvents,(Display *a,Window b,int *c),(a,b,c),return)
#endif

/* XRandR support */
#if SDL_VIDEO_DRIVER_X11_XRANDR
SDL_X11_MODULE(XRANDR)
SDL_X11_SYM(Status,XRRQueryVersion,(Display *dpy,int *major_versionp,int *minor_versionp),(dpy,major_versionp,minor_versionp),return)
SDL_X11_SYM(XRRScreenConfiguration *,XRRGetScreenInfo,(Display *dpy,Drawable draw),(dpy,draw),return)
SDL_X11_SYM(SizeID,XRRConfigCurrentConfiguration,(XRRScreenConfiguration *config,Rotation *rotation),(config,rotation),return)
SDL_X11_SYM(short,XRRConfigCurrentRate,(XRRScreenConfiguration *config),(config),return)
SDL_X11_SYM(short *,XRRConfigRates,(XRRScreenConfiguration *config,int sizeID,int *nrates),(config,sizeID,nrates),return)
SDL_X11_SYM(XRRScreenSize *,XRRConfigSizes,(XRRScreenConfiguration *config,int *nsizes),(config,nsizes),return)
SDL_X11_SYM(Status,XRRSetScreenConfigAndRate,(Display *dpy,XRRScreenConfiguration *config,Drawable draw,int size_index,Rotation rotation,short rate,Time timestamp),(dpy,config,draw,size_index,rotation,rate,timestamp),return)
SDL_X11_SYM(void,XRRFreeScreenConfigInfo,(XRRScreenConfiguration *config),(config),)
#endif

/* MIT-SCREEN-SAVER support */
#if SDL_VIDEO_DRIVER_X11_XSCRNSAVER
SDL_X11_MODULE(XSS)
SDL_X11_SYM(Bool,XScreenSaverQueryExtension,(Display *dpy,int *event_base,int *error_base),(dpy,event_base,error_base),return)
SDL_X11_SYM(Status,XScreenSaverQueryVersion,(Display *dpy,int *major_versionp,int *minor_versionp),(dpy,major_versionp,minor_versionp),return)
SDL_X11_SYM(void,XScreenSaverSuspend,(Display *dpy,Bool suspend),(dpy,suspend),return)
#endif

#if SDL_VIDEO_DRIVER_X11_XSHAPE
SDL_X11_MODULE(XSHAPE)
SDL_X11_SYM(void,XShapeCombineMask,(Display *dpy,Window dest,int dest_kind,int x_off,int y_off,Pixmap src,int op),(dpy,dest,dest_kind,x_off,y_off,src,op),)
#endif

#if SDL_VIDEO_DRIVER_X11_XVIDMODE
SDL_X11_MODULE(XVIDMODE)
SDL_X11_SYM(Bool,XF86VidModeGetAllModeLines,(Display *a,int b,int *c,XF86VidModeModeInfo ***d),(a,b,c,d),return)
SDL_X11_SYM(Bool,XF86VidModeGetModeLine,(Display *a,int b,int *c,XF86VidModeModeLine *d),(a,b,c,d),return)
SDL_X11_SYM(Bool,XF86VidModeGetViewPort,(Display *a,int b,int *c,int *d),(a,b,c,d),return)
SDL_X11_SYM(Bool,XF86VidModeQueryExtension,(Display *a,int *b,int *c),(a,b,c),return)
SDL_X11_SYM(Bool,XF86VidModeQueryVersion,(Display *a,int *b,int *c),(a,b,c),return)
SDL_X11_SYM(Bool,XF86VidModeSwitchToMode,(Display *a,int b,XF86VidModeModeInfo *c),(a,b,c),return)
#endif

/* *INDENT-ON* */

/* vi: set ts=4 sw=4 expandtab: */
