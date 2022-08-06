//
//  StickyController.m
//  principia
//
//  Created by Emil on 2013-12-16.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "StickyController.h"
#include "ui.hh"
#include "main.hh"

@interface StickyController ()

@end

@implementation StickyController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

char *coolio4_nsstring_to_cstring(NSString *s)
{
    NSUInteger len = [s lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    NSUInteger tja;
    char *tmp =  malloc(len+1);
    NSRange r = NSMakeRange(0, [s length]);
    [s getBytes:tmp maxLength:len usedLength:&tja encoding:NSUTF8StringEncoding options:0 range:r remainingRange:NULL];
    tmp[len] = '\0';
    return tmp;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
    
    UITextView *tv = (UITextView*)[self.view viewWithTag:100];
    [tv setText:[NSString stringWithUTF8String:ui_get_property_string(0)]];
    
    UISegmentedControl *seg = (UISegmentedControl*)[self.view viewWithTag:101];
    [seg setSelectedSegmentIndex:ui_get_property_uint8(3)];
    
    UISwitch *s = (UISwitch*)[self.view viewWithTag:102];
    [s setOn:ui_get_property_uint8(1)];
    
    s = (UISwitch*)[self.view viewWithTag:103];
    [s setOn:ui_get_property_uint8(2)];
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
    UITextView *tv = (UITextView*)[self.view viewWithTag:100];
    
    UISegmentedControl *seg = (UISegmentedControl*)[self.view viewWithTag:101];
    ui_set_property_uint8(3, [seg selectedSegmentIndex]);
    
    UISwitch *s = (UISwitch*)[self.view viewWithTag:102];
    ui_set_property_uint8(1, [s isOn]);
    
    s = (UISwitch*)[self.view viewWithTag:103];
    ui_set_property_uint8(2, [s isOn]);
    
    char *tmp;
    tmp = coolio4_nsstring_to_cstring([tv text]);
    P_add_action(ACTION_SET_STICKY_TEXT, strdup(tmp));
    free(tmp);
    
    [self dismissModalViewControllerAnimated:YES];
}

- (void)viewWillLayoutSubviews{
    [super viewWillLayoutSubviews];
    
    self.view.superview.bounds = CGRectMake(0,0,480,320);
}

@end
