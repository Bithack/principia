//
//  CursorfieldController.m
//  principia
//
//  Created by Emil on 2013-12-16.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "CursorfieldController.h"
#include "RangeSlider.h"
#include "ui.hh"
#include "main.hh"

@interface CursorfieldController ()

@end

@implementation CursorfieldController

static RangeSlider *rs0, *rs1;

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
    
    UIView *rs = [self.view viewWithTag:100];
    
    rs0 = [[RangeSlider alloc] initWithFrame:[rs bounds]];
    
    
    [rs0 setMinimumValue:-3.f];
    [rs0 setMaximumValue:3.f];
    [rs0 setMinimumRange:.25];
    [rs0 setSelectedMaximumValue:ui_get_property_float(0)];
    [rs0 setSelectedMinimumValue:ui_get_property_float(2)];
    
    [rs addSubview:rs0];
    
    rs = [self.view viewWithTag:101];
    rs1 = [[RangeSlider alloc] initWithFrame:[rs bounds]];
    [rs1 setMinimumValue:-3.f];
    [rs1 setMaximumValue:3.f];
    [rs1 setMinimumRange:.25];
    [rs1 setSelectedMaximumValue:ui_get_property_float(1)];
    [rs1 setSelectedMinimumValue:ui_get_property_float(3)];
    [rs addSubview:rs1];
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

- (IBAction)done_click:(id)sender
{
    ui_set_property_float(0, [rs0 selectedMaximumValue]);
    ui_set_property_float(1, [rs1 selectedMaximumValue]);
    ui_set_property_float(2, [rs0 selectedMinimumValue]);
    ui_set_property_float(3, [rs1 selectedMinimumValue]);
    [self dismissModalViewControllerAnimated:YES];
}

- (void)viewWillLayoutSubviews{
    [super viewWillLayoutSubviews];
    
    self.view.superview.bounds = CGRectMake(0,0,480,320);
}

@end
