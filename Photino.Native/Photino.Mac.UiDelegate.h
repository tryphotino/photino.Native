#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#include "Photino.h"

typedef void (*WebMessageReceivedCallback) (char* message);

@interface MyUiDelegate : NSObject <WKUIDelegate, WKScriptMessageHandler> {
    @public
    NSWindow * window;
    WebWindow * webWindow;
    WebMessageReceivedCallback webMessageReceivedCallback;
}
@end
