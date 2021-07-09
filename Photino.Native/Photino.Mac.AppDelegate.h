#ifdef __APPLE__
#pragma once
#include <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate> {
    NSWindow * window;
}
@end
#endif
