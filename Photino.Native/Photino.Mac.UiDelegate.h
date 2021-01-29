#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#include "Photino.h"

typedef void (*WebMessageReceivedCallback) (char* message);

@interface MyUiDelegate : NSObject <WKUIDelegate, WKScriptMessageHandler> {
    @public
    NSWindow * window;
    Photino * photino;
    WebMessageReceivedCallback webMessageReceivedCallback;
}
@end
