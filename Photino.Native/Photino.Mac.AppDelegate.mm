#ifdef __APPLE__
#import "Photino.Mac.AppDelegate.h"
#import <objc/runtime.h>

NSString * const PhotinoAppBundleID = @"com.photino.net";

@implementation NSBundle (FakeBundleIdentifier)

- (NSString *)__bundleIdentifier;
{
    return PhotinoAppBundleID;
}

@end

static BOOL InstallFakeBundleIdentifierHook()
{
  Class nsBundleClass = objc_getClass("NSBundle");
  if (nsBundleClass) {
    method_exchangeImplementations(class_getInstanceMethod(nsBundleClass, @selector(bundleIdentifier)),
                                   class_getInstanceMethod(nsBundleClass, @selector(__bundleIdentifier)));
    return YES;
  }
  return NO;
}

@implementation NSUserDefaults (SubscriptAndUnescape)
- (id)objectForKeyedSubscript:(id)key;
{
  id obj = [self objectForKey:key];
  if ([obj isKindOfClass:[NSString class]] && [(NSString *)obj hasPrefix:@"\\"]) {
    obj = [(NSString *)obj substringFromIndex:1];
  }
  return obj;
}
@end


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
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    
    @autoreleasepool {
        if (InstallFakeBundleIdentifierHook()) {
            NSLog(@"%@", defaults[@"sender"]);
        }
    }

    //NSLog(@"applicationDidFinishLaunching fired!");
    //NSAlert *alert = [[[NSAlert alloc] init] autorelease];
    //[alert setMessageText:@"Hi there."];
    //[alert runModal];
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