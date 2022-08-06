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

#if SDL_VIDEO_DRIVER_UIKIT

#import "../SDL_sysvideo.h"
#import "SDL_assert.h"
#import "SDL_hints.h"
#import "../../SDL_hints_c.h"
#import "SDL_system.h"

#import "SDL_uikitappdelegate.h"
#import "SDL_uikitopenglview.h"
#import "../../events/SDL_events_c.h"
#import "jumphack.h"

#ifdef main
#undef main
#endif

extern int SDL_main(int argc, char *argv[]);
static int forward_argc;
static char **forward_argv;
static int exit_status;

int main(int argc, char **argv)
{
    int i;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    /* store arguments */
    forward_argc = argc;
    forward_argv = (char **)malloc((argc+1) * sizeof(char *));
    for (i = 0; i < argc; i++) {
        forward_argv[i] = malloc( (strlen(argv[i])+1) * sizeof(char));
        strcpy(forward_argv[i], argv[i]);
    }
    forward_argv[i] = NULL;

    /* Give over control to run loop, SDLUIKitDelegate will handle most things from here */
    UIApplicationMain(argc, argv, NULL, [SDLUIKitDelegate getAppDelegateClassName]);

    /* free the memory we used to hold copies of argc and argv */
    for (i = 0; i < forward_argc; i++) {
        free(forward_argv[i]);
    }
    free(forward_argv);

    [pool release];
    return exit_status;
}

static void SDL_IdleTimerDisabledChanged(const char *name, const char *oldValue, const char *newValue)
{
    SDL_assert(SDL_strcmp(name, SDL_HINT_IDLE_TIMER_DISABLED) == 0);

    BOOL disable = (*newValue != '0');
    [UIApplication sharedApplication].idleTimerDisabled = disable;
}

@implementation SDLUIKitDelegate

/* convenience method */
+ (id) sharedAppDelegate
{
    /* the delegate is set in UIApplicationMain(), which is garaunteed to be called before this method */
    return [[UIApplication sharedApplication] delegate];
}

+ (NSString *)getAppDelegateClassName
{
    /* subclassing notice: when you subclass this appdelegate, make sure to add a category to override
       this method and return the actual name of the delegate */
    return @"SDLUIKitDelegate";
}

- (id)init
{
    self = [super init];
    return self;
}

- (void)postFinishLaunch
{
    /* run the user's application, passing argc and argv */
    printf("postFinishLaunch\n");
    SDL_iPhoneSetEventPump(SDL_TRUE);
    exit_status = SDL_main(forward_argc, forward_argv);
    SDL_iPhoneSetEventPump(SDL_FALSE);

    /* exit, passing the return status from the user's application */
    // We don't actually exit to support applications that do setup in
    // their main function and then allow the Cocoa event loop to run.
    // exit(exit_status);
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    printf("didFinishLaunchingWithOptions\n");

    /* Set working directory to resource path */
    [[NSFileManager defaultManager] changeCurrentDirectoryPath: [[NSBundle mainBundle] resourcePath]];

    /* register a callback for the idletimer hint */
    SDL_SetHint(SDL_HINT_IDLE_TIMER_DISABLED, "0");
    SDL_RegisterHintChangedCb(SDL_HINT_IDLE_TIMER_DISABLED, &SDL_IdleTimerDisabledChanged);

    [self performSelector:@selector(postFinishLaunch) withObject:nil afterDelay:0.0];

    return YES;
}

static char *_tmp[2];
- (BOOL)application:(UIApplication *)application handleOpenURL:(NSURL *)url
{
    _tmp[0] = "";
    
    printf("received url\n");
    
    NSString *s = [url absoluteString];
    _tmp[1] = (char*)[s cString];
    
    tproject_set_args(2, _tmp);
    
    
    printf("received url 2\n");

    
    //[s release];
    printf("received url 3\n");

    return YES;
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    SDL_SendQuit();
     /* hack to prevent automatic termination.  See SDL_uikitevents.m for details */
    longjmp(*(jump_env()), 1);
}

- (void) applicationWillResignActive:(UIApplication*)application
{
    //NSLog(@"%@", NSStringFromSelector(_cmd));

    // Send every window on every screen a MINIMIZED event.
    SDL_VideoDevice *_this = SDL_GetVideoDevice();
    if (!_this) {
        return;
    }

    SDL_Window *window;
    for (window = _this->windows; window != nil; window = window->next) {
        SDL_SendWindowEvent(window, SDL_WINDOWEVENT_FOCUS_LOST, 0, 0);
        SDL_SendWindowEvent(window, SDL_WINDOWEVENT_MINIMIZED, 0, 0);
    }
}

- (void) applicationDidBecomeActive:(UIApplication*)application
{
    //NSLog(@"%@", NSStringFromSelector(_cmd));

    // Send every window on every screen a RESTORED event.
    SDL_VideoDevice *_this = SDL_GetVideoDevice();
    if (!_this) {
        return;
    }

    SDL_Window *window;
    for (window = _this->windows; window != nil; window = window->next) {
        SDL_SendWindowEvent(window, SDL_WINDOWEVENT_FOCUS_GAINED, 0, 0);
        SDL_SendWindowEvent(window, SDL_WINDOWEVENT_RESTORED, 0, 0);
    }
}


@end

#endif /* SDL_VIDEO_DRIVER_UIKIT */

/* vi: set ts=4 sw=4 expandtab: */
