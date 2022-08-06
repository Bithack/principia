//
//  FreqRangeController.m
//  principia
//
//  Created by Emil on 2013-12-16.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "FreqRangeController.h"

@interface FreqRangeController ()

@end

@implementation FreqRangeController

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
    UITextField *tf = (UITextField*)[self.view viewWithTag:100];
    [tf setText:[NSString stringWithFormat:@"%d", ui_get_property_uint32(0)]];
    tf = (UITextField*)[self.view viewWithTag:101];
    [tf setText:[NSString stringWithFormat:@"%d", ui_get_property_uint32(1)]];
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
    UITextField *tf = (UITextField*)[self.view viewWithTag:100];
    int freq = [[tf text] intValue];
    
    tf = (UITextField*)[self.view viewWithTag:101];
    int range = [[tf text] intValue];
    ui_set_property_uint32(1, range);
    
    [self dismissModalViewControllerAnimated:YES];
}

- (void)viewWillLayoutSubviews{
    [super viewWillLayoutSubviews];
    
    self.view.superview.bounds = CGRectMake(0,0,480,320);
}
@end
