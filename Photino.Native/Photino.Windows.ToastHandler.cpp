#ifdef _WIN32
#include "Photino.Windows.ToastHandler.h"
#include "Photino.Notification.h"
using namespace WinToastLib;

void PhotinoWinToastHandler::toastActivated() const 
{
	ShowWindow(this->_window->getHwnd(), SW_SHOW);
	ShowWindow(this->_window->getHwnd(), SW_RESTORE);
	SetForegroundWindow(this->_window->getHwnd());

	_notification->InvokeActivated();
}

void PhotinoWinToastHandler::toastActivated(int actionIndex) const 
{
	_notification->InvokeAction(actionIndex);
}

void PhotinoWinToastHandler::toastActivated(std::wstring response) const
{
	// Already implemented above
}

void PhotinoWinToastHandler::toastDismissed(WinToastDismissalReason state) const
{
	switch (state) 
	{
		case WinToastDismissalReason::UserCanceled:
			_notification->InvokeDismissed(PhotinoNotificationDismissalReason::UserCanceled);
			break;

		case WinToastDismissalReason::ApplicationHidden:
			_notification->InvokeDismissed(PhotinoNotificationDismissalReason::ApplicationHidden);
			break;

		case WinToastDismissalReason::TimedOut:
			_notification->InvokeDismissed(PhotinoNotificationDismissalReason::TimedOut);
			break;
	}
}

void PhotinoWinToastHandler::toastFailed() const 
{
	// Not implemented
}
#endif