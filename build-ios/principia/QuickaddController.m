//
//  QuickaddController.m
//  principia
//
//  Created by Emil on 2013-12-30.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "QuickaddController.h"
#include "ui.hh"
#include "main.hh"

static NSMutableArray *objects = nil;
static char *last_string = 0;

void ios_quickadd_clear()
{
    if (objects == nil) {
        objects = [[NSMutableArray alloc] init];
        NSLog(@"alloc objects");
    }

    if (last_string) {
    } else {
        [objects removeAllObjects];
        
        for (int x=0; x<g_num_menu_objects; x++) {
            [objects addObject:[NSString stringWithUTF8String:g_all_menu_objects[x].name]];
        }
    }
}

char *coolio6_nsstring_to_cstring(NSString *s)
{
    NSUInteger len = [s lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    NSUInteger tja;
    char *tmp =  malloc(len+1);
    NSRange r = NSMakeRange(0, [s length]);
    [s getBytes:tmp maxLength:len usedLength:&tja encoding:NSUTF8StringEncoding options:0 range:r remainingRange:NULL];
    tmp[len] = '\0';
    return tmp;
}

@interface QuickaddController ()

@end

@implementation QuickaddController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return objects == nil ? 0 : [objects count];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (IBAction)close_click:(id)sender
{
    [self dismissModalViewControllerAnimated:YES];
}

- (IBAction)done_click:(id)sender
{
    if (last_string) free(last_string);
    last_string = coolio6_nsstring_to_cstring([(UITextField*)[self.view viewWithTag:100] text]);
    
    NSString *search = [(UITextField*)[self.view viewWithTag:100] text];
    
    for (int x=0; x<g_num_menu_objects; x++) {
        NSString *oname =[NSString stringWithUTF8String:g_all_menu_objects[x].name];
        
        if ([search length] <= [oname length]) {
            if ([[oname substringToIndex:[search length]] compare:search options:NSCaseInsensitiveSearch] == NSOrderedSame) {
                
                //[objects addObject:oname];
                
                P_add_action(ACTION_CONSTRUCT_ENTITY, (void*)g_all_menu_objects[x].g_id);
                break;
            }
        }
    }
    
    [self dismissModalViewControllerAnimated:YES];
}

- (BOOL)textField:(UITextField *)textfield shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
    if ([string isEqualToString:@"\n"]) {
        [self done_click:0];
        return NO;
    }
    
    [objects removeAllObjects];
    
    NSString *search = [NSString stringWithString:textfield.text];
    search = [search stringByReplacingCharactersInRange:range withString:string];
    
    for (int x=0; x<g_num_menu_objects; x++) {
        NSString *oname =[NSString stringWithUTF8String:g_all_menu_objects[x].name];
        
        if ([search length] <= [oname length]) {
            if ([[oname substringToIndex:[search length]] compare:search options:NSCaseInsensitiveSearch] == NSOrderedSame) {
                
                [objects addObject:oname];
            }
        }
    }
    
    UITableView *t = (UITableView*)[self.view viewWithTag:101];
    [t reloadData];
    return YES;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"Cell";
    
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier] autorelease];
    }
    
    cell.textLabel.text = objects[indexPath.row];
    
    cell.accessoryType = UITableViewCellAccessoryNone;
    
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (last_string) free(last_string);
    last_string = coolio6_nsstring_to_cstring([(UITextField*)[self.view viewWithTag:100] text]);
    
    NSString *selected = objects[indexPath.row];
    
    for (int x=0; x<g_num_menu_objects; x++) {
        //if (strcmp(last_string, g_all_menu_objects[x].name) == 0) {
        if ([selected compare:[NSString stringWithUTF8String:g_all_menu_objects[x].name]] == NSOrderedSame) {
            P_add_action(ACTION_CONSTRUCT_ENTITY, (void*)g_all_menu_objects[x].g_id);
        }
    }
    
    [self dismissModalViewControllerAnimated:YES];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
    
    [[self.view viewWithTag:100] becomeFirstResponder];
    if (last_string != 0)
        [(UITextField*)[self.view viewWithTag:100] setText:[NSString stringWithUTF8String:last_string]];
    
    [(UITableView*)[self.view viewWithTag:101] reloadData];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
