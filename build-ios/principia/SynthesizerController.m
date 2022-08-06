//
//  SynthesizerController.m
//  principia
//
//  Created by Emil on 2013-12-11.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "SynthesizerController.h"
#include "ui.hh"

@interface SynthesizerController ()

@end

@implementation SynthesizerController

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

    UITextField *bf = (UITextField*)[self.view viewWithTag:101];
    UITextField *bp = (UITextField*)[self.view viewWithTag:102];
    UISegmentedControl *wave = (UISegmentedControl*)[self.view viewWithTag:100];
    
    [bf setText:[NSString stringWithFormat:@"%.2f", ui_get_property_float(0)]];
    [bp setText:[NSString stringWithFormat:@"%.2f", ui_get_property_float(1)]];
    [wave setSelectedSegmentIndex:ui_get_property_uint32(2)];
    
    for (int x=0; x<6; x++) {
        UISlider *s = (UISlider*)[self.view viewWithTag:x+103];
        [s setValue:ui_get_property_float(x+3)];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


- (void)viewWillLayoutSubviews{
    [super viewWillLayoutSubviews];
    
    self.view.superview.bounds = CGRectMake(0,0,480,320);
}


- (IBAction)done_click:(id)sender
{
    UITextField *bf = (UITextField*)[self.view viewWithTag:101];
    UITextField *bp = (UITextField*)[self.view viewWithTag:102];
    UISegmentedControl *wave = (UISegmentedControl*)[self.view viewWithTag:100];
    
    float base = [[bf text] doubleValue];
    float peak = [[bp text] doubleValue];
    
    if (base < 0.f) base = 0.f;
    else if (base > 3520.f) base = 3520.f;
    
    
    if (peak < base) peak = base;
    else if (peak > 3520.f) peak = 3520.f;
    
    ui_set_property_float(0, base);
    ui_set_property_float(1, peak);
    ui_set_property_uint32(2, [wave selectedSegmentIndex]);
    
    for (int x=0; x<6; x++) {
        UISlider *s = (UISlider*)[self.view viewWithTag:x+103];
        ui_set_property_float(x+3, [s value]);
    }
    
    [self dismissModalViewControllerAnimated:YES];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}


@end
