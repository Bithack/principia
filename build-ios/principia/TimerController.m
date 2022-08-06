//
//  TimerController.m
//  principia
//
//  Created by Emil on 2013-12-13.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "TimerController.h"
#include "ui.hh"
#include "main.hh"

@interface TimerController ()

@end

@implementation TimerController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
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


- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component
{
    NSLog(@"sel");
}

- (NSInteger)numberOfComponentsInPickerView:
(UIPickerView *)pickerView
{
    return 3;
}

- (NSInteger)pickerView:(UIPickerView *)pickerView
numberOfRowsInComponent:(NSInteger)component
{
    return component == 0 ? 120 : (component == 1 ? 50 : 255);
}

- (NSString *)pickerView:(UIPickerView *)pickerView
             titleForRow:(NSInteger)row
            forComponent:(NSInteger)component
{
    if (component == 0) {
        return [NSString stringWithFormat:@"%d s", row];
    } else if (component == 1) {
        return [NSString stringWithFormat:@"%d ms", row*20];
    } else if (component == 2) {
        return [NSString stringWithFormat:@"%d ticks", row];
    }
    
    return @"";
}


- (IBAction)done_click:(id)sender
{
    UIPickerView *tv = (UIPickerView*)[self.view viewWithTag:100];
    
    int ms = [tv selectedRowInComponent:0] * 1000 + [tv selectedRowInComponent:1]*20;
    int num_ticks = [tv selectedRowInComponent:2];
    
    if (ms < 16) {
        ms = 16;
    }
    
    ui_set_property_uint32(0,(uint32_t) ms);
    ui_set_property_uint8(1,(uint8_t) num_ticks);
    
    [self dismissModalViewControllerAnimated:YES];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}


- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
    
    UIPickerView *tv = (UIPickerView*)[self.view viewWithTag:100];
    
    int s = (int)floor((float)ui_get_property_uint32(0) / 1000.f);
    int ms = (int)roundf((float)(ui_get_property_uint32(0) % 1000) / 20.f);
    
    if (s >= 120) s = 119;
    if (ms > 49) ms = 49;
    
    int num_ticks = ui_get_property_uint8(1);
    
    [tv selectRow:s inComponent:0 animated:NO];
    [tv selectRow:ms inComponent:1 animated:NO];
    [tv selectRow:num_ticks inComponent:2 animated:NO];
}

@end
