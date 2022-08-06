//
//  RobotController.m
//  principia
//
//  Created by Emil on 2013-12-11.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "RobotController.h"
#include "ui.hh"
#include "main.hh"

@interface RobotController ()

@end

@implementation RobotController

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
    
    UISwitch *s = (UISwitch*)[self.view viewWithTag:100];
    UISegmentedControl *state = (UISegmentedControl*)[self.view viewWithTag:101];
    UISegmentedControl *dir = (UISegmentedControl*)[self.view viewWithTag:102];
    
    [s setOn:ui_get_property_uint8(2)];
    switch (ui_get_property_uint8(1)) {
        default:case 0:[state setSelectedSegmentIndex:0]; break;
        case 1:[state setSelectedSegmentIndex:1]; break;
        case 3:[state setSelectedSegmentIndex:2]; break;

    }
    switch (ui_get_property_uint8(4)) {
        default:case 0:[dir setSelectedSegmentIndex:1]; break;
        case 1:[dir setSelectedSegmentIndex:0]; break;
        case 2:[dir setSelectedSegmentIndex:2]; break;
            
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)done_click:(id)sender
{
    
    UISwitch *s = (UISwitch*)[self.view viewWithTag:100];
    UISegmentedControl *state = (UISegmentedControl*)[self.view viewWithTag:101];
    UISegmentedControl *dir = (UISegmentedControl*)[self.view viewWithTag:102];
    
    ui_set_property_uint8(2, [s isOn]);
    
    int state_v = [state selectedSegmentIndex];
    int dir_v = [dir selectedSegmentIndex];
    
    switch (state_v) {
        case 0: ui_set_property_uint8(1, /*ROBOT_IDLE*/0); break;
        case 1: ui_set_property_uint8(1, /*ROBOT_WALK*/1); break;
        case 2: ui_set_property_uint8(1, /*ROBOT_DEAD*/3); break;
    }
    
    ui_cb_set_robot_dir(dir_v);
    P_add_action(ACTION_HIGHLIGHT_SELECTED, 0);
    P_add_action(ACTION_RESELECT, 0);
    [self dismissModalViewControllerAnimated:YES];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}
- (void)viewWillLayoutSubviews{
    [super viewWillLayoutSubviews];
    
    self.view.superview.bounds = CGRectMake(0,0,480,320);
}
@end
