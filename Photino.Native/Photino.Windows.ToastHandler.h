#ifndef TOASTHANDLER_H
#define TOASTHANDLER_H
#include "Photino.h"
#include "Dependencies/wintoastlib.h"
#include <WinUser.h>

using namespace WinToastLib;

class WinToastHandler : public IWinToastHandler
{
private:
    Photino* _window;

public:
    WinToastHandler(Photino* window)
    {
        this->_window = window;
    }

    void toastActivated() const
    {
        ShowWindow(this->_window->getHwnd(), SW_SHOW);    // Make the window visible if it was hidden
        ShowWindow(this->_window->getHwnd(), SW_RESTORE); // Next, restore it if it was minimized
        SetForegroundWindow(this->_window->getHwnd());    // Finally, activate the window
    }

    void toastActivated(int actionIndex) const
    {
        //
    }

    void toastDismissed(WinToastDismissalReason state) const
    {
        //
    }

    void toastFailed() const
    {
        //
    }
};
#endif