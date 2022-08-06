//
//  SequencerController.m
//  principia
//
//  Created by Emil on 2013-12-13.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "SequencerController.h"
#include "ui.hh"
#include "main.hh"

@interface SequencerController ()

@end

@implementation SequencerController

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
    
    UITextField *tf = (UITextField*)[self.view viewWithTag:101];
    [tf setText:[NSString stringWithUTF8String:ui_get_property_string(0)]];
    
    UISwitch *ss = (UISwitch*)[self.view viewWithTag:102];
    [ss setOn:ui_get_property_uint8(2)];
    
    
    int s = (int)floor((float)ui_get_property_uint32(1) / 1000.f);
    int ms = (int)roundf((float)(ui_get_property_uint32(1) % 1000) / 20.f);
    
    if (s >= 120) s = 119;
    if (ms > 49) ms = 49;
    
    UIPickerView *tv = (UIPickerView*)[self.view viewWithTag:100];
    [tv selectRow:s inComponent:0 animated:NO];
    [tv selectRow:ms inComponent:1 animated:NO];
    
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
    return 2;
}

- (NSInteger)pickerView:(UIPickerView *)pickerView
numberOfRowsInComponent:(NSInteger)component
{
    return component == 0 ? 120 : 50;
}

- (NSString *)pickerView:(UIPickerView *)pickerView
             titleForRow:(NSInteger)row
            forComponent:(NSInteger)component
{
    if (component == 0) {
        return [NSString stringWithFormat:@"%d s", row];
    } else if (component == 1) {
        return [NSString stringWithFormat:@"%d ms", row*20];
    }
    
    return @"";
}


char *asdf_nsstring_to_cstring(NSString *s)
{
    NSUInteger len = [s lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    NSUInteger tja;
    char *tmp =  malloc(len+1);
    NSRange r = NSMakeRange(0, [s length]);
    [s getBytes:tmp maxLength:len usedLength:&tja encoding:NSUTF8StringEncoding options:0 range:r remainingRange:NULL];
    tmp[len] = '\0';
    return tmp;
}


- (IBAction)done_click:(id)sender
{
    UIPickerView *tv = (UIPickerView*)[self.view viewWithTag:100];

    int ms = [tv selectedRowInComponent:0] * 1000 + [tv selectedRowInComponent:1]*20;
    if (ms < 16) {
        ms = 16;
    }
    
    ui_set_property_uint32(1, ms);
    
    UITextField *tf = (UITextField*)[self.view viewWithTag:101];
    char *tmp = asdf_nsstring_to_cstring([tf text]);
    if (strlen(tmp)) {
        ui_set_property_string(0, tmp);
    } else
        ui_set_property_string(0, "010101010");
    free(tmp);
    
    UISwitch *s = (UISwitch*)[self.view viewWithTag:102];
    ui_set_property_uint8(2, [s isOn]);
    
    ui_cb_refresh_sequencer();
    
    [self dismissModalViewControllerAnimated:YES];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}
@end
