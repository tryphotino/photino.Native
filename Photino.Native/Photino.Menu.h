#pragma once

#ifdef __APPLE__
#include "Photino.Mac.Menu.h"
#elif __linux__
#include "Photino.Linux.Menu.h"
#else
#include "Photino.Windows.Menu.h"
#endif