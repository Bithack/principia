

#include "ui.hh"
#include <tms/core/tms.h>
#include "SDL/src/video/uikit/SDL_uikitviewcontroller.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "TSAlertView.h"
#import "FXEmitterController.h"
#import "SFXEmitterController.h"
#import "SynthesizerController.h"
#import "RobotController.h"
#import "InfoController.h"
#import "VarController.h"
#import "ColorController.h"
#import "DisplayController.h"
#import "SettingsController.h"
#import "PublishController.h"
#import "RegisterController.h"
#import "LoginController.h"
#import "PublishedController.h"
#import "TimerController.h"
#import "SequencerController.h"
#import "PromptEditController.h"
#import "FrequencyController.h"
#import "FreqRangeController.h"
#import "ImportController.h"
#import "StickyController.h"
#import "CursorfieldController.h"
#import "EscriptController.h"
#import "QuickaddController.h"
#import "TipsTricksController.h"
#import "CommunityController.h"
#import "RubberController.h"
#import "ShapeExtruderController.h"

#include "main.hh"

typedef enum iToastGravity {
	iToastGravityTop = 1000001,
	iToastGravityBottom,
	iToastGravityCenter
}iToastGravity;

enum iToastDuration {
	iToastDurationLong = 10000,
	iToastDurationShort = 1000,
	iToastDurationNormal = 3000
}iToastDuration;

typedef enum iToastType {
	iToastTypeInfo = -100000,
	iToastTypeNotice,
	iToastTypeWarning,
	iToastTypeError,
	iToastTypeNone // For internal use only (to force no image)
}iToastType;

typedef enum {
    iToastImageLocationTop,
    iToastImageLocationLeft
} iToastImageLocation;


@class iToastSettings;

@interface iToast : NSObject {
	iToastSettings *_settings;
	
	NSTimer *timer;
	
	UIView *view;
	NSString *text;
}

- (void) show;
- (void) show:(iToastType) type;
- (iToast *) setDuration:(NSInteger ) duration;
- (iToast *) setGravity:(iToastGravity) gravity
			 offsetLeft:(NSInteger) left
              offsetTop:(NSInteger) top;
- (iToast *) setGravity:(iToastGravity) gravity;
- (iToast *) setPostion:(CGPoint) position;
- (iToast *) setFontSize:(CGFloat) fontSize;
- (iToast *) setUseShadow:(BOOL) useShadow;
- (iToast *) setCornerRadius:(CGFloat) cornerRadius;
- (iToast *) setBgRed:(CGFloat) bgRed;
- (iToast *) setBgGreen:(CGFloat) bgGreen;
- (iToast *) setBgBlue:(CGFloat) bgBlue;
- (iToast *) setBgAlpha:(CGFloat) bgAlpha;

+ (iToast *) makeText:(NSString *) text;

-(iToastSettings *) theSettings;

@end

@interface iToastSettings : NSObject<NSCopying>{
	NSInteger duration;
	iToastGravity gravity;
	CGPoint postition;
	iToastType toastType;
	CGFloat fontSize;
	BOOL useShadow;
	CGFloat cornerRadius;
	CGFloat bgRed;
	CGFloat bgGreen;
	CGFloat bgBlue;
	CGFloat bgAlpha;
	NSInteger offsetLeft;
	NSInteger offsetTop;
    
	NSDictionary *images;
	
	BOOL positionIsSet;
}


@property(assign) NSInteger duration;
@property(assign) iToastGravity gravity;
@property(assign) CGPoint postition;
@property(assign) CGFloat fontSize;
@property(assign) BOOL useShadow;
@property(assign) CGFloat cornerRadius;
@property(assign) CGFloat bgRed;
@property(assign) CGFloat bgGreen;
@property(assign) CGFloat bgBlue;
@property(assign) CGFloat bgAlpha;
@property(assign) NSInteger offsetLeft;
@property(assign) NSInteger offsetTop;
@property(readonly) NSDictionary *images;
@property(assign) iToastImageLocation imageLocation;


- (void) setImage:(UIImage *)img forType:(iToastType) type;
- (void) setImage:(UIImage *)img withLocation:(iToastImageLocation)location forType:(iToastType)type;
+ (iToastSettings *) getSharedSettings;

@end

/* ------------------------------------ */

#import <QuartzCore/QuartzCore.h>
#import "OpenViewController.h"
#import "LevelPropertiesController.h"

void ui_message(const char *msg, bool long_duration)
{
    [[[iToast makeText:[NSString stringWithUTF8String:msg]] setDuration:(long_duration ? iToastDurationLong : iToastDurationNormal)] show];
}

void ui_init(){};

void ui_open_sandbox_tips()
{
    TipsTricksController *f = [[TipsTricksController alloc] initWithNibName:@"TipsTricksController" bundle:nil];
    f.modalPresentationStyle = UIModalPresentationFormSheet;
    [g_SDL_viewcontroller presentModalViewController:f animated:YES];
}

void ui_open_dialog(int num)
{
    tms_infof("open dialog %d", num);

    switch (num) {
        case DIALOG_SANDBOX_MENU: [g_SDL_viewcontroller showActionSheet]; break;
        case DIALOG_PLAY_MENU: [g_SDL_viewcontroller showActionSheet_Play]; break;
        case DIALOG_COMMUNITY: [g_SDL_viewcontroller showActionSheet_Community]; break;
        case DIALOG_PUZZLE_PLAY: [g_SDL_viewcontroller showActionSheet_Testplay]; break;
            
        case DIALOG_LEVEL_PROPERTIES:
        {
            LevelPropertiesController *f = [[LevelPropertiesController alloc] initWithNibName:@"LevelPropertiesController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
        break;
            
        case DIALOG_PUBLISHED:
        {
            PublishedController *f = [[PublishedController alloc] initWithNibName:@"PublishedController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
        
        case DIALOG_OPEN:
        {
            OpenViewController *f = [[OpenViewController alloc] initWithNibName:@"OpenViewController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
        break;
            
        case DIALOG_PIXEL_COLOR:
        case DIALOG_BEAM_COLOR:
        {
            ColorController *f = [[ColorController alloc] initWithNibName:@"ColorController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
        break;
            
            
        case DIALOG_ESCRIPT:
        {
            EscriptController *f = [[EscriptController alloc] initWithNibName:@"EscriptController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFullScreen;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_STICKY:
        {
            StickyController *f = [[StickyController alloc] initWithNibName:@"StickyController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_ROBOT:
        {
            RobotController *f = [[RobotController alloc] initWithNibName:@"RobotController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_PUBLISH:
        {
            PublishController *f = [[PublishController alloc] initWithNibName:@"PublishController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_DIGITALDISPLAY:
        {
            DisplayController *f = [[DisplayController alloc] initWithNibName:@"DisplayController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_REGISTER:
        {
            RegisterController *f = [[RegisterController alloc] initWithNibName:@"RegisterController" bundle:nil];
            
            register_dialog = f;
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:NO];
        }
            break;
            
        case DIALOG_TIMER:
        {
            TimerController *f = [[TimerController alloc] initWithNibName:@"TimerController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_LOGIN:
        {
            LoginController *f = [[LoginController alloc] initWithNibName:@"LoginController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_SET_FREQ_RANGE:
        {
            FreqRangeController *f = [[FreqRangeController alloc] initWithNibName:@"FreqRangeController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_SET_FREQUENCY:
        {
            FrequencyController *f = [[FrequencyController alloc] initWithNibName:@"FrequencyController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_VARIABLE:
        {
            VarController *f = [[VarController alloc] initWithNibName:@"VarController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_PROMPT:
        {
            const char *buttons[3] ={0,0,0};
            int num_buttons =0;
            for (int x=0; x<3; x++) {
                if (_prompt_buttons[x] && strlen(_prompt_buttons[x])) {
                    buttons[num_buttons] = _prompt_buttons[x];
                    num_buttons++;
                }
            }
            TSAlertView *a = [[[TSAlertView alloc] initWithTitle:nil message:[NSString stringWithUTF8String:_prompt_text] delegate:g_SDL_viewcontroller cancelButtonTitle:nil otherButtonTitles:
                               buttons[0] ? [NSString stringWithUTF8String:buttons[0]] : nil,
                               buttons[1] ? [NSString stringWithUTF8String:buttons[1]] : nil,
                               buttons[2] ? [NSString stringWithUTF8String:buttons[2]] : nil,
                               nil] autorelease];
            a.buttonLayout = TSAlertViewButtonLayoutNormal;
            a.style = TSAlertViewStyleNormal;
            a.cancelButtonIndex = -1;
            a.tag = num;
            P_focus(0);
            [a show];
            break;
        }
            
        case DIALOG_PROMPT_SETTINGS:
        {
            PromptEditController *f = [[PromptEditController alloc] initWithNibName:@"PromptEditController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_SEQUENCER:
        {
            SequencerController *f = [[SequencerController alloc] initWithNibName:@"SequencerController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_SHAPEEXTRUDER:
        {
            ShapeExtruderController *f = [[ShapeExtruderController alloc] initWithNibName:@"ShapeExtruderController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_RUBBER:
        {
            RubberController *f = [[RubberController alloc] initWithNibName:@"RubberController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_SYNTHESIZER:
        {
            SynthesizerController *f = [[SynthesizerController alloc] initWithNibName:@"SynthesizerController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
        break;
            
        case DIALOG_SFXEMITTER:
        {
            SFXEmitterController *f = [[SFXEmitterController alloc] initWithNibName:@"SFXEmitterController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
        break;
            
            
        case DIALOG_SETTINGS:
        {
            SettingsController *f = [[SettingsController alloc] initWithNibName:@"SettingsController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_FXEMITTER:
        {
            FXEmitterController *f = [[FXEmitterController alloc] initWithNibName:@"FXEmitterController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
/*
            TSAlertView *a = [[[TSAlertView alloc] init] autorelease];
            
            a.title = @"FX Emitter";
            a.message = @"Select one or two effects to emit.";
            a.style = TSAlertViewStyleNormal;
            a.delegate = g_SDL_viewcontroller;
            a.pickerinitial_0 = ui_cb_get_fx(0);
            a.pickervalues_0 = @[ @"",
                                @"Explosion",
                                @"Highlight",
                                @"Destroy connections",
                                @"Smoke",
                                @"Magic",
                                @"Break",
                                  ];
            a.pickervalues_1 = a.pickervalues_0;
            a.pickerinitial_1 = ui_cb_get_fx(1);
            a.tag = num;
            
            [a addButtonWithTitle:@"Done"];
            [a show];*/
        }
        break;
        
        case DIALOG_EVENTLISTENER:
        {
            TSAlertView *a = [[[TSAlertView alloc] init] autorelease];
            
            a.title = @"Event Listener";
            a.message = @"Please specify what event to report.";
            a.style = TSAlertViewStyleNormal;
            a.delegate = g_SDL_viewcontroller;
            a.pickerinitial_0 = ui_cb_get_event();
            a.pickervalues_0 = @[
                               @"Player die",
                               @"Enemy die",
                               @"Interactive object destroyed",
                               @"Player respawn",
                               @"Touch/Mouse click",
                               @"Touch/Mouse release",
                               @"Any absorber activated",                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
                               ];
            a.tag = num;
            
            P_focus(0);
            [a addButtonWithTitle:@"Done"];
            [a show];
        }
            break;
            
        case DIALOG_TOUCHFIELD:
        {
            CursorfieldController *f = [[CursorfieldController alloc] initWithNibName:@"CursorfieldController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_CAMTARGETER:
        {
            TSAlertView *a = [[[TSAlertView alloc] init] autorelease];
            
            a.title = @"Camera Targeter";
            a.message = @"Please specify the follow mode.";
            a.style = TSAlertViewStyleNormal;
            a.delegate = g_SDL_viewcontroller;
            a.pickerinitial_0 = ui_cb_get_followmode();
            a.pickervalues_0 = @[
                               @"Smooth follow",
                               @"Snap to object",
                               @"Relative follow",
                               ];
            a.tag = num;
            
            [a addButtonWithTitle:@"Done"];
            P_focus(0);
            [a show];
        }
        break;
            
            
        case DIALOG_OPEN_AUTOSAVE:
        {
            UIAlertView *a = [[[UIAlertView alloc] initWithTitle:@"Auto-save detected" message:@"An autosave file was detected. Would you like to open it or delete it?" delegate:g_SDL_viewcontroller cancelButtonTitle:@"Ignore" otherButtonTitles:@"Open", @"Delete", nil] autorelease];
           
            a.tag = num;
            P_focus(0);
            [a show];
        }
        break;
            
        case DIALOG_JUMPER:
        {
            UIAlertView *a = [[[UIAlertView alloc] initWithTitle:@"Jumper" message:@"Please specify a precise value for this jumper." delegate:g_SDL_viewcontroller cancelButtonTitle:@"Cancel" otherButtonTitles:@"OK", nil] autorelease];
            a.alertViewStyle = UIAlertViewStylePlainTextInput;
            
            [a textFieldAtIndex:0].text = [NSString stringWithFormat:@"%f", ui_get_property_float(0)];
            [[a textFieldAtIndex:0] setKeyboardType:UIKeyboardTypeNumbersAndPunctuation];
            
            a.tag = num;
            P_focus(0);
            [a show];
            break;
        }
            
        case DIALOG_SAVE_COPY:
        {
            UIAlertView *a = [[[UIAlertView alloc] initWithTitle:@"Save a copy" message:@"Please enter a name for this level." delegate:g_SDL_viewcontroller cancelButtonTitle:@"Cancel" otherButtonTitles:@"Save", nil] autorelease];
            a.alertViewStyle = UIAlertViewStylePlainTextInput;
            
            [a textFieldAtIndex:0].text = [NSString stringWithUTF8String:ui_cb_get_level_title()];
            
            a.tag = num;
            P_focus(0);
            [a show];
            break;
        }
            
        case DIALOG_MULTIEMITTER:
        {
            import_dialog_multiemit = 1;
            ImportController *f = [[ImportController alloc] initWithNibName:@"ImportController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_OPEN_OBJECT:
        {
            import_dialog_multiemit = 0;
            ImportController *f = [[ImportController alloc] initWithNibName:@"ImportController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_EXPORT:
        {
            UIAlertView *a = [[[UIAlertView alloc] initWithTitle:@"Export object" message:@"Please enter a name for this object." delegate:g_SDL_viewcontroller cancelButtonTitle:@"Cancel" otherButtonTitles:@"Export", nil] autorelease];
            a.alertViewStyle = UIAlertViewStylePlainTextInput;

            [a textFieldAtIndex:0].text = @"";

            a.tag = num;
            P_focus(0);
            [a show];
            break;
        }
            
        case DIALOG_QUICKADD:
        {
            ios_quickadd_clear();
            QuickaddController *f = [[QuickaddController alloc] initWithNibName:@"QuickaddController" bundle:nil];
            f.modalPresentationStyle = UIModalPresentationFormSheet;
            [g_SDL_viewcontroller presentModalViewController:f animated:YES];
        }
            break;
            
        case DIALOG_SAVE:
        {
            const char *name = ui_cb_get_level_title();
            
            if (!strlen(name)) {
            
                UIAlertView *a = [[[UIAlertView alloc] initWithTitle:@"Save level" message:@"Please enter a name for this level." delegate:g_SDL_viewcontroller cancelButtonTitle:@"Cancel" otherButtonTitles:@"Save",  nil] autorelease];
                a.alertViewStyle = UIAlertViewStylePlainTextInput;
            
                //[a textFieldAtIndex:0].text = [NSString stringWithUTF8String:ui_cb_get_level_title()];
            
                a.tag = num;
                P_focus(0);
                [a show];
            } else
                P_add_action(ACTION_SAVE, 0);
            
            break;
        }
            
        case DIALOG_NEW_LEVEL:
        {
            UIAlertView *a = [[[UIAlertView alloc] initWithTitle:@"Create Level" message:@"Please choose a level type.\nCustom - Build anything you want.\nAdventure - Player controls the robot.\nPuzzle - Player will move and build with your objects to complete your level." delegate:g_SDL_viewcontroller cancelButtonTitle:@"Cancel" otherButtonTitles:@"Custom", @"Adventure", @"Puzzle", nil] autorelease];
            
            a.tag = num;
            P_focus(0);
            [a show];
        }
            break;
            
        case DIALOG_MAIN_MENU_PKG:
        {
            UIAlertView *a = [[[UIAlertView alloc] initWithTitle:@"Select a Level Package" message:nil delegate:g_SDL_viewcontroller cancelButtonTitle:@"Close" otherButtonTitles:@"Main Puzzles", @"Adventure Introduction", nil] autorelease];
            
            a.tag = num;
            P_focus(0);
            [a show];
        }
            break;
            
        case DIALOG_SANDBOX_MODE:
        {
            UIAlertView *a = [[[UIAlertView alloc] initWithTitle:@"Sandbox Mode" message:nil delegate:g_SDL_viewcontroller cancelButtonTitle:@"Close" otherButtonTitles:@"Connection Edit", @"Multi-Select", @"Terrain Paint", nil] autorelease];
            
            a.tag = num;
            P_focus(0);
            [a show];
        }
            break;
            
        case DIALOG_SET_COMMAND:
        {
            TSAlertView *a = [[[TSAlertView alloc] init] autorelease];
            
            a.title = @"Command Pad";
            a.message = @"Please specify which command this command pad will send to a robot that steps on it.";
            a.style = TSAlertViewStyleNormal;
            a.delegate = g_SDL_viewcontroller;
            a.pickerinitial_0 = ui_cb_get_command();
            a.pickervalues_0 = @[
                               @"Stop",
                               @"Start/Stop toggle",
                               @"Left",
                               @"Right",
                               @"Left/Right toggle",
                               @"Jump",
                               @"Aim",
                               @"Attack",
                               @"Layer up",
                               @"Layer down",
                               @"Increase speed",
                               @"Decrease speed",
                               @"Set speed",
                               @"Full Health"
                               ];
            a.tag = num;
            
            [a addButtonWithTitle:@"Done"];
            P_focus(0);
            [a show];
        }
            break;
            
        case DIALOG_CONSUMABLE:
        {
            TSAlertView *a = [[[TSAlertView alloc] init] autorelease];
            
            a.title = @"Consumable";
            //a.message = @"Please specify which command this command pad will send to a robot that steps on it.";
            a.style = TSAlertViewStyleNormal;
            a.delegate = g_SDL_viewcontroller;
            a.pickerinitial_0 = ui_get_property_uint32(0);
            a.pickervalues_0 = @[
                                 @"Arm Cannon",
                                 @"Builder",
                                 @"Shotgun",
                                 @"Railgun",
                                 @"Oil Barrel",
                                 @"Speed Oil",
                                 @"Jump Oil",
                                 @"Armour Oil",
                                 @"Miner",
                                 @"Miner Upgrade"
                                 ];
            a.tag = num;
            
            [a addButtonWithTitle:@"Done"];
            P_focus(0);
            [a show];
        }
            break;
    }
}

static CommunityController *_cf = nil;

void ui_open_url(const char *url){
    //[[UIApplication sharedApplication] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]];
    
    if (!_cf) {
        _cf = [[CommunityController alloc] initWithNibName:@"CommunityController" bundle:nil];
        _cf.modalPresentationStyle = UIModalPresentationFullScreen;
    }
    if (url == 0)
        community_next_url = 0;
    else
        community_next_url = strdup(url);
    [g_SDL_viewcontroller presentModalViewController:_cf animated:YES];
    
};
void ui_open_help_dialog(const char *title, const char *description)
{
    InfoController *f = [[InfoController alloc] initWithNibName:@"InfoController" bundle:nil];
    f.modalPresentationStyle = UIModalPresentationFormSheet;
    
    f.txt_descr = [[NSString alloc] initWithUTF8String:description];
    
    [g_SDL_viewcontroller presentModalViewController:f animated:YES];
};
void ui_open_error_dialog(const char *error_msg)
{
    InfoController *f = [[InfoController alloc] initWithNibName:@"InfoController" bundle:nil];
    f.modalPresentationStyle = UIModalPresentationFormSheet;
    
    f.txt_descr = [[NSString alloc] initWithUTF8String:error_msg];
    
    [g_SDL_viewcontroller presentModalViewController:f animated:YES];
};

void ui_emit_signal(int num){

    switch (num) {
        case SIGNAL_LOGIN_SUCCESS:
            P_add_action(ACTION_PUBLISH, 0);
            break;
            
        case SIGNAL_REGISTER_SUCCESS:
            [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                if (register_dialog != 0)
                    [register_dialog dismiss_with_success];
            }];
            break;
            
        case SIGNAL_REGISTER_FAILED:
        {
            [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                if (register_dialog != 0)
                    [register_dialog enable_inputs];
            }];
            break;
        }
    }
};

#define CURRENT_TOAST_TAG 6984678

static const CGFloat kComponentPadding = 5;

static iToastSettings *sharedSettings = nil;

@interface iToast(private)

- (iToast *) settings;
- (CGRect)_toastFrameForImageSize:(CGSize)imageSize withLocation:(iToastImageLocation)location andTextSize:(CGSize)textSize;
- (CGRect)_frameForImage:(iToastType)type inToastFrame:(CGRect)toastFrame;

@end


@implementation iToast


- (id) initWithText:(NSString *) tex{
	if (self = [super init]) {
		text = [tex copy];
	}
	
	return self;
}

- (void) show{
	[self show:iToastTypeNone];
}

- (void) show:(iToastType) type {
	
	iToastSettings *theSettings = _settings;
	
	if (!theSettings) {
		theSettings = [iToastSettings getSharedSettings];
	}
	
	UIImage *image = [theSettings.images valueForKey:[NSString stringWithFormat:@"%i", type]];
	
	UIFont *font = [UIFont systemFontOfSize:theSettings.fontSize];
	CGSize textSize = [text sizeWithFont:font constrainedToSize:CGSizeMake(280, 60)];
	
	UILabel *label = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, textSize.width + kComponentPadding, textSize.height + kComponentPadding)];
	label.backgroundColor = [UIColor clearColor];
	label.textColor = [UIColor whiteColor];
	label.font = font;
	label.text = text;
	label.numberOfLines = 0;
	if (theSettings.useShadow) {
		label.shadowColor = [UIColor darkGrayColor];
		label.shadowOffset = CGSizeMake(1, 1);
	}
	
	UIButton *v = [UIButton buttonWithType:UIButtonTypeCustom];
	if (image) {
		v.frame = [self _toastFrameForImageSize:image.size withLocation:[theSettings imageLocation] andTextSize:textSize];
        
        switch ([theSettings imageLocation]) {
            case iToastImageLocationLeft:
                [label setTextAlignment:UITextAlignmentLeft];
                label.center = CGPointMake(image.size.width + kComponentPadding * 2
                                           + (v.frame.size.width - image.size.width - kComponentPadding * 2) / 2,
                                           v.frame.size.height / 2);
                break;
            case iToastImageLocationTop:
                [label setTextAlignment:UITextAlignmentCenter];
                label.center = CGPointMake(v.frame.size.width / 2,
                                           (image.size.height + kComponentPadding * 2
                                            + (v.frame.size.height - image.size.height - kComponentPadding * 2) / 2));
                break;
            default:
                break;
        }
		
	} else {
		v.frame = CGRectMake(0, 0, textSize.width + kComponentPadding * 2, textSize.height + kComponentPadding * 2);
		label.center = CGPointMake(v.frame.size.width / 2, v.frame.size.height / 2);
	}
	CGRect lbfrm = label.frame;
	lbfrm.origin.x = ceil(lbfrm.origin.x);
	lbfrm.origin.y = ceil(lbfrm.origin.y);
	label.frame = lbfrm;
	[v addSubview:label];
	[label release];
	
	if (image) {
		UIImageView *imageView = [[UIImageView alloc] initWithImage:image];
		imageView.frame = [self _frameForImage:type inToastFrame:v.frame];
		[v addSubview:imageView];
		[imageView release];
	}
	
	v.backgroundColor = [UIColor colorWithRed:theSettings.bgRed green:theSettings.bgGreen blue:theSettings.bgBlue alpha:theSettings.bgAlpha];
	v.layer.cornerRadius = theSettings.cornerRadius;
	
	UIWindow *window = [[[UIApplication sharedApplication] windows] objectAtIndex:0];
	
	CGPoint point;
	
	// Set correct orientation/location regarding device orientation
	UIInterfaceOrientation orientation = (UIInterfaceOrientation)[[UIApplication sharedApplication] statusBarOrientation];
	switch (orientation) {
		case UIDeviceOrientationPortrait:
		{
			if (theSettings.gravity == iToastGravityTop) {
				point = CGPointMake(window.frame.size.width / 2, 45);
			} else if (theSettings.gravity == iToastGravityBottom) {
				point = CGPointMake(window.frame.size.width / 2, window.frame.size.height - 45);
			} else if (theSettings.gravity == iToastGravityCenter) {
				point = CGPointMake(window.frame.size.width/2, window.frame.size.height/2);
			} else {
				point = theSettings.postition;
			}
			
			point = CGPointMake(point.x + theSettings.offsetLeft, point.y + theSettings.offsetTop);
			break;
		}
		case UIDeviceOrientationPortraitUpsideDown:
		{
			v.transform = CGAffineTransformMakeRotation(M_PI);
			
			float width = window.frame.size.width;
			float height = window.frame.size.height;
			
			if (theSettings.gravity == iToastGravityTop) {
				point = CGPointMake(width / 2, height - 45);
			} else if (theSettings.gravity == iToastGravityBottom) {
				point = CGPointMake(width / 2, 45);
			} else if (theSettings.gravity == iToastGravityCenter) {
				point = CGPointMake(width/2, height/2);
			} else {
				// TODO : handle this case
				point = theSettings.postition;
			}
			
			point = CGPointMake(point.x - theSettings.offsetLeft, point.y - theSettings.offsetTop);
			break;
		}
		case UIDeviceOrientationLandscapeLeft:
		{
			v.transform = CGAffineTransformMakeRotation(M_PI/2); //rotation in radians
			
			if (theSettings.gravity == iToastGravityTop) {
				point = CGPointMake(window.frame.size.width - 45, window.frame.size.height / 2);
			} else if (theSettings.gravity == iToastGravityBottom) {
				point = CGPointMake(45,window.frame.size.height / 2);
			} else if (theSettings.gravity == iToastGravityCenter) {
				point = CGPointMake(window.frame.size.width/2, window.frame.size.height/2);
			} else {
				// TODO : handle this case
				point = theSettings.postition;
			}
			
			point = CGPointMake(point.x - theSettings.offsetTop, point.y - theSettings.offsetLeft);
			break;
		}
		case UIDeviceOrientationLandscapeRight:
		{
			v.transform = CGAffineTransformMakeRotation(-M_PI/2);
			
			if (theSettings.gravity == iToastGravityTop) {
				point = CGPointMake(45, window.frame.size.height / 2);
			} else if (theSettings.gravity == iToastGravityBottom) {
				point = CGPointMake(window.frame.size.width - 45, window.frame.size.height/2);
			} else if (theSettings.gravity == iToastGravityCenter) {
				point = CGPointMake(window.frame.size.width/2, window.frame.size.height/2);
			} else {
				// TODO : handle this case
				point = theSettings.postition;
			}
			
			point = CGPointMake(point.x + theSettings.offsetTop, point.y + theSettings.offsetLeft);
			break;
		}
		default:
			break;
	}
    
	v.center = point;
	v.frame = CGRectIntegral(v.frame);
	
	NSTimer *timer1 = [NSTimer timerWithTimeInterval:((float)theSettings.duration)/1000
                                              target:self selector:@selector(hideToast:)
                                            userInfo:nil repeats:NO];
	[[NSRunLoop mainRunLoop] addTimer:timer1 forMode:NSDefaultRunLoopMode];
	
	v.tag = CURRENT_TOAST_TAG;
    
	UIView *currentToast = [window viewWithTag:CURRENT_TOAST_TAG];
	if (currentToast != nil) {
    	[currentToast removeFromSuperview];
	}
    
	v.alpha = 0;
	[window addSubview:v];
	[UIView beginAnimations:nil context:nil];
	v.alpha = 1;
	[UIView commitAnimations];
	
	view = [v retain];
	
	[v addTarget:self action:@selector(hideToast:) forControlEvents:UIControlEventTouchDown];
}

- (CGRect)_toastFrameForImageSize:(CGSize)imageSize withLocation:(iToastImageLocation)location andTextSize:(CGSize)textSize {
    CGRect theRect = CGRectZero;
    switch (location) {
        case iToastImageLocationLeft:
            theRect = CGRectMake(0, 0,
                                 imageSize.width + textSize.width + kComponentPadding * 3,
                                 MAX(textSize.height, imageSize.height) + kComponentPadding * 2);
            break;
        case iToastImageLocationTop:
            theRect = CGRectMake(0, 0,
                                 MAX(textSize.width, imageSize.width) + kComponentPadding * 2,
                                 imageSize.height + textSize.height + kComponentPadding * 3);
            
        default:
            break;
    }
    return theRect;
}

- (CGRect)_frameForImage:(iToastType)type inToastFrame:(CGRect)toastFrame {
    iToastSettings *theSettings = _settings;
    UIImage *image = [theSettings.images valueForKey:[NSString stringWithFormat:@"%i", type]];
    
    if (!image) return CGRectZero;
    
    CGRect imageFrame = CGRectZero;
    
    switch ([theSettings imageLocation]) {
        case iToastImageLocationLeft:
            imageFrame = CGRectMake(kComponentPadding, (toastFrame.size.height - image.size.height) / 2, image.size.width, image.size.height);
            break;
        case iToastImageLocationTop:
            imageFrame = CGRectMake((toastFrame.size.width - image.size.width) / 2, kComponentPadding, image.size.width, image.size.height);
            break;
            
        default:
            break;
    }
    
    return imageFrame;
    
}

- (void) hideToast:(NSTimer*)theTimer{
	[UIView beginAnimations:nil context:NULL];
	view.alpha = 0;
	[UIView commitAnimations];
	
	NSTimer *timer2 = [NSTimer timerWithTimeInterval:500
                                              target:self selector:@selector(hideToast:)
                                            userInfo:nil repeats:NO];
	[[NSRunLoop mainRunLoop] addTimer:timer2 forMode:NSDefaultRunLoopMode];
}

- (void) removeToast:(NSTimer*)theTimer{
	[view removeFromSuperview];
}


+ (iToast *) makeText:(NSString *) _text{
	iToast *toast = [[[iToast alloc] initWithText:_text] autorelease];
	
	return toast;
}


- (iToast *) setDuration:(NSInteger ) duration{
	[self theSettings].duration = duration;
	return self;
}

- (iToast *) setGravity:(iToastGravity) gravity
			 offsetLeft:(NSInteger) left
			  offsetTop:(NSInteger) top{
	[self theSettings].gravity = gravity;
	[self theSettings].offsetLeft = left;
	[self theSettings].offsetTop = top;
	return self;
}

- (iToast *) setGravity:(iToastGravity) gravity{
	[self theSettings].gravity = gravity;
	return self;
}

- (iToast *) setPostion:(CGPoint) _position{
	[self theSettings].postition = CGPointMake(_position.x, _position.y);
	
	return self;
}

- (iToast *) setFontSize:(CGFloat) fontSize{
	[self theSettings].fontSize = fontSize;
	return self;
}

- (iToast *) setUseShadow:(BOOL) useShadow{
	[self theSettings].useShadow = useShadow;
	return self;
}

- (iToast *) setCornerRadius:(CGFloat) cornerRadius{
	[self theSettings].cornerRadius = cornerRadius;
	return self;
}

- (iToast *) setBgRed:(CGFloat) bgRed{
	[self theSettings].bgRed = bgRed;
	return self;
}

- (iToast *) setBgGreen:(CGFloat) bgGreen{
	[self theSettings].bgGreen = bgGreen;
	return self;
}

- (iToast *) setBgBlue:(CGFloat) bgBlue{
	[self theSettings].bgBlue = bgBlue;
	return self;
}

- (iToast *) setBgAlpha:(CGFloat) bgAlpha{
	[self theSettings].bgAlpha = bgAlpha;
	return self;
}


-(iToastSettings *) theSettings{
	if (!_settings) {
		_settings = [[iToastSettings getSharedSettings] copy];
	}
	
	return _settings;
}

@end


@implementation iToastSettings
@synthesize duration;
@synthesize gravity;
@synthesize postition;
@synthesize fontSize;
@synthesize useShadow;
@synthesize cornerRadius;
@synthesize bgRed;
@synthesize bgGreen;
@synthesize bgBlue;
@synthesize bgAlpha;
@synthesize images;
@synthesize imageLocation;

- (void) setImage:(UIImage *) img withLocation:(iToastImageLocation)location forType:(iToastType) type {
	if (type == iToastTypeNone) {
		// This should not be used, internal use only (to force no image)
		return;
	}
	
	if (!images) {
		images = [[NSMutableDictionary alloc] initWithCapacity:4];
	}
	
	if (img) {
		NSString *key = [NSString stringWithFormat:@"%i", type];
		[images setValue:img forKey:key];
	}
    
    [self setImageLocation:location];
}

- (void)setImage:(UIImage *)img forType:(iToastType)type {
    [self setImage:img withLocation:iToastImageLocationLeft forType:type];
}


+ (iToastSettings *) getSharedSettings{
	if (!sharedSettings) {
		sharedSettings = [iToastSettings new];
		sharedSettings.gravity = iToastGravityCenter;
		sharedSettings.duration = iToastDurationShort;
		sharedSettings.fontSize = 16.0;
		sharedSettings.useShadow = YES;
		sharedSettings.cornerRadius = 5.0;
		sharedSettings.bgRed = 0;
		sharedSettings.bgGreen = 0;
		sharedSettings.bgBlue = 0;
		sharedSettings.bgAlpha = 0.7;
		sharedSettings.offsetLeft = 0;
		sharedSettings.offsetTop = 0;
	}
	
	return sharedSettings;
	
}

- (id) copyWithZone:(NSZone *)zone{
	iToastSettings *copy = [iToastSettings new];
	copy.gravity = self.gravity;
	copy.duration = self.duration;
	copy.postition = self.postition;
	copy.fontSize = self.fontSize;
	copy.useShadow = self.useShadow;
	copy.cornerRadius = self.cornerRadius;
	copy.bgRed = self.bgRed;
	copy.bgGreen = self.bgGreen;
	copy.bgBlue = self.bgBlue;
	copy.bgAlpha = self.bgAlpha;
	copy.offsetLeft = self.offsetLeft;
	copy.offsetTop = self.offsetTop;
	
	NSArray *keys = [self.images allKeys];
	
	for (NSString *key in keys){
		[copy setImage:[images valueForKey:key] forType:[key intValue]];
	}
    
    [copy setImageLocation:imageLocation];
	
	return copy;
}

@end