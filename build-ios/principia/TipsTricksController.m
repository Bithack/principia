//
//  TipsTricksController.m
//  principia
//
//  Created by Emil on 2014-01-01.
//  Copyright (c) 2014 Bithack AB. All rights reserved.
//

#import "TipsTricksController.h"
#include "ui.hh"

@interface TipsTricksController ()

@end

@implementation TipsTricksController

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
    
    [self next_click:0];
    UISwitch *s = (UISwitch*)[self.view viewWithTag:101];
    [s setOn:ui_cb_get_hide_tips()?YES:NO];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


- (IBAction)more_click:(id)sender
{
    [self done_click:sender];
    ui_open_url("http://principiagame.com/help");
}

- (IBAction)next_click:(id)sender
{
    UITextView *v = (UITextView*)[self.view viewWithTag:100];
    [v setText:[NSString stringWithUTF8String:ui_cb_get_tip()]];
}

- (IBAction)done_click:(id)sender
{
    UISwitch *s = (UISwitch*)[self.view viewWithTag:101];
    
    ui_cb_set_hide_tips([s isOn]?1:0);
    
    [self dismissModalViewControllerAnimated:NO];
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
