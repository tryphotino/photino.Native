#ifdef __APPLE__
#import "Photino.Mac.AppDelegate.h"
#import <objc/runtime.h>

@implementation AppDelegate : NSObject
- (id)init {
    if (self = [super init]) {
        // allocate and initialize window and stuff here ..
        //NSLog(@"init fired!");
    }
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    [window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    // NSLog(@"applicationDidFinishLaunching fired!");
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    //NSLog(@"applicationShouldTerminateAfterLastWindowClosed fired!");
    return true;
}

- (void)dealloc {
    [window release];
    [super dealloc];
}

@end
#endif