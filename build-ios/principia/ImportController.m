//
//  ImportController.m
//  principia
//
//  Created by Emil on 2013-12-16.
//  Copyright (c) 2013 Bithack AB. All rights reserved.
//

#import "ImportController.h"
#include "pkgman.hh"
#include "ui.hh"
#include "main.hh"


int import_dialog_multiemit = 0;

@interface ImportController ()

@end

@implementation ImportController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    NSLog(@"Number of rows");
    return self->num_levels;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"Cell";
    
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:CellIdentifier] autorelease];
    }
    
    // Set the data for this cell:
    cell.textLabel.text = [[NSString alloc] initWithUTF8String:self->levels[indexPath.row].name];
    cell.detailTextLabel.text = [[NSString alloc] initWithUTF8String:self->levels[indexPath.row].modified_date];
    
    // set the accessory view:
    cell.accessoryType = UITableViewCellAccessoryNone;
    
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    
    int selectedRow = indexPath.row;
    
    uint32_t lvl_id = self->levels[indexPath.row].id;
    
    if (import_dialog_multiemit)
        P_add_action(ACTION_MULTIEMITTER_SET, lvl_id);
    else
        P_add_action(ACTION_IMPORT_OBJECT, lvl_id);
    
    [self dismissModalViewControllerAnimated:YES];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    struct lvlfile *l = pkgman_get_levels(LEVEL_PARTIAL), *lp;

    self->num_levels = 0;

    lp = l;
    while (lp) {
        self->num_levels ++;
        lp = lp->next;
    }
    
    if (self->num_levels)
        self->levels = malloc(self->num_levels * sizeof(struct lvlfile));
    
    int x=0;
    while (l) {
        self->levels[x] = *l;
        x++;
        l = l->next;
    }
}


- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}

- (IBAction)done_click:(id)sender
{
    [self dismissModalViewControllerAnimated:YES];
}

@end
