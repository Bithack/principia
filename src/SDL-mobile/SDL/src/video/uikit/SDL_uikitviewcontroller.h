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

#import <UIKit/UIKit.h>
#import "TSAlertView.h"

#include "../SDL_sysvideo.h"

@interface SDL_uikitviewcontroller : UIViewController <UIActionSheetDelegate, TSAlertViewDelegate, UIAlertViewDelegate> {
@private
    SDL_Window *window;
}

@property (readwrite) SDL_Window *window;

- (id)initWithSDLWindow:(SDL_Window *)_window;
- (void)loadView;
- (void)viewDidLayoutSubviews;
- (NSUInteger)supportedInterfaceOrientations;
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orient;
- (void)showActionSheet;
- (void)showActionSheet_Play;
- (void)showActionSheet_Community;
- (void)showActionSheet_Testplay;

- (void)alertView:(TSAlertView *)alertView clickedButtonAtIndexTS:(NSInteger)buttonIndex;
- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex;

@end

extern SDL_uikitviewcontroller *g_SDL_viewcontroller;