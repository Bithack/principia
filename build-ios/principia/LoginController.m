//
//  LoginController.m
//  principia
//
//  Created by Emil on 2013-12-13.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "LoginController.h"
#include "ui.hh"
#include "main.hh"

@interface LoginController ()

@end

@implementation LoginController

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
}

char *coolio_nsstring_to_cstring(NSString *s)
{
    NSUInteger len = [s lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    NSUInteger tja;
    char *tmp =  malloc(len+1);
    NSRange r = NSMakeRange(0, [s length]);
    [s getBytes:tmp maxLength:len usedLength:&tja encoding:NSUTF8StringEncoding options:0 range:r remainingRange:NULL];
    tmp[len] = '\0';
    return tmp;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}
- (IBAction)cancel_click:(id)sender
{
    [self dismissModalViewControllerAnimated:YES];
}

- (IBAction)register_click:(id)sender
{
    [self dismissModalViewControllerAnimated:NO];
    ui_open_dialog(DIALOG_REGISTER);
}

- (IBAction)done_click:(id)sender
{
    UITextField *tf_username = (UITextField*)[self.view viewWithTag:101];
    UITextField *tf_password = (UITextField*)[self.view viewWithTag:102];
    
    char *u = coolio_nsstring_to_cstring([tf_username text]);
    char *p = coolio_nsstring_to_cstring([tf_password text]);
    
    int u_len = strlen(u);
    int p_len = strlen(p);
    if (u_len >= 255) u_len = 255;
    if (p_len >= 255) p_len = 255;
    
    NSLog(@"username %d, pwd %d", u_len, p_len);
    
    if (u_len < 1 || p_len < 1) {
        free(u);
        free(p);
        ui_message("Please enter your username and password.", false);
        return;
    }
    
    struct login_data *data = malloc(sizeof(struct login_data));
    
    memcpy(data->username, u, u_len);
    memcpy(data->password, p, p_len);
    
    data->username[u_len] = '\0';
    data->password[p_len] = '\0';
    
    free(u);
    free(p);
    
    P_add_action(ACTION_LOGIN, (void*)data);
    
    [self dismissModalViewControllerAnimated:YES];
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
