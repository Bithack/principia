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

#if SDL_VIDEO_DRIVER_COCOA
#include "SDL_timer.h"

#include "SDL_cocoavideo.h"
#include "../../events/SDL_events_c.h"

#if !defined(UsrActivity) && defined(__LP64__) && !defined(__POWER__)
/*
 * Workaround for a bug in the 10.5 SDK: By accident, OSService.h does
 * not include Power.h at all when compiling in 64bit mode. This has
 * been fixed in 10.6, but for 10.5, we manually define UsrActivity
 * to ensure compilation works.
 */
#define UsrActivity 1
#endif

/* setAppleMenu disappeared from the headers in 10.4 */
@interface NSApplication(NSAppleMenu)
- (void)setAppleMenu:(NSMenu *)menu;
@end

@interface SDLAppDelegate : NSObject
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
@end

@implementation SDLAppDelegate : NSObject
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    SDL_SendQuit();
    return NSTerminateCancel;
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
    return (BOOL)SDL_SendDropFile([filename UTF8String]);
}
@end

static NSString *
GetApplicationName(void)
{
    NSDictionary *dict;
    NSString *appName = 0;

    /* Determine the application name */
    dict = (NSDictionary *)CFBundleGetInfoDictionary(CFBundleGetMainBundle());
    if (dict)
        appName = [dict objectForKey: @"CFBundleName"];
    
    if (![appName length])
        appName = [[NSProcessInfo processInfo] processName];

    return appName;
}

static void
CreateApplicationMenus(void)
{
    NSString *appName;
    NSString *title;
    NSMenu *appleMenu;
    NSMenu *windowMenu;
    NSMenuItem *menuItem;
    
    /* Create the main menu bar */
    [NSApp setMainMenu:[[NSMenu alloc] init]];

    /* Create the application menu */
    appName = GetApplicationName();
    appleMenu = [[NSMenu alloc] initWithTitle:@""];
    
    /* Add menu items */
    title = [@"About " stringByAppendingString:appName];
    [appleMenu addItemWithTitle:title action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];

    [appleMenu addItem:[NSMenuItem separatorItem]];

    [appleMenu addItemWithTitle:@"Preferences" action:nil keyEquivalent:@""];

    [appleMenu addItem:[NSMenuItem separatorItem]];

    title = [@"Hide " stringByAppendingString:appName];
    [appleMenu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@/*"h"*/""];

    menuItem = (NSMenuItem *)[appleMenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@/*"h"*/""];
    [menuItem setKeyEquivalentModifierMask:(NSAlternateKeyMask|NSCommandKeyMask)];

    [appleMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];

    [appleMenu addItem:[NSMenuItem separatorItem]];

    title = [@"Quit " stringByAppendingString:appName];
    [appleMenu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@/*"q"*/""];
    
    /* Put menu into the menubar */
    menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:appleMenu];
    [[NSApp mainMenu] addItem:menuItem];
    [menuItem release];

    /* Tell the application object that this is now the application menu */
    [NSApp setAppleMenu:appleMenu];
    [appleMenu release];


    /* Create the window menu */
    windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
    
    /* "Minimize" item */
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@/*"m"*/""];
    [windowMenu addItem:menuItem];
    [menuItem release];
    
    /* Put menu into the menubar */
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:windowMenu];
    [[NSApp mainMenu] addItem:menuItem];
    [menuItem release];
    
    /* Tell the application object that this is now the window menu */
    [NSApp setWindowsMenu:windowMenu];
    [windowMenu release];
}

void
Cocoa_RegisterApp(void)
{
    ProcessSerialNumber psn;
    NSAutoreleasePool *pool;

    if (!GetCurrentProcess(&psn)) {
        TransformProcessType(&psn, kProcessTransformToForegroundApplication);
        SetFrontProcess(&psn);
    }

    pool = [[NSAutoreleasePool alloc] init];
    if (NSApp == nil) {
        [NSApplication sharedApplication];

        if ([NSApp mainMenu] == nil) {
            CreateApplicationMenus();
        }
        [NSApp finishLaunching];
    }
    if ([NSApp delegate] == nil) {
        [NSApp setDelegate:[[SDLAppDelegate alloc] init]];
    }
    [pool release];
}

void
Cocoa_PumpEvents(_THIS)
{
    NSAutoreleasePool *pool;

    /* Update activity every 30 seconds to prevent screensaver */
    if (_this->suspend_screensaver) {
        SDL_VideoData *data = (SDL_VideoData *)_this->driverdata;
        Uint32 now = SDL_GetTicks();
        if (!data->screensaver_activity ||
            (int)(now-data->screensaver_activity) >= 30000) {
            UpdateSystemActivity(UsrActivity);
            data->screensaver_activity = now;
        }
    }

    pool = [[NSAutoreleasePool alloc] init];
    for ( ; ; ) {
        NSEvent *event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES ];
        if ( event == nil ) {
            break;
        }
		
        switch ([event type]) {
        case NSLeftMouseDown:
        case NSOtherMouseDown:
        case NSRightMouseDown:
        case NSLeftMouseUp:
        case NSOtherMouseUp:
        case NSRightMouseUp:
        case NSLeftMouseDragged:
        case NSRightMouseDragged:
        case NSOtherMouseDragged: /* usually middle mouse dragged */
        case NSMouseMoved:
        case NSScrollWheel:
            Cocoa_HandleMouseEvent(_this, event);
            /* Pass through to NSApp to make sure everything stays in sync */
            [NSApp sendEvent:event];
            break;
        case NSKeyDown:
        case NSKeyUp:
        case NSFlagsChanged:
            Cocoa_HandleKeyEvent(_this, event);
            /* Fall through to pass event to NSApp; er, nevermind... */

            /* Add to support system-wide keyboard shortcuts like CMD+Space */
            if (([event modifierFlags] & NSCommandKeyMask) || [event type] == NSFlagsChanged)
               [NSApp sendEvent: event];
            break;
        default:
            [NSApp sendEvent:event];
            break;
        }
    }
    [pool release];
}

#endif /* SDL_VIDEO_DRIVER_COCOA */

/* vi: set ts=4 sw=4 expandtab: */
