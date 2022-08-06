//
//  EscriptController.m
//  principia
//
//  Created by Emil on 2013-12-30.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "EscriptController.h"
#include "ui.hh"

@interface EscriptController ()

@end

@implementation EscriptController

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
    
    
    UITextView *tv = (UITextView*)[self.view viewWithTag:100];
    
    [tv setText:[NSString stringWithUTF8String:ui_get_property_string(0)]];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWasShown:)
                                                 name:UIKeyboardDidShowNotification object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWillBeHidden:)
                                                 name:UIKeyboardWillHideNotification object:nil];

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


char *coolio5_nsstring_to_cstring(NSString *s)
{
    NSUInteger len = [s lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    NSUInteger tja;
    char *tmp =  malloc(len+1);
    NSRange r = NSMakeRange(0, [s length]);
    [s getBytes:tmp maxLength:len usedLength:&tja encoding:NSUTF8StringEncoding options:0 range:r remainingRange:NULL];
    tmp[len] = '\0';
    return tmp;
}

- (IBAction)close_click:(id)sender
{
    [self dismissModalViewControllerAnimated:YES];
}

- (void)textViewDidBeginEditing:(UITextView *)textView {
    /*CGRect textViewFrame = CGRectInset(self.view.bounds, 20., 20.);
    NSLog(@"start edit");
    textViewFrame.size.height -= 216;
    textView.frame = textViewFrame;*/
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    /*CGRect textViewFrame = CGRectInset(self.view.bounds, 20., 20.);
    textView.frame = textViewFrame;
    [textView endEditing:YES];*/
    [super touchesBegan:touches withEvent:event];
}

- (void)keyboardWasShown:(NSNotification*)notification {
    NSDictionary* info = [notification userInfo];
    CGSize keyboardSize = [[info objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue].size;
    
    textView.contentInset = UIEdgeInsetsMake(0, 0, keyboardSize.height, 0);
    textView.scrollIndicatorInsets = textView.contentInset;
}

- (void)keyboardWillBeHidden:(NSNotification*)notification {
    textView.contentInset = UIEdgeInsetsZero;
    textView.scrollIndicatorInsets = UIEdgeInsetsZero;
}

- (IBAction)done_click:(id)sender
{
    UITextView *tv = (UITextView*)[self.view viewWithTag:100];
    
    char *tmp = coolio5_nsstring_to_cstring([tv text]);
    ui_set_property_string(0, tmp);
    free(tmp);
    
    [self dismissModalViewControllerAnimated:YES];
}

- (void)viewWillLayoutSubviews{
    [super viewWillLayoutSubviews];
    
    //self.view.superview.bounds = CGRectMake(0,0,480,320);
}

@end
