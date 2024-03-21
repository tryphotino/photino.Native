#ifdef __APPLE__
#pragma once
#include "Photino.h"

@interface WindowDelegate : NSObject <NSWindowDelegate>
{
    @public
        Photino * photino;
}
@end
#endif