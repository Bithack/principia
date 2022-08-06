//
//  SettingsController.m
//  principia
//
//  Created by Emil on 2013-12-12.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "SettingsController.h"
#include "ui.hh"
#include "main.hh"
#include "SDL.h"

@interface SettingsController ()

@end

@implementation SettingsController

static const char *resolutions[] = {
    "256x256","512x256","512x512","1024x512","1024x1024","2048x1024","2048x2048"
};
#define NUM_RESOLUTIONS (sizeof(resolutions)/sizeof(char*))

static const int res_x[] = {256,512,512,1024,1024,2048,2048};
static const int res_y[] = {256,256,512,512,1024,1024,2048};

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
    
    UIScrollView *scroll = (UIScrollView*)[self.view viewWithTag:1000];
    
    [scroll setContentSize:CGSizeMake(480, 638)];
    
    int sresx = ui_settings_get_shadow_resx();
    int sresy = ui_settings_get_shadow_resy();
    int enable_shadows = ui_settings_get_enable_shadows();
    int shadow_quality = ui_settings_get_shadow_quality();
    int ao_quality = ui_settings_get_ao_quality();
    int enable_ao = ui_settings_get_enable_ao();
    int texture_quality = ui_settings_get_texture_quality();
    float ui_scale = ui_settings_get_ui_scale();
    float cam_speed = ui_settings_get_cam_speed();
    float zoom_speed = ui_settings_get_zoom_speed();
    int enable_smooth_cam = ui_settings_get_enable_smooth_cam();
    int enable_smooth_zoom = ui_settings_get_enable_smooth_zoom();
    int enable_border_scrolling = ui_settings_get_enable_border_scrolling();
    int enable_object_ids = ui_settings_get_enable_object_ids();
    float border_scroll_speed = ui_settings_get_border_scrolling_speed();
    
    UISegmentedControl *seg;
    UISlider *sl;
    UISwitch *s = (UISwitch*)[self.view viewWithTag:100];
    UILabel *sll = (UILabel*)[self.view viewWithTag:104];
    [s setOn:enable_shadows];

    s = (UISwitch*)[self.view viewWithTag:101];
    [s setOn:enable_smooth_cam];
    
    s = (UISwitch*)[self.view viewWithTag:102];
    [s setOn:enable_smooth_zoom];
    
    char res[512];
    int x;
    for (x=0; x<NUM_RESOLUTIONS; x++) {
        sprintf(res, "%ux%u", sresx, sresy);
        if (strcmp(res, resolutions[x]) == 0) {
            break;
        }
    }
    if (x == NUM_RESOLUTIONS) x = 3;
    
    sl = (UISlider*)[self.view viewWithTag:103];
    [sl setValue:(1.f/NUM_RESOLUTIONS)*x];
    [sll setText:[NSString stringWithUTF8String:resolutions[x]]];
    
    sl = (UISlider*)[self.view viewWithTag:105];
    [sl setValue:cam_speed];
    
    sl = (UISlider*)[self.view viewWithTag:106];
    [sl setValue:zoom_speed];
    
    sl = (UISlider*)[self.view viewWithTag:107];
    [sl setValue:ui_scale];
    
    seg = (UISegmentedControl*)[self.view viewWithTag:108];
    [seg setSelectedSegmentIndex:shadow_quality];
    
    seg = (UISegmentedControl*)[self.view viewWithTag:109];
    [seg setSelectedSegmentIndex:enable_ao ? (ao_quality == 512 ? 3 : (ao_quality == 256 ? 2 : 1)) : 0];
    
    seg = (UISegmentedControl*)[self.view viewWithTag:110];
    [seg setSelectedSegmentIndex:texture_quality == 0 ? 0 : 1];
    
    s = (UISwitch*)[self.view viewWithTag:150];
    [s setOn:enable_border_scrolling];
    
    sl = (UISlider*)[self.view viewWithTag:151];
    [sl setValue:border_scroll_speed];
    
    s = (UISwitch*)[self.view viewWithTag:152];
    [s setOn:enable_object_ids];
}


- (IBAction)shadow_res_change:(id)sender
{
    UISlider *sl = (UISlider*)[self.view viewWithTag:103];
    int res = (int)roundf(((float)NUM_RESOLUTIONS) * [sl value] );
    if (res < 0) res = 0;
    if (res >= NUM_RESOLUTIONS) res = NUM_RESOLUTIONS-1;
    
    UILabel *sll = (UILabel*)[self.view viewWithTag:104];
    [sl setValue:res * (1.f / (float)NUM_RESOLUTIONS)];
    [sll setText:[NSString stringWithUTF8String:resolutions[res]]];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)cancel_click:(id)sender
{
    [self dismissModalViewControllerAnimated:YES];
}

- (IBAction)done_click:(id)sender
{
    UISwitch *s;
    UISlider *sl = (UISlider*)[self.view viewWithTag:103];
    UISegmentedControl *seg;
    int res = (int)roundf(((float)NUM_RESOLUTIONS) * [sl value] );
    if (res < 0) res = 0;
    if (res >= NUM_RESOLUTIONS) res = NUM_RESOLUTIONS-1;

    
    int sresx = res_x[res];
    int sresy = res_y[res];
    int do_reload_graphics = 0;
    
    s = (UISwitch*)[self.view viewWithTag:100];
    int enable_shadows = [s isOn];
    
    s = (UISwitch*)[self.view viewWithTag:101];
    int smooth_cam = [s isOn];
    
    s = (UISwitch*)[self.view viewWithTag:102];
    int smooth_zoom = [s isOn];
    
    s = (UISwitch*)[self.view viewWithTag:150];
    int enable_border_scrolling = [s isOn];
    
    s = (UISwitch*)[self.view viewWithTag:152];
    int enable_object_ids = [s isOn];
    
    seg = (UISegmentedControl*)[self.view viewWithTag:108];
    int shadow_quality = [seg selectedSegmentIndex];
    
    seg = (UISegmentedControl*)[self.view viewWithTag:109];
    int val = [seg selectedSegmentIndex];
    int enable_ao = (val != 0);
    int ao_quality = (val == 1 ? 128 : (val == 2 ?  256 : (val == 3 ? 512 : 128)));
    
    seg = (UISegmentedControl*)[self.view viewWithTag:110];
    int texture_quality = [seg selectedSegmentIndex] == 0 ? 0 : 2;
    
    sl = (UISlider*)[self.view viewWithTag:105];
    float cam_speed = [sl value];
    
    sl = (UISlider*)[self.view viewWithTag:106];
    float zoom_speed = [sl value];
    
    sl = (UISlider*)[self.view viewWithTag:107];
    float uiscale = [sl value];
    
    sl = (UISlider*)[self.view viewWithTag:151];
    float border_scrolling_speed = [sl value];
    
    int need_restart = 0;
    if (fabsf(uiscale  - ui_settings_get_ui_scale()) > .01f) {
        need_restart = 1;
    }
    
    if (sresx != ui_settings_get_shadow_resx()) do_reload_graphics = 1;
    if (sresy != ui_settings_get_shadow_resy()) do_reload_graphics = 1;
    if (enable_shadows != ui_settings_get_enable_shadows()) do_reload_graphics = 1;
    if (shadow_quality != ui_settings_get_shadow_quality()) do_reload_graphics = 1;
    if (ao_quality != ui_settings_get_ao_quality()) do_reload_graphics = 1;
    if (enable_ao != ui_settings_get_enable_ao()) do_reload_graphics = 1;
    if (texture_quality != ui_settings_get_texture_quality()) do_reload_graphics = 1;
    
    if (do_reload_graphics) {
        P_set_can_reload_graphics(false);
        P_set_can_set_settings(false);
        P_add_action(ACTION_RELOAD_GRAPHICS, 0);

        while (P_get_can_set_settings()) {
            SDL_Delay(5);
        }
    }
    
    ui_settings_set_shadow_resx(sresx);
    ui_settings_set_shadow_resy(sresy);
    ui_settings_set_enable_shadows(enable_shadows);
    ui_settings_set_shadow_quality(shadow_quality);
    ui_settings_set_ao_quality(ao_quality);
    ui_settings_set_enable_ao(enable_ao);
    ui_settings_set_texture_quality(texture_quality);
    ui_settings_set_ui_scale(uiscale);
    ui_settings_set_cam_speed(cam_speed);
    ui_settings_set_zoom_speed(zoom_speed);
    ui_settings_set_enable_smooth_cam(smooth_cam);
    ui_settings_set_enable_smooth_zoom(smooth_zoom);
    ui_settings_set_enable_border_scrolling(enable_border_scrolling);
    ui_settings_set_border_scrolling_speed(border_scrolling_speed);
    ui_settings_set_enable_object_ids(enable_object_ids);

    if (do_reload_graphics) {
        P_set_can_reload_graphics(true);
    }
    
    ui_settings_save();
    
    
    
    [self dismissModalViewControllerAnimated:YES];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}
@end
