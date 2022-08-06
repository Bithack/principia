//
//  PropertiesWorldController.m
//  principia
//
//  Created by Emil on 2013-10-24.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "PropertiesWorldController.h"

@interface PropertiesWorldController ()

@end

@implementation PropertiesWorldController

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

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component
{
}


- (NSInteger)numberOfComponentsInPickerView:
(UIPickerView *)pickerView
{
    return 1;
}

- (NSInteger)pickerView:(UIPickerView *)pickerView
numberOfRowsInComponent:(NSInteger)component
{
    return 6;
}

- (IBAction)slider_change:(id)sender
{
    for (int x=0; x<4; x++) {
        UISlider *s = (UISlider*)[self.view viewWithTag:100+x];
        UILabel *l = (UILabel*)[self.view viewWithTag:200+x];
        
        if (x < 2)
            [l setText:[NSString stringWithFormat:@"%d", (int)[s value]]];
        else
            [l setText:[NSString stringWithFormat:@"%.4f", [s value]]];
    }
}


- (NSString *)pickerView:(UIPickerView *)pickerView
             titleForRow:(NSInteger)row
            forComponent:(NSInteger)component
{
    switch (row) {
        case 0: return @"Wood 1";
        case 1: return @"Wood 2";
        case 2: return @"Concrete";
        case 3: return @"Space";
        case 4: return @"Wood 3";
        case 5: return @"Outdoor";
    }
    return @"";
}


- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
