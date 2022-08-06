//
//  ImportController.h
//  principia
//
//  Created by Emil on 2013-12-16.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import <UIKit/UIKit.h>

extern int import_dialog_multiemit;

@interface ImportController : UIViewController <UITableViewDelegate, UITableViewDataSource> {
    IBOutlet UINavigationBar *navbar;
    
	struct lvlfile*                levels;
    int num_levels;
}

- (IBAction)done_click:(id)sender;
@end
