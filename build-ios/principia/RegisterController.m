//
//  RegisterController.m
//  principia
//
//  Created by Emil on 2013-12-13.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "RegisterController.h"
#include "ui.hh"
#include "main.hh"

RegisterController *register_dialog = 0;

@interface RegisterController ()

@end

@implementation RegisterController

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

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}
- (IBAction)cancel_click:(id)sender
{
    register_dialog = 0;
    [self dismissModalViewControllerAnimated:YES];
}

char *coolio2_nsstring_to_cstring(NSString *s)
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
    if (![(UITextField*)[self.view viewWithTag:100] isEnabled])
        return;
    
    UITextField *tf_username = (UITextField*)[self.view viewWithTag:100];
    UITextField *tf_email = (UITextField*)[self.view viewWithTag:101];
    UITextField *tf_password = (UITextField*)[self.view viewWithTag:102];
    UITextField *tf_confirm = (UITextField*)[self.view viewWithTag:103];
    
    char *u = coolio2_nsstring_to_cstring([tf_username text]);
    char *e = coolio2_nsstring_to_cstring([tf_email text]);
    char *p = coolio2_nsstring_to_cstring([tf_password text]);
    char *c = coolio2_nsstring_to_cstring([tf_confirm text]);
    
    int u_len = strlen(u);
    int e_len = strlen(e);
    int p_len = strlen(p);
    int c_len = strlen(c);
    if (u_len >= 255) u_len = 255;
    if (e_len >= 255) e_len = 255;
    if (p_len >= 255) p_len = 255;
    if (c_len >= 255) c_len = 255;
    
    if (u_len < 1 || p_len < 1 || e_len < 1 || c_len < 1) {
        ui_message("Please enter all credentials to register.", false);
        goto err;
    }
    
    if (strcmp(p,c) != 0) {
        ui_message("The passwords do not match.", false);
        goto err;
    }
    
    struct register_data *data = malloc(sizeof(struct register_data));
    data->platform = PLATFORM_IOS;
    
    memcpy(data->password, p, p_len);
    data->password[p_len] = '\0';
    memcpy(data->username, u, u_len);
    data->username[u_len] = '\0';
    memcpy(data->email, e, e_len);
    data->email[e_len] = '\0';
    
    strcpy(data->userdata, "");
    strcpy(data->signature, "");
    
    P_add_action(ACTION_REGISTER, (void*)data);
    
    [(UITextField*)[self.view viewWithTag:100] setEnabled:NO];
    [(UITextField*)[self.view viewWithTag:101] setEnabled:NO];
    [(UITextField*)[self.view viewWithTag:102] setEnabled:NO];
    [(UITextField*)[self.view viewWithTag:103] setEnabled:NO];
    
    [(UILabel*)[self.view viewWithTag:900] setText:@"Loading. Please wait..."];
    
   // [self dismissModalViewControllerAnimated:YES];
    
err:
    free(u);
    free(e);
    free(c);
    free(p);
}

- (void)enable_inputs
{
    [(UITextField*)[self.view viewWithTag:100] setEnabled:YES];
    [(UITextField*)[self.view viewWithTag:101] setEnabled:YES];
    [(UITextField*)[self.view viewWithTag:102] setEnabled:YES];
    [(UITextField*)[self.view viewWithTag:103] setEnabled:YES];
    [(UILabel*)[self.view viewWithTag:900] setText:@""];
}

- (void)dismiss_with_success
{
    register_dialog = 0;
    [self dismissModalViewControllerAnimated:NO];
    P_add_action(ACTION_PUBLISH, 0);
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
