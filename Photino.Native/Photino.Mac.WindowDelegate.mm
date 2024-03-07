#ifdef __APPLE__
#import "Photino.Mac.WindowDelegate.h"

@implementation WindowDelegate : NSObject
- (void)windowDidResize:(NSNotification *)notification {
    int width, height;
    photino->GetSize(&width, &height);
    photino->InvokeResize(width, height);
}  

- (void)windowDidMove:(NSNotification *)notification {
    int x, y;
    photino->GetPosition(&x, &y);
    photino->InvokeMove(x, y);
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
    photino->InvokeFocusIn();
}

- (void)windowDidResignKey:(NSNotification *)notification {
    photino->InvokeFocusOut();
}

- (void)windowDidMiniaturize:(NSNotification *)notification {
    photino->InvokeMinimized();
}

- (void)windowDidDeminiaturize:(NSNotification *)notification {
    photino->InvokeRestored();
}

- (void)windowWillClose: (NSWindow *)sender
{
    photino->InvokeClose();
}
@end

#endif