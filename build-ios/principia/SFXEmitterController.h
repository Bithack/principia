//
//  SFXEmitterController.h
//  principia
//
//  Created by Emil on 2013-12-09.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface SFXEmitterController : UIViewController <UIPickerViewDataSource, UIPickerViewDelegate> {
    
    IBOutlet UIPickerView*           inputPicker;
}

@property(nonatomic, assign) NSMutableArray *pickervalues;
- (IBAction)done_click:(id)sender;

@end
