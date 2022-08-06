//
//  PromptEditController.m
//  principia
//
//  Created by Emil on 2013-12-13.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "PromptEditController.h"
#include "ui.hh"
#include "main.hh"

@interface PromptEditController ()

@end

@implementation PromptEditController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}


char *coolio3_nsstring_to_cstring(NSString *s)
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
    
    UITextField *tf = (UITextField*)[self.view viewWithTag:101];
    [tf setText:[NSString stringWithUTF8String:ui_get_property_string(0)]];
    
    tf = (UITextField*)[self.view viewWithTag:102];
    [tf setText:[NSString stringWithUTF8String:ui_get_property_string(1)]];
    
    tf = (UITextField*)[self.view viewWithTag:103];
    [tf setText:[NSString stringWithUTF8String:ui_get_property_string(2)]];
    
    UITextView *v = (UITextView*)[self.view viewWithTag:100];
    
    [v setText:[NSString stringWithUTF8String:ui_get_property_string(3)]];
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
    char *tmp;
    UITextField *tf = (UITextField*)[self.view viewWithTag:101];
    tmp = coolio3_nsstring_to_cstring([tf text]);
    ui_set_property_string(0, tmp);
    if (!strlen(tmp))
        ui_set_property_string(0, "OK");
    free(tmp);
    
    tf = (UITextField*)[self.view viewWithTag:102];
    tmp = coolio3_nsstring_to_cstring([tf text]);
    ui_set_property_string(1, tmp);
    free(tmp);
    
    tf = (UITextField*)[self.view viewWithTag:103];
    tmp = coolio3_nsstring_to_cstring([tf text]);
    ui_set_property_string(2, tmp);
    free(tmp);
    
    UITextView *v = (UITextView*)[self.view viewWithTag:100];
    tmp = coolio3_nsstring_to_cstring([v text]);
    ui_set_property_string(3, tmp);
    if (!strlen(tmp))
        ui_set_property_string(3, "???");
    free(tmp);
    
    [self dismissModalViewControllerAnimated:YES];
}

- (void)viewWillLayoutSubviews{
    [super viewWillLayoutSubviews];
    
    self.view.superview.bounds = CGRectMake(0,0,480,320);
}
@end
