//
//  ColorController.m
//  principia
//
//  Created by Emil on 2013-12-11.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "ColorController.h"
#include "ui.hh"
#include "main.hh"

@interface ColorController ()

@end

@implementation ColorController

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
    UISlider *s;
    
    s = (UISlider*)[self.view viewWithTag:100];
    [s setValue:ui_get_property_float(1)];
    s = (UISlider*)[self.view viewWithTag:101];
    [s setValue:ui_get_property_float(2)];
    s = (UISlider*)[self.view viewWithTag:102];
    [s setValue:ui_get_property_float(3)];
    s = (UISlider*)[self.view viewWithTag:103];
    
    if (ui_get_entity_gid() == 122) {
        [s setValue:(float)ui_get_property_uint8(4)/255.f];
        [s setEnabled:YES];
    } else {
        [s setValue:0];
        [s setEnabled:NO];
    }
    
    [self update_color];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)done_click:(id)sender
{
    UISlider *s;
    
    s = (UISlider*)[self.view viewWithTag:100];
    float red = [s value];
    s = (UISlider*)[self.view viewWithTag:101];
    float green = [s value];
    s = (UISlider*)[self.view viewWithTag:102];
    float blue = [s value];
    s = (UISlider*)[self.view viewWithTag:103];
    float alpha = [s value];
    
    ui_cb_set_color(red,green,blue,alpha);
    
    [self dismissModalViewControllerAnimated:YES];
}

- (IBAction)slider_change:(id)sender
{
    [self update_color];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}

- (void)update_color
{
    UIView *v = [self.view viewWithTag:300];
    UISlider *s;
    UITextField *f;
    
    s = (UISlider*)[self.view viewWithTag:100];
    float red = [s value];
    s = (UISlider*)[self.view viewWithTag:101];
    float green = [s value];
    s = (UISlider*)[self.view viewWithTag:102];
    float blue = [s value];
 //   s = (UISlider*)[self.view viewWithTag:103];
   // float alpha = [s value];
    
    for (int x=0; x<4; x++) {
        s = (UISlider*)[self.view viewWithTag:100+x];
        f = (UITextField*)[self.view viewWithTag:200+x];
        
        [f setText:[NSString stringWithFormat:@"%.0f", [s value]*255]];
    }
    
    [v setBackgroundColor:[UIColor colorWithRed:red green:green blue:blue alpha:1.f]];
}

- (void)viewWillLayoutSubviews{
    [super viewWillLayoutSubviews];
    
    self.view.superview.bounds = CGRectMake(0,0,480,320);
}

@end
