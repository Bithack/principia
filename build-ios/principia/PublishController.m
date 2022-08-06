//
//  PublishController.m
//  principia
//
//  Created by Emil on 2013-12-13.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "PublishController.h"
#include "ui.hh"
#include "main.hh"

@interface PublishController ()

@end

@implementation PublishController

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
    
    UITextField *tf;
    UITextView *tv;
    UISwitch *derivatives_switch = (UISwitch*)[self.view viewWithTag:3];
    UISwitch *hidden_switch = (UISwitch*)[self.view viewWithTag:4];
    tf = (UITextField*)[self.view viewWithTag:1];
    tv = (UITextView*)[self.view viewWithTag:2];
    
    [derivatives_switch setOn:ui_cb_get_allow_derivatives()];
    [hidden_switch setOn:ui_cb_get_locked()];
    
    [tf setText:[NSString stringWithUTF8String:ui_cb_get_level_title()]];
    [tv setText:[NSString stringWithUTF8String:ui_cb_get_level_description()]];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

char *hora_nsstring_to_cstring(NSString *s)
{
    NSUInteger len = [s lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    NSUInteger tja;
    char *tmp =  malloc(len+1);
    NSRange r = NSMakeRange(0, [s length]);
    [s getBytes:tmp maxLength:len usedLength:&tja encoding:NSUTF8StringEncoding options:0 range:r remainingRange:NULL];
    tmp[len] = '\0';
    return tmp;
}


- (IBAction)cancel_click:(id)sender
{
    [self dismissModalViewControllerAnimated:YES];
}

- (IBAction)done_click:(id)sender
{
    UITextField *tf;
    UITextView *tv;
    char *tmp = 0;
    UISwitch *derivatives_switch = (UISwitch*)[self.view viewWithTag:3];
    UISwitch *hidden_switch = (UISwitch*)[self.view viewWithTag:4];
    
    tf = (UITextField*)[self.view viewWithTag:1];
    tmp = hora_nsstring_to_cstring([tf text]);
    
    if (!strlen(tmp)) {
        ui_message("You must specify a name for the level.", false);
        return;
    }
    
    ui_cb_set_level_title(tmp);
    free (tmp);
    
    tv = (UITextView*)[self.view viewWithTag:2];
    tmp = hora_nsstring_to_cstring([tv text]);
    ui_cb_set_level_description(tmp);
    free (tmp);
    
    ui_cb_set_allow_derivatives([derivatives_switch isOn]);
    ui_cb_set_locked([hidden_switch isOn]);
    
    P_add_action(ACTION_PUBLISH, 0);
    
    [self dismissModalViewControllerAnimated:NO];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}
- (void)viewWillLayoutSubviews{
    [super viewWillLayoutSubviews];
    
    self.view.superview.bounds = CGRectMake(0,0,480,320);
}

@end
