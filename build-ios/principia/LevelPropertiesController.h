//
//  LevelPropertiesController.h
//  principia
//
//  Created by Emil on 2013-10-24.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "PropertiesInfoController.h"
#import "PropertiesWorldController.h"
#import "PropertiesGameplayController.h"

@interface LevelPropertiesController : UIViewController <UITabBarDelegate> {
    IBOutlet UITabBar *tabbar;
    IBOutlet UIScrollView *scrollview;
    
    NSArray *views;
    
    PropertiesInfoController *info;
    PropertiesWorldController *world;
    PropertiesGameplayController *gameplay;
}

- (IBAction)done_click:(id)sender;


@end
