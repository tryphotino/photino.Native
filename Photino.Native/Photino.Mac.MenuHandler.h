#pragma once

#ifdef __APPLE__

#import <Cocoa/Cocoa.h>

class Photino;

@interface MenuActionHandler : NSObject {
    @public
    Photino *photino;
    char *command;
}

- (void)menuItemClicked:(id)sender;
- (void)dealloc;

@end

#endif
