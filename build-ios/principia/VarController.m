//
//  VarController.m
//  principia
//
//  Created by Emil on 2013-12-11.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "VarController.h"
#include "ui.hh"
#include "main.hh"

@interface VarController ()

@end

@implementation VarController

static char name[51];
static int name_len = 0;

const char* cleanup_name(const char* s)
{
    int s_len = strlen(s);
    name_len = 0;
    for (int x=0; x<s_len; x++) {
        if (isalnum(s[x]) || s[x] == '_' || s[x] == '-')
            name[name_len++] = s[x];
    }
    name[name_len] = '\0';
    return name;
}

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
    
    UITextField *v = (UITextField*)[self.view viewWithTag:100];
    
    [v setText:[[NSString alloc] initWithUTF8String:ui_get_property_string(0)]];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


char *hej_nsstring_to_cstring(NSString *s)
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
    UITextField *v = (UITextField*)[self.view viewWithTag:100];
    char *tmp =hej_nsstring_to_cstring([v text]);
    
    cleanup_name(tmp);
    free(tmp);
    
    if (name_len)
        ui_set_property_string(0, name);
    
    [self dismissModalViewControllerAnimated:YES];
}


- (IBAction)reset_variable:(id)sender
{
    UITextField *v = (UITextField*)[self.view viewWithTag:100];
    char *tmp =hej_nsstring_to_cstring([v text]);
    cleanup_name(tmp);
    free(tmp);
    
    ui_cb_reset_variable(name);
}
                         
- (IBAction)reset_all_variables:(id)sender
{
    ui_cb_reset_all_variables();
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
