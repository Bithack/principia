//
//  SFXEmitterController.m
//  principia
//
//  Created by Emil on 2013-12-09.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "SFXEmitterController.h"
#include "ui.hh"
#include "i2o0gate.hh"

@interface SFXEmitterController ()

@end

@implementation SFXEmitterController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
        
        _pickervalues = [[NSMutableArray alloc] initWithCapacity:NUM_SFXEMITTER_OPTIONS];
        for (int x=0; x<NUM_SFXEMITTER_OPTIONS; x++) {
            [_pickervalues addObject:[[NSString alloc] initWithUTF8String:sfxemitter_options[x].name]];
        }
        
    }
    return self;
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
    return 1;
}

- (NSInteger)pickerView:(UIPickerView *)pickerView
numberOfRowsInComponent:(NSInteger)component
{
    return _pickervalues.count;
}

- (NSString *)pickerView:(UIPickerView *)pickerView
             titleForRow:(NSInteger)row
            forComponent:(NSInteger)component
{
    return _pickervalues[row];
}


- (IBAction)done_click:(id)sender
{
    UIPickerView *tv = (UIPickerView*)[self.view viewWithTag:100];
    UISwitch *ts = (UISwitch*)[self.view viewWithTag:101];
    ui_set_property_uint32(0, [tv selectedRowInComponent:0]);
    ui_set_property_uint8(1, [ts isOn]);
    
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
    UISwitch *ts = (UISwitch*)[self.view viewWithTag:101];
    [ts setOn:ui_get_property_uint8(1)];
    [tv selectRow:ui_get_property_uint32(0) inComponent:0 animated:NO];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
