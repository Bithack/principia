//
//  DisplayController.h
//  principia
//
//  Created by Emil on 2013-12-11.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface DisplayController : UIViewController

- (IBAction)done_click:(id)sender;
- (IBAction)next_click:(id)sender;
- (IBAction)previous_click:(id)sender;
- (IBAction)insert_click:(id)sender;
- (IBAction)append_click:(id)sender;
- (IBAction)remove_click:(id)sender;

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event;

@end
