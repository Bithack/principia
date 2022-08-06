//
//  FXEmitterController.h
//  principia
//
//  Created by Emil on 2013-12-09.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface FXEmitterController : UIViewController <UIPickerViewDataSource, UIPickerViewDelegate> {
    
    IBOutlet UIPickerView*           inputPicker;
}

@property(nonatomic, assign) NSArray *pickervalues;

- (IBAction)done_click:(id)sender;


- (NSInteger)getSelected;
- (NSInteger)getSelected_2;
- (NSInteger)getSelected_3;

@end
