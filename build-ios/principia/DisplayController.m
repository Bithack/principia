//
//  DisplayController.m
//  principia
//
//  Created by Emil on 2013-12-11.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "DisplayController.h"
#include "ui.hh"
#include "main.hh"

@interface DisplayController ()

@end

@implementation DisplayController

struct symbol {
    char values[35];
};

static struct symbol symbols[40];
static int num_symbols;
static int active_symbol;

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
    
    int sn = 0;
    const char *s = ui_get_property_string(2);
    num_symbols = 0;
    
    while (*s && sn < 40) {
        memset(&symbols[sn], 0, sizeof(struct symbol));
        int x;
        for (x=0; x<35 && *s; ) {
            if (*s == '1') {
                symbols[sn].values[x] = 1;
                x++;
            } else if (*s =='0') {
                x++;
            }
            
            s ++;
        }
        
        if (x > 0) sn ++;
    }
    
    num_symbols = sn;
    active_symbol = ui_get_property_uint8(1);
    
    UISwitch *ss = (UISwitch*)[self.view viewWithTag:600];
    [ss setOn:ui_get_property_uint8(0)];
    
    [self load_symbol];
}

- (void)load_symbol
{
    for (int x=0; x<35; x++) {
        UIView *v = [self.view viewWithTag:100+x];
        
        if (!symbols[active_symbol].values[x]) {
            [v setBackgroundColor:[UIColor colorWithRed:0.3f green:0.3f blue:0.3f alpha:1.0f]];
            [v setOpaque:NO];
        } else {
            [v setBackgroundColor:[UIColor colorWithRed:0.7f green:1.f blue:0.7f alpha:1.0f]];
            [v setOpaque:YES];
        }
    }
    
    UILabel *l = (UILabel*)[self.view viewWithTag:500];
    [l setText:[NSString stringWithFormat:@"Symbol %d/%d", active_symbol+1, num_symbols]];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    UITouch *touch = [touches anyObject];
    
    NSLog(@"touched view %d", touch.view.tag);
    
    if (touch.view.tag >= 100) {
        if ([touch.view isOpaque]) {
            [touch.view setBackgroundColor:[UIColor colorWithRed:0.3f green:0.3f blue:0.3f alpha:1.0f]];
            [touch.view setOpaque:NO];
            symbols[active_symbol].values[touch.view.tag-100] = 0;
        } else {
            [touch.view setBackgroundColor:[UIColor colorWithRed:0.7f green:1.f blue:0.7f alpha:1.0f]];
            [touch.view setOpaque:YES];
            symbols[active_symbol].values[touch.view.tag-100] = 1;
        }
    }
    
    [super touchesBegan:touches withEvent:event];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)viewWillLayoutSubviews{
    [super viewWillLayoutSubviews];
    
    self.view.superview.bounds = CGRectMake(0,0,480,320);
}

- (IBAction)done_click:(id)sender
{
    char str[35*num_symbols+1];
    for (int x=0; x<num_symbols; x++) {
        for (int y=0; y<35; y++) {
            str[x*35+y] = symbols[x].values[y] ? '1' : '0';
        }
    }
    str[num_symbols*35] = '\0';
    
    ui_set_property_string(2, str);
    ui_set_property_uint8(1, active_symbol);
    
    UISwitch *s = (UISwitch*)[self.view viewWithTag:600];
    ui_set_property_uint8(0, [s isOn]);
    
    P_add_action(ACTION_RELOAD_DISPLAY, 0);
    [self dismissModalViewControllerAnimated:YES];
}

- (IBAction)next_click:(id)sender
{
    active_symbol ++;
    if (active_symbol >= num_symbols) active_symbol = num_symbols-1;
    [self load_symbol];
}

- (IBAction)previous_click:(id)sender
{
    active_symbol --;
    if (active_symbol <= 0) active_symbol = 0;
    [self load_symbol];
}


- (IBAction)insert_click:(id)sender
{
    if (num_symbols < 40) {
        int count = (num_symbols-(active_symbol));
        if (count>0) {
            memmove(&symbols[active_symbol+1], &symbols[active_symbol], count*sizeof(struct symbol));
        }
        
        memset(&symbols[active_symbol], 0, sizeof(struct symbol));
        num_symbols ++;
        [self load_symbol];
    }
}


- (IBAction)append_click:(id)sender
{
    if (num_symbols < 40) {
        int count = (num_symbols-(active_symbol+1));
        if (count>0) {
            memmove(&symbols[active_symbol+2], &symbols[active_symbol+1], count*sizeof(struct symbol));
        }
        
        memset(&symbols[active_symbol+1], 0, sizeof(struct symbol));
        num_symbols ++;
        active_symbol++;
        [self load_symbol];
    }
}

- (IBAction)remove_click:(id)sender
{
    if (num_symbols >1) {
        int count = (num_symbols-(active_symbol+1));
        if (count>0) {
            memmove(&symbols[active_symbol], &symbols[active_symbol+1], count*sizeof(struct symbol));
        }
        
        num_symbols --;
        if (active_symbol >= num_symbols) active_symbol = num_symbols-1;
        [self load_symbol];
    }
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}
@end
