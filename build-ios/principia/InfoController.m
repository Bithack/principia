//
//  InfoController.m
//  principia
//
//  Created by Emil on 2013-12-09.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "InfoController.h"

@interface InfoController ()

@end

@implementation InfoController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (IBAction)done_click:(id)sender
{
    [self dismissModalViewControllerAnimated:YES];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    UIWebView *tv = (UIWebView*)[self.view viewWithTag:100];
    
    [tv loadHTMLString:[_txt_descr stringByReplacingOccurrencesOfString:@"\n" withString:@"<br />"] baseURL:[[NSURL alloc] initWithString:@"http://www.principiagame.com/"]];
    
   // [tv setText:_txt_descr];
    
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
