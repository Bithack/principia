//
//  ShapeExtruderController.m
//  principia
//
//  Created by Emil on 2014-02-12.
//  Copyright (c) 2014 Bithack AB. All rights reserved.
//

#import "ShapeExtruderController.h"
#include "ui.hh"

@interface ShapeExtruderController ()

@end

@implementation ShapeExtruderController

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
    
    for (int x=0; x<4; x++) {
        UISlider *s = (UISlider*)[self.view viewWithTag:100+x];
        [s setValue:ui_get_property_float(x)];
    }
    
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
    for (int x=0; x<4; x++) {
        UISlider *s = (UISlider*)[self.view viewWithTag:100+x];
        UILabel *l = (UILabel*)[self.view viewWithTag:200+x];
        
        [l setText:[NSString stringWithFormat:@"%.2f", [s value]]];
    }
}

- (IBAction)done_click:(id)sender
{
    for (int x=0; x<4; x++) {
        UISlider *s = (UISlider*)[self.view viewWithTag:100+x];
        ui_set_property_float(x, [s value]);
    }
    [self dismissModalViewControllerAnimated:YES];
}

- (void)viewWillLayoutSubviews{
    [super viewWillLayoutSubviews];
    
    self.view.superview.bounds = CGRectMake(0,0,480,320);
}

@end
