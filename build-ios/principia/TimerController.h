//
//  TimerController.h
//  principia
//
//  Created by Emil on 2013-12-13.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface TimerController: UIViewController <UIPickerViewDataSource, UIPickerViewDelegate> {
    
    IBOutlet UIPickerView*           inputPicker;
}


@end
