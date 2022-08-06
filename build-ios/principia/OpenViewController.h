//
//  CommandPadViewController.h
//  principia
//
//  Created by Emil on 2013-10-21.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import <UIKit/UIKit.h>
#include "pkgman.hh"

@interface OpenViewController : UIViewController <UITableViewDelegate, UITableViewDataSource> {
    IBOutlet UINavigationBar *navbar;
    
	struct lvlfile*                levels;
    int num_levels;
}

- (IBAction)done_click:(id)sender;

@end
