#ifdef __APPLE__
#pragma once
#include "Photino.h"

@interface NavigationDelegate: NSObject<WKNavigationDelegate>{
    @public
    NSWindow * window;
    Photino * photino;
}
@end
#endif