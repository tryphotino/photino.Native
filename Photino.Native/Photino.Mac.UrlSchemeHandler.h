#ifdef __APPLE__
#pragma once
#include "Photino.h"

@interface UrlSchemeHandler : NSObject <WKURLSchemeHandler> {
    @public
    WebResourceRequestedCallback requestHandler;
}
@end
#endif