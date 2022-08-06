//
//  CommunityController.m
//  principia
//
//  Created by Emil on 2014-01-02.
//  Copyright (c) 2014 Bithack AB. All rights reserved.
//

#import "CommunityController.h"
#include "main.hh"

char *community_next_url = 0;

@interface CommunityController ()

@end

@implementation CommunityController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    NSDictionary *dictionary = [NSDictionary dictionaryWithObjectsAndKeys:
                                [NSString stringWithUTF8String: "Principia WebView/" PRINCIPIA_VERSION_CODE_STRING " (" OS_STRING ")"], @"UserAgent", nil];
    [[NSUserDefaults standardUserDefaults] registerDefaults:dictionary];
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (IBAction)back_click:(id)sender
{
    UIWebView *w = (UIWebView*)[self.view viewWithTag:100];
    
    [w goBack];
}

- (IBAction)reload_click:(id)sender
{
    UIWebView *w = (UIWebView*)[self.view viewWithTag:100];
    
    [w reload];
}

- (IBAction)close_click:(id)sender
{
    [self dismissModalViewControllerAnimated:YES];
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    
    uint64_t uid = 1;
    
    /* sync client and browser session cookie */
    NSArray *cookies = [[NSHTTPCookieStorage sharedHTTPCookieStorage] cookiesForURL:[NSURL URLWithString:@"http://principiagame.com/"]];
    
    NSHTTPCookie *cookie;
    for (cookie in cookies) {
        if ([[cookie name] compare:@"phpbb_ziao2_u"] == NSOrderedSame) {
            uid = [[cookie value] longLongValue];
        }
    }
    
    if (uid == 1) {
        /* user is not logged in in the webview, move over curl data if we have any */
        char *u,*k,*sid;
        P_get_cookie_data(&u,&k,&sid);
        
        if (!u) u = "1";
        if (!sid) sid = "";
        if (!k) k = "";
        
        long p_uid = atol(u);
        
        NSLog(@"cookie data %s %s %s", u, k, sid);
        
        if (p_uid != 1 || sid[0]=='\0') {
            {
                NSDictionary *props = [NSDictionary dictionaryWithObjectsAndKeys:
                                    @"principiagame.com", NSHTTPCookieOriginURL,
                                    @"phpbb_ziao2_u", NSHTTPCookieName,
                                   [NSString stringWithFormat:@"%u", (uint32_t)p_uid], NSHTTPCookieValue,
                                   @"/", NSHTTPCookiePath,
                                   @".principiagame.com", NSHTTPCookieDomain,
                                   nil
                                   ];
                NSHTTPCookie *cookie = [NSHTTPCookie cookieWithProperties:props];
                [[NSHTTPCookieStorage sharedHTTPCookieStorage] setCookie:cookie];
            }
            {
                NSDictionary *props = [NSDictionary dictionaryWithObjectsAndKeys:
                                       @"principiagame.com", NSHTTPCookieOriginURL,
                                       @"phpbb_ziao2_sid", NSHTTPCookieName,
                                       [NSString stringWithUTF8String:sid], NSHTTPCookieValue,
                                       @"/", NSHTTPCookiePath,
                                       @".principiagame.com", NSHTTPCookieDomain,
                                       nil
                                       ];
                NSHTTPCookie *cookie = [NSHTTPCookie cookieWithProperties:props];
                [[NSHTTPCookieStorage sharedHTTPCookieStorage] setCookie:cookie];
            }
            {
                NSDictionary *props = [NSDictionary dictionaryWithObjectsAndKeys:
                                       @"principiagame.com", NSHTTPCookieOriginURL,
                                       @"phpbb_ziao2_k", NSHTTPCookieName,
                                       [NSString stringWithUTF8String:k], NSHTTPCookieValue,
                                       @"/", NSHTTPCookiePath,
                                       @".principiagame.com", NSHTTPCookieDomain,
                                       nil
                                       ];
                NSHTTPCookie *cookie = [NSHTTPCookie cookieWithProperties:props];
                [[NSHTTPCookieStorage sharedHTTPCookieStorage] setCookie:cookie];
            }
        }
        
    }
    
    if ([[[request URL] scheme] compare:@"principia"] == NSOrderedSame) {
        NSLog(@"opening principia scheme url");
        [self close_click:0];
    } else if ([[[request URL] scheme] compare:@"http"] != NSOrderedSame) {
        return NO;
    } else {
        /* if the url is a principiagame.com url, open in webview, otherwise open in default browser */
        if ([[[request URL] host] compare:@"principiagame.com"] == NSOrderedSame
            || [[[request URL] host] compare:@"www.principiagame.com"] == NSOrderedSame
            || [[[request URL] host] compare:@"img.principiagame.com"] == NSOrderedSame
            || [[[request URL] host] compare:@"test.principiagame.com"] == NSOrderedSame
            ) {
            
            NSLog(@"open internal %@", [request URL]);
            //[webView stopLoading];
            return YES;
        } else {
            [[UIApplication sharedApplication] openURL:[request URL]];
            NSLog(@"opening url %@", [request URL]);
            return NO;
        }
    }
    //[webView stopLoading];
    return YES;
}

- (void)webView:(UIWebView *)webview didFailLoadWithError:(NSError*)error
{
    UILabel *l = (UILabel*)[self.view viewWithTag:201];
    [l setText:@"Community - Error"];
    NSLog(@"Failed loading");
    
    UIWebView *w = (UIWebView*)[self.view viewWithTag:100];
    
    NSLog(@"%@", error.localizedDescription);
    
    NSString* errorString = [NSString stringWithFormat:
                             
                             @"<html><body><font size=+2 color='red'>An error occurred while loading the community website:<br><strong>%@</strong><br><br>If the trouble persists, please contact an administrator through support@bithack.se</font></center></body></html>",
                             
                             error.localizedDescription];
    
    [w loadHTMLString:errorString baseURL:[NSURL URLWithString:@"http://principiagame.com/"]];
}

- (void)webViewDidStartLoad:(UIWebView*)webView
{
    UILabel *l = (UILabel*)[self.view viewWithTag:201];
    [l setText:@"Loading ..."];
    
    NSLog(@"Start loading");
}


- (void)webViewDidFinishLoad:(UIWebView*)webView
{
    UILabel *l = (UILabel*)[self.view viewWithTag:201];
    [l setText:@"Community"];
    NSLog(@"Finished loading");
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
    
    UIWebView *w = (UIWebView*)[self.view viewWithTag:100];
    
    
    if (community_next_url == 0)
        [w loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"http://principiagame.com/"]]];
    
    UIToolbar *t = (UIToolbar*)[self.view viewWithTag:500];
    
}

- (void)viewDidAppear:(BOOL)animated
{
    UIWebView *w = (UIWebView*)[self.view viewWithTag:100];

    if (community_next_url != 0) {
        NSString *curr_url = [w stringByEvaluatingJavaScriptFromString:@"window.location.href"];
        NSLog(@"Current URL: %@, new url: %s", curr_url, community_next_url);
        if (curr_url == nil || [curr_url compare:[NSString stringWithUTF8String:community_next_url]] != NSOrderedSame) {
            [w loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:[NSString stringWithUTF8String:community_next_url]]]];
        } else
            NSLog(@"URL %s already loaded", community_next_url);
        community_next_url = 0;
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
