#ifdef __APPLE__
#import "Photino.Mac.NSWindowBorderless.h"

@implementation NSWindowBorderless : NSWindow
- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}
@end
#endif