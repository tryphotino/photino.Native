#ifdef __APPLE__
#pragma once
#include "Photino.h"

@interface UiDelegate : NSObject <WKUIDelegate, WKScriptMessageHandler> {
    @public
    NSWindow * window;
    Photino * photino;
    WebMessageReceivedCallback webMessageReceivedCallback;
}
@end
#endif