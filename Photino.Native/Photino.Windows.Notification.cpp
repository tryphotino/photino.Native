#include "Photino.Notification.h"

#ifdef _WIN32
#include "Photino.Windows.ToastHandler.h"
#endif

WinToastLib::WinToastTemplate _toast;

PhotinoNotification::PhotinoNotification(Photino* window)
{
	_window = window;
	_handler = new PhotinoWinToastHandler(_window, this);
}

PhotinoNotification::~PhotinoNotification() 
{
	delete _handler;
}

std::vector<AutoString> PhotinoNotification::GetActions() const
{
	return _actionNames;
}

void PhotinoNotification::SetType(PhotinoNotificationType type)
{
	_type = type;
	
	WinToastLib::WinToastTemplate::WinToastTemplateType winToastType;

	switch (_type)
	{
		case PhotinoNotificationType::ImageAndText01:
			winToastType = WinToastLib::WinToastTemplate::ImageAndText01;
			break;

		case PhotinoNotificationType::ImageAndText02:
			winToastType = WinToastLib::WinToastTemplate::ImageAndText02;
			break;

		case PhotinoNotificationType::ImageAndText03:
			winToastType = WinToastLib::WinToastTemplate::ImageAndText03;
			break;

		case PhotinoNotificationType::ImageAndText04:
			winToastType = WinToastLib::WinToastTemplate::ImageAndText04;
			break;

		case PhotinoNotificationType::Text01:
			winToastType = WinToastLib::WinToastTemplate::Text01;
			break;

		case PhotinoNotificationType::Text02:
			winToastType = WinToastLib::WinToastTemplate::Text02;
			break;

		case PhotinoNotificationType::Text03:
			winToastType = WinToastLib::WinToastTemplate::Text03;
			break;

		case PhotinoNotificationType::Text04:
			winToastType = WinToastLib::WinToastTemplate::Text04;
			break;

		default:
			winToastType = WinToastLib::WinToastTemplate::Text01;
			break;
	}

	_toast = WinToastLib::WinToastTemplate(winToastType);
}

void PhotinoNotification::AddText(AutoString text)
{
	AutoString conv = _window->ToUTF16String(text);

	_textLines.push_back(conv);

	switch (_textLines.size())
	{
		case 1:
			_toast.setTextField(conv, WinToastLib::WinToastTemplate::FirstLine);
			break;

		case 2:
			_toast.setTextField(conv, WinToastLib::WinToastTemplate::SecondLine);
			break;

		case 3:
			_toast.setTextField(conv, WinToastLib::WinToastTemplate::ThirdLine);
			break;

		case 4:
			_toast.setAttributionText(conv);
			break;
	}
}

void PhotinoNotification::AddAction(AutoString action)
{
	_actionNames.push_back(action);
	_toast.addAction(_window->ToUTF16String(action));
}

void PhotinoNotification::SetImagePath(AutoString imagePath)
{
	_imagePath = imagePath;
	_toast.setImagePath(_window->ToUTF16String(imagePath));
}

void PhotinoNotification::Show()
{
	WinToastLib::WinToast::instance()->showToast(_toast, _handler);
}