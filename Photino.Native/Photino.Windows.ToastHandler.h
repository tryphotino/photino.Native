#ifdef _WIN32
#ifndef TOASTHANDLER_H
#define TOASTHANDLER_H

#include "Photino.h"
#include <vector>
#include "Dependencies/wintoastlib.h"
#include <WinUser.h>

using namespace WinToastLib;

class PhotinoNotification;

class PhotinoWinToastHandler : public WinToastLib::IWinToastHandler
{
private:
    Photino* _window{};
    PhotinoNotification* _notification{};

public:
    explicit PhotinoWinToastHandler(Photino* window, PhotinoNotification* notification)
    {
        _window = window;
        _notification = notification;
    }

    void toastActivated() const override;
    void toastActivated(int actionIndex) const override;
    void toastActivated(std::wstring response) const override;
    void toastDismissed(WinToastDismissalReason state) const override;
    void toastFailed() const override;
};
#endif
#endif