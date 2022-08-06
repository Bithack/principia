//
//  FXEmitterController.m
//  principia
//
//  Created by Emil on 2013-12-09.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "FXEmitterController.h"
#import "ui.hh"

@interface FXEmitterController ()

@end

@implementation FXEmitterController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
        
        _pickervalues = [[NSArray alloc] initWithArray:@[ @"",
                                                          @"Explosion",
                                                          @"Highlight",
                                                          @"Destroy connections",
                                                          @"Smoke",
                                                          @"Magic",
                                                          @"Break",
                                                          ]];
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
    
    for (int x=0; x<3; x++) {
        int fx = ui_cb_get_fx(x);
        /*if (fx == 0xdeadbeef) fx = 0;
        else fx ++;*/
        NSLog(@"fx %d is %d", x, fx);
        [inputPicker selectRow:fx inComponent:x animated:FALSE];
    }
        
    UISlider *slider;
        
    slider = (UISlider*)[self.view viewWithTag:100]; /* radius */
    [slider setValue:(int)roundf(ui_get_property_float(0) * 1000.) animated:NO];
        
    slider = (UISlider*)[self.view viewWithTag:101]; /* count */
    [slider setValue:ui_get_property_uint32(1) animated:NO];
        
    slider = (UISlider*)[self.view viewWithTag:102]; /* interval */
    [slider setValue:(int)roundf(ui_get_property_float(2) * 1000.) animated:NO];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)done_click:(id)sender
{
    for (int x=0; x<3; x++) {
        int fx = [inputPicker selectedRowInComponent:x];
        /*if (fx == 0) fx = 0xdeadbeef;
        else fx--;*/
        NSLog(@"setting fx %d %d", x, fx);
        ui_cb_set_fx(x, fx);
    }
    
    UISlider *slider;
    
    slider = (UISlider*)[self.view viewWithTag:100]; /* radius */
    ui_set_property_float(0, [slider value] / 1000.f);
    NSLog(@"slider value %f", [slider value]);
    
    slider = (UISlider*)[self.view viewWithTag:101]; /* count */
    ui_set_property_uint32(1, (uint32_t)[slider value]);
    NSLog(@"slider value %f", [slider value]);
    
    slider = (UISlider*)[self.view viewWithTag:102]; /* interval */
    ui_set_property_float(2, [slider value] / 1000.f);
    NSLog(@"slider value %f", [slider value]);
    
    [self dismissModalViewControllerAnimated:YES];
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
    return _pickervalues.count;
}

- (NSString *)pickerView:(UIPickerView *)pickerView
             titleForRow:(NSInteger)row
            forComponent:(NSInteger)component
{
    return _pickervalues[row];
}

- (NSInteger)getSelected
{
    return [inputPicker selectedRowInComponent:0];
}

- (NSInteger)getSelected_2
{
    return [inputPicker selectedRowInComponent:1];
}

- (NSInteger)getSelected_3
{
    return [inputPicker selectedRowInComponent:2];
}


@end
