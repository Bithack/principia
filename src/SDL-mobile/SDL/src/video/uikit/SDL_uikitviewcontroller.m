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

#include "main.hh"
#include "ui.hh"
#include "SDL_video.h"
#include "SDL_assert.h"
#include "SDL_hints.h"
#include "../SDL_sysvideo.h"
#include "../../events/SDL_events_c.h"

#include "SDL_uikitwindow.h"
#include "SDL_uikitviewcontroller.h"
#include "SDL_uikitvideo.h"


int tbackend_is_tablet(void)
{
    return (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);
}

SDL_uikitviewcontroller *g_SDL_viewcontroller;

#ifndef __IPHONE_6_0
// This enum isn't available in older SDKs, but we use it for our own purposes on iOS 5.1 and for the system on iOS 6.0
enum UIInterfaceOrientationMask
{
    UIInterfaceOrientationMaskPortrait = (1 << UIInterfaceOrientationPortrait),
    UIInterfaceOrientationMaskLandscapeLeft = (1 << UIInterfaceOrientationLandscapeLeft),
    UIInterfaceOrientationMaskLandscapeRight = (1 << UIInterfaceOrientationLandscapeRight),
    UIInterfaceOrientationMaskPortraitUpsideDown = (1 << UIInterfaceOrientationPortraitUpsideDown),
    UIInterfaceOrientationMaskLandscape = (UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight),
    UIInterfaceOrientationMaskAll = (UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight | UIInterfaceOrientationMaskPortraitUpsideDown),
    UIInterfaceOrientationMaskAllButUpsideDown = (UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight),
};
#endif // !__IPHONE_6_0

    
@implementation SDL_uikitviewcontroller

@synthesize window;

- (id)initWithSDLWindow:(SDL_Window *)_window
{
    self = [self init];
    if (self == nil) {
        return nil;
    }
    self.window = _window;

    return self;
}

- (void)loadView
{
    // do nothing.
}

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
    P_focus(1);
    if (actionSheet.tag == 0) {
        /* sandbox menu */
        switch (buttonIndex) {
            default:
                ui_cb_menu_item_selected((int)buttonIndex);
        }
        
    } else if (actionSheet.tag == 2) {
        /* community or not menu */
        switch (buttonIndex) {
            case 0:
                ui_cb_back_to_community();
                break;
                
            case 1:
                P_add_action(ACTION_GOTO_MAINMENU, 0);
                break;
        }
    } else if (actionSheet.tag == 3) {
        /* testplay */
        switch (buttonIndex) {
            case 0:
                P_add_action(ACTION_PUZZLEPLAY, 0);
                break;
                
            case 1:
                P_add_action(ACTION_PUZZLEPLAY, 1);
                break;
        }
    } else if (actionSheet.tag == 1) {
        /* play menu */
        switch (buttonIndex) {
            case 0:
                P_add_action(ACTION_RESTART_LEVEL, 0);
                break;
                
            case 1:
                P_add_action(ACTION_BACK, 0);
                break;
        }
    }
    
}

char *sho_nsstring_to_cstring(NSString *s)
{
    NSUInteger len = [s lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    NSUInteger tja;
    char *tmp =  malloc(len+1);
    NSRange r = NSMakeRange(0, [s length]);
    [s getBytes:tmp maxLength:len usedLength:&tja encoding:NSUTF8StringEncoding options:0 range:r remainingRange:NULL];
    tmp[len] = '\0';
    return tmp;
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    P_focus(1);
    switch (alertView.tag) {
            
        case DIALOG_JUMPER:
        {
            float value = [[[alertView textFieldAtIndex:0] text] floatValue];
            if (value > 1.f) value = 1.f;
            else if (value < 0.f) value = 0.f;
            ui_set_property_float(0, value);
            ui_cb_update_jumper();
            P_add_action(ACTION_HIGHLIGHT_SELECTED, 0);
            P_add_action(ACTION_RESELECT, 0);
        }
            break;
        case DIALOG_SAVE_COPY:
            if (buttonIndex == 1) {
                char *name = sho_nsstring_to_cstring([alertView textFieldAtIndex:0].text);
                if (!strlen(name)) {
                    ui_message("Please specify a name for the level.", false);
                    ui_open_dialog(DIALOG_SAVE_COPY);
                } else {
                    ui_cb_set_level_title(name);
                    P_add_action(ACTION_SAVE_COPY, 0);
                }
                free(name);
            }
            break;
            
        case DIALOG_SAVE:
            if (buttonIndex == 1) {
                char *name = sho_nsstring_to_cstring([alertView textFieldAtIndex:0].text);
                if (!strlen(name)) {
                    ui_message("Please specify a name for the level.", false);
                    ui_open_dialog(DIALOG_SAVE);
                } else {
                    ui_cb_set_level_title(name);
                    P_add_action(ACTION_SAVE, 0);
                }
                free(name);
            }
            break;
            
        case DIALOG_EXPORT:
            if (buttonIndex == 1) {
                char *name = sho_nsstring_to_cstring([alertView textFieldAtIndex:0].text);
                if (!strlen(name)) {
                    ui_message("Please specify a name for the level.", false);
                    ui_open_dialog(DIALOG_EXPORT);
                } else {
                    P_add_action(ACTION_EXPORT_OBJECT, strdup(name));
                }
                free(name);
            }
            break;
            
        case DIALOG_OPEN_AUTOSAVE:
            if (buttonIndex == 1) {
                P_add_action(ACTION_OPEN_AUTOSAVE,0);
            } else if (buttonIndex == 2) {
                NSLog(@"Removing autosave");
                P_add_action(ACTION_REMOVE_AUTOSAVE,0);
            } else
                NSLog(@"Ignoring autosave");
            break;
            
            
        case DIALOG_NEW_LEVEL:
            {
                int level_type = 2;
                if (buttonIndex == 1) level_type = 2;
                else if (buttonIndex == 2) level_type = 1;
                else if (buttonIndex == 3) level_type = 0;
                else {
                    NSLog(@"Not creating new level.");
                    break;
                }
                P_add_action(ACTION_NEW_LEVEL, (void*)level_type);
            }
            break;
            
        case DIALOG_MAIN_MENU_PKG:
        {
            switch (buttonIndex) {
                case 1:
                    P_add_action(ACTION_MAIN_MENU_PKG, 0);
                    break;
                case 2:
                    P_add_action(ACTION_MAIN_MENU_PKG, 1);
                    break;
            }
            
        }
            break;
            
        case DIALOG_SANDBOX_MODE:
        {
            int mode = 0;
            switch (buttonIndex) {
                case 1: /* conn edit */
                    mode = 16;
                    break;
                case 2: /* Multiselect */
                    mode = 6;
                    break;
                case 3: /* terrain */
                    mode = 13;
                    break;
            }
            
            if (mode != 0)
                P_add_action(ACTION_SET_MODE, (void*)mode);
        }
            break;
            
    }
}

- (void)alertView:(TSAlertView *)alertView clickedButtonAtIndexTS:(NSInteger)buttonIndex
{
    P_focus(1);
    switch (alertView.tag) {
        case DIALOG_PROMPT:
        {
            int y = 0;
            for (int x=0; x<3; x++) {
                if (_prompt_buttons[x] && strlen(_prompt_buttons[x])) {
                    if (y == buttonIndex) {
                        ui_cb_prompt_response(x+1);
                        NSLog(@"setting response to %d", x);
                        break;
                    }
                    y ++;
                }
            }
            P_focus(true);
        }
            break;
        case DIALOG_CONSUMABLE: ui_cb_set_consumable([alertView getSelected]); break;
        case DIALOG_SET_COMMAND: ui_cb_set_command([alertView getSelected]); break;
        case DIALOG_CAMTARGETER: ui_cb_set_followmode([alertView getSelected]); break;
        case DIALOG_EVENTLISTENER: ui_cb_set_event([alertView getSelected]); break;
        case DIALOG_FXEMITTER: ui_cb_set_fx(0, [alertView getSelected]); ui_cb_set_fx(1, [alertView getSelected_2]); break;

    }
}

- (void)showActionSheet
{
    NSString *actionSheetTitle = @"Menu";
    UIActionSheet *actionSheet = [[UIActionSheet alloc]
                                  initWithTitle:actionSheetTitle
                                  delegate:self
                                  cancelButtonTitle:@"Close"
                                  destructiveButtonTitle:nil
                                  otherButtonTitles:
                                  @"Level Properties",
                                  @"New level",
                                  @"Save",
                                  @"Save a copy",
                                  @"Open",
                                  @"Publish online",
                                  @"Settings",
                                  @"Back to menu",
                                  nil];
    actionSheet.tag = 0;
    P_focus(0);
    [actionSheet showInView:self.view];
    printf("SHOWING\n");
}

- (void)showActionSheet_Play
{
    NSString *actionSheetTitle = @"Menu";
    UIActionSheet *actionSheet = [[UIActionSheet alloc]
                                  initWithTitle:actionSheetTitle
                                  delegate:self
                                  cancelButtonTitle:@"Close"
                                  destructiveButtonTitle:nil
                                  otherButtonTitles:
                                  @"Restart level",
                                  @"Go back",
                                  nil];
    actionSheet.tag = 1;
    P_focus(0);
    [actionSheet showInView:self.view];
    printf("SHOWING\n");
}

- (void)showActionSheet_Community
{
    NSString *actionSheetTitle = @"Menu";
    UIActionSheet *actionSheet = [[UIActionSheet alloc]
                                  initWithTitle:actionSheetTitle
                                  delegate:self
                                  cancelButtonTitle:@"Close"
                                  destructiveButtonTitle:nil
                                  otherButtonTitles:
                                  @"Back to Community Website",
                                  @"Back to Menu",
                                  nil];
    actionSheet.tag = 2;
    P_focus(0);
    [actionSheet showInView:self.view];
    printf("SHOWING\n");
}

- (void)showActionSheet_Testplay
{
    NSString *actionSheetTitle = @"Menu";
    UIActionSheet *actionSheet = [[UIActionSheet alloc]
                                  initWithTitle:actionSheetTitle
                                  delegate:self
                                  cancelButtonTitle:@"Close"
                                  destructiveButtonTitle:nil
                                  otherButtonTitles:
                                  @"Test-play",
                                  @"Simulate level",
                                  nil];
    actionSheet.tag = 3;
    P_focus(0);
    [actionSheet showInView:self.view];
    printf("SHOWING\n");
}

- (void)viewDidLayoutSubviews
{
    if (self->window->flags & SDL_WINDOW_RESIZABLE) {
        SDL_WindowData *data = self->window->driverdata;
        SDL_VideoDisplay *display = SDL_GetDisplayForWindow(self->window);
        SDL_DisplayModeData *displaymodedata = (SDL_DisplayModeData *) display->current_mode.driverdata;
        const CGSize size = data->view.bounds.size;
        int w, h;
        
        w = (int)(size.width * displaymodedata->scale);
        h = (int)(size.height * displaymodedata->scale);
        
        SDL_SendWindowEvent(self->window, SDL_WINDOWEVENT_RESIZED, w, h);
    }
}



- (NSUInteger)supportedInterfaceOrientations
{
    NSUInteger orientationMask = 0;
	return UIInterfaceOrientationMaskLandscapeRight;
    
    const char *orientationsCString;
    if ((orientationsCString = SDL_GetHint(SDL_HINT_ORIENTATIONS)) != NULL) {
        BOOL rotate = NO;
        NSString *orientationsNSString = [NSString stringWithCString:orientationsCString
                                                            encoding:NSUTF8StringEncoding];
        NSArray *orientations = [orientationsNSString componentsSeparatedByCharactersInSet:
                                 [NSCharacterSet characterSetWithCharactersInString:@" "]];
        
        if ([orientations containsObject:@"LandscapeLeft"]) {
            orientationMask |= UIInterfaceOrientationMaskLandscapeLeft;
        }
        if ([orientations containsObject:@"LandscapeRight"]) {
            orientationMask |= UIInterfaceOrientationMaskLandscapeRight;
        }
        if ([orientations containsObject:@"Portrait"]) {
            orientationMask |= UIInterfaceOrientationMaskPortrait;
        }
        if ([orientations containsObject:@"PortraitUpsideDown"]) {
            orientationMask |= UIInterfaceOrientationMaskPortraitUpsideDown;
        }
        
    } else if (self->window->flags & SDL_WINDOW_RESIZABLE) {
        orientationMask = UIInterfaceOrientationMaskAll;  // any orientation is okay.
    } else {
        if (self->window->w >= self->window->h) {
            orientationMask |= UIInterfaceOrientationMaskLandscape;
        }
        if (self->window->h >= self->window->w) {
            orientationMask |= (UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown);
        }
    }
    
    // Don't allow upside-down orientation on the phone, so answering calls is in the natural orientation
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        orientationMask &= ~UIInterfaceOrientationMaskPortraitUpsideDown;
    }
    return orientationMask;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orient
{
    NSUInteger orientationMask = [self supportedInterfaceOrientations];
    return (orientationMask & (1 << orient));
}

@end

#endif /* SDL_VIDEO_DRIVER_UIKIT */

/* vi: set ts=4 sw=4 expandtab: */
