//
//  RubberController.m
//  principia
//
//  Created by Emil on 2014-02-12.
//  Copyright (c) 2014 Bithack AB. All rights reserved.
//

#import "RubberController.h"
#include "ui.hh"
#include "main.hh"

@interface RubberController ()

@end

@implementation RubberController

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
    
    UISlider *s = (UISlider*)[self.view viewWithTag:100];
    [s setValue:ui_get_property_float(1)];
    s = (UISlider*)[self.view viewWithTag:101];
    [s setValue:ui_get_property_float(2)];
    
    [self slider_change:0];
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

- (IBAction)slider_change:(id)sender
{
    for (int x=0; x<2; x++) {
        UISlider *s = (UISlider*)[self.view viewWithTag:100+x];
        UILabel *l = (UILabel*)[self.view viewWithTag:200+x];
        
        [l setText:[NSString stringWithFormat:@"%.2f", [s value]]];
    }
}

- (IBAction)done_click:(id)sender
{
    UISlider *s = (UISlider*)[self.view viewWithTag:100];
    ui_set_property_float(1, [s value]);
    s = (UISlider*)[self.view viewWithTag:101];
    ui_set_property_float(2, [s value]);
    ui_cb_update_rubber();
    [self dismissModalViewControllerAnimated:YES];
}

- (void)viewWillLayoutSubviews{
    [super viewWillLayoutSubviews];
    
    self.view.superview.bounds = CGRectMake(0,0,480,320);
}

@end
