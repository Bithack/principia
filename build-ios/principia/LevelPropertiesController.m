//
//  LevelPropertiesController.m
//  principia
//
//  Created by Emil on 2013-10-24.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "LevelPropertiesController.h"
#include "ui.hh"
#include "main.hh"

@interface LevelPropertiesController ()

@end

@implementation LevelPropertiesController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
        
        info = [[PropertiesInfoController alloc] initWithNibName:@"PropertiesInfoController" bundle:nil];
        world = [[PropertiesWorldController alloc] initWithNibName:@"PropertiesWorldController" bundle:nil];
        gameplay = [[PropertiesGameplayController alloc] initWithNibName:@"PropertiesGameplayController" bundle:nil];
        
        views = [[NSArray alloc] initWithObjects:
                       info.view,
                       world.view,
                       gameplay.view,
                       nil];
    }
    return self;
}


- (void)tabBar:(UITabBar *)tabBar didSelectItem:(UITabBarItem *)item
{
    [[scrollview subviews]
     makeObjectsPerformSelector:@selector(removeFromSuperview)];
    [scrollview addSubview:((UIView*)views[item.tag])];
    [scrollview setContentSize:((UIView*)views[item.tag]).frame.size];
}

char *nsstring_to_cstring(NSString *s)
{
    NSUInteger len = [s lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    NSUInteger tja;
    char *tmp =  malloc(len+1);
    NSRange r = NSMakeRange(0, [s length]);
    [s getBytes:tmp maxLength:len usedLength:&tja encoding:NSUTF8StringEncoding options:0 range:r remainingRange:NULL];
    tmp[len] = '\0';
    return tmp;
}


- (IBAction)cancel_click:(id)sender
{
    [self dismissModalViewControllerAnimated:YES];
}

- (IBAction)done_click:(id)sender
{
    /* info shit */
    UITextField *tf;
    UITextView *tv;
    UISegmentedControl *sc;
    char *tmp = 0;
    
    /* info screen */
    /* 1 title */
    /* 2 description */
    /* 3 type */
    tf = (UITextField*)[info.view viewWithTag:1];
    tmp = nsstring_to_cstring([tf text]);
    ui_cb_set_level_title(tmp);
    free (tmp);
    
    tv = (UITextView*)[info.view viewWithTag:2];
    tmp = nsstring_to_cstring([tv text]);
    ui_cb_set_level_description(tmp);
    free (tmp);
    
    sc = (UISegmentedControl*)[info.view viewWithTag:3];
    ui_cb_set_level_type([sc selectedSegmentIndex]);
    
    /* gameplay shit */
    tf = (UITextField*)[gameplay.view viewWithTag:100];
    ui_cb_set_level_final_score((uint32_t)[[tf text] intValue]);
    UISwitch *s = (UISwitch*)[gameplay.view viewWithTag:101];
    ui_cb_set_pause_on_win((int)[s isOn]);
    
    s = (UISwitch*)[gameplay.view viewWithTag:102];
    ui_cb_set_display_score((int)[s isOn]);
    
    for (int x=200; x<300; x++) {
        s = (UISwitch*)[gameplay.view viewWithTag:x];
        
        if (!s) break;
        
        ui_cb_set_level_flag((1ull << ((uint64_t)x-200)), (int)[s isOn]);
    }
    
    /* world */
    tf = (UITextField*)[world.view viewWithTag:1];
    ui_cb_set_level_border(2, (uint16_t)abs([[tf text] integerValue]));
    tf = (UITextField*)[world.view viewWithTag:2];
    ui_cb_set_level_border(0, (uint16_t)abs([[tf text] integerValue]));
    tf = (UITextField*)[world.view viewWithTag:3];
    ui_cb_set_level_border(1, (uint16_t)abs([[tf text] integerValue]));
    tf = (UITextField*)[world.view viewWithTag:4];
    ui_cb_set_level_border(3, (uint16_t)abs([[tf text] integerValue]));
    
    UIPickerView *pv = (UIPickerView*)[world.view viewWithTag:9];
    ui_cb_set_level_bg([pv selectedRowInComponent:0]);
    
    UISlider *sl = (UISlider*)[world.view viewWithTag:100];
    ui_cb_set_level_pos_iter((uint8_t)[sl value]);
    sl = (UISlider*)[world.view viewWithTag:101];
    ui_cb_set_level_vel_iter((uint8_t)[sl value]);
    sl = (UISlider*)[world.view viewWithTag:102];
    ui_cb_set_level_prism_tolerance((uint8_t)[sl value]);
    sl = (UISlider*)[world.view viewWithTag:103];
    ui_cb_set_level_pivot_tolerance((uint8_t)[sl value]);
    
    tf = (UITextField*)[world.view viewWithTag:5];
    ui_cb_set_level_gravity_x([[tf text] floatValue]);
    tf = (UITextField*)[world.view viewWithTag:6];
    ui_cb_set_level_gravity_y([[tf text] floatValue]);
    
    P_add_action(ACTION_RELOAD_LEVEL, 0);
    [self dismissModalViewControllerAnimated:YES];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    char *tmp;
    
    UITextField *tf;
    UITextView *tv;
    UISegmentedControl *sc;
    /* info screen */
    /* 1 title */
    /* 2 description */
    /* 3 type */
    tf = (UITextField*)[info.view viewWithTag:1];
    tmp = ui_cb_get_level_title();
    [tf setText:[[NSString alloc] initWithUTF8String:tmp]];
    free(tmp);
    
    tv = (UITextView*)[info.view viewWithTag:2];
    [tv setText:[[NSString alloc] initWithUTF8String:ui_cb_get_level_description()]];
    
    tv.layer.borderWidth = 1.0f;
    tv.layer.borderColor = [[UIColor blackColor] CGColor];
    
    sc= (UISegmentedControl*)[info.view viewWithTag:3];
    [sc setSelectedSegmentIndex:ui_cb_get_level_type()];
    
    /* 100: final score uitextfield */
    /* 101: pause on win uiswitch */
    /* 102: displayscore uiswitch */
    
    tf = (UITextField*)[gameplay.view viewWithTag:100];
    [tf setText:[NSString stringWithFormat:@"%d", ui_cb_get_level_final_score()]];

    UISwitch *s = (UISwitch*)[gameplay.view viewWithTag:101];
    [s setOn:(BOOL)ui_cb_get_pause_on_win() animated:NO];
    
    s = (UISwitch*)[gameplay.view viewWithTag:102];
    [s setOn:(BOOL)ui_cb_get_display_score() animated:NO];
    
    for (int x=200; x<300; x++) {
        s = (UISwitch*)[gameplay.view viewWithTag:x];
        
        if (!s) break;
        
        if (s != nil) {
            [s setOn:(BOOL)ui_cb_get_level_flag((1ull << ((uint64_t)x-200))) animated:NO];
        }
    }
    
    /* world */
    tf = (UITextField*)[world.view viewWithTag:1];
    [tf setText:[NSString stringWithFormat:@"%u", (uint32_t)ui_cb_get_level_border(2)]];
    tf = (UITextField*)[world.view viewWithTag:2];
    [tf setText:[NSString stringWithFormat:@"%u", (uint32_t)ui_cb_get_level_border(0)]];
    tf = (UITextField*)[world.view viewWithTag:3];
    [tf setText:[NSString stringWithFormat:@"%u", (uint32_t)ui_cb_get_level_border(1)]];
    tf = (UITextField*)[world.view viewWithTag:4];
    [tf setText:[NSString stringWithFormat:@"%u", (uint32_t)ui_cb_get_level_border(3)]];
    
    UIPickerView *pv = (UIPickerView*)[world.view viewWithTag:9];
    [pv selectRow:ui_cb_get_level_bg() inComponent:0 animated:NO];
    
    UISlider *sl = (UISlider*)[world.view viewWithTag:100];
    [sl setValue:(float)ui_cb_get_level_pos_iter()];
    sl = (UISlider*)[world.view viewWithTag:101];
    [sl setValue:(float)ui_cb_get_level_vel_iter()];
    sl = (UISlider*)[world.view viewWithTag:102];
    [sl setValue:(float)ui_cb_get_level_prism_tolerance()];
    sl = (UISlider*)[world.view viewWithTag:103];
    [sl setValue:(float)ui_cb_get_level_pivot_tolerance()];
    [((PropertiesWorldController*)world) slider_change:0];
    
    tf = (UITextField*)[world.view viewWithTag:5];
    [tf setText:[NSString stringWithFormat:@"%.2f", ui_cb_get_level_gravity_x()]];
    tf = (UITextField*)[world.view viewWithTag:6];
    [tf setText:[NSString stringWithFormat:@"%.2f", ui_cb_get_level_gravity_y()]];
    
    [[scrollview subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];
    [scrollview addSubview:((UIView*)views[0])];
    [scrollview setContentSize:((UIView*)views[0]).frame.size];
    
    [tabbar setSelectedItem:[tabbar.items objectAtIndex:0]];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}

@end
