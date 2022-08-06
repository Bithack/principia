//
//  RegisterController.h
//  principia
//
//  Created by Emil on 2013-12-13.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RegisterController : UIViewController

- (void)dismiss_with_success;
- (void)enable_inputs;

@end

extern RegisterController *register_dialog;