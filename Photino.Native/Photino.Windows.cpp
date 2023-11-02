#include "Photino.h"
#include "Photino.Dialog.h"
#include "Photino.Windows.DarkMode.h"
#include "Photino.Windows.ToastHandler.h"

#include <mutex>
#include <condition_variable>
#include <comdef.h>
#include <Shlwapi.h>
#include <wrl.h>
#include <windows.h>
#include <algorithm>
#include <limits>
#include <WebView2EnvironmentOptions.h>

#pragma comment(lib, "Urlmon.lib")
#pragma warning(disable: 4996)		//disable warning about wcscpy vs. wcscpy_s

#define WM_USER_INVOKE (WM_USER + 0x0002)

using namespace WinToastLib;
using namespace Microsoft::WRL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LPCWSTR CLASS_NAME = L"Photino";
std::mutex invokeLockMutex;
HINSTANCE Photino::_hInstance;
HWND messageLoopRootWindowHandle;
std::map<HWND, Photino*> hwndToPhotino;
wchar_t _webview2RuntimePath[MAX_PATH];


struct InvokeWaitInfo
{
	std::condition_variable completionNotifier;
	bool isCompleted;
};

struct ShowMessageParams
{
	std::wstring title;
	std::wstring body;
	UINT type = 0;
};


void Photino::Register(HINSTANCE hInstance)
{
	InitDarkModeSupport();

	_hInstance = hInstance;

	// Register the window class
	WNDCLASSEX wcx;
	wcx.cbSize = sizeof WNDCLASSEX;
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = WindowProc;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = hInstance;
	wcx.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcx.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcx.lpszMenuName = nullptr;
	wcx.lpszClassName = CLASS_NAME;
	wcx.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);

	RegisterClassEx(&wcx);

	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
}


Photino::Photino(PhotinoInitParams* initParams)
{
	//wchar_t msg[50];
	//swprintf(msg, 50, L"Size: %i", initParams->Size);
	//MessageBox(nullptr, msg, L"", MB_OK);

	//wchar_t msg[50];
	//swprintf(msg, 50, L"MaxWidth: %i", initParams->MaxWidth);
	//MessageBox(nullptr, msg, L"", MB_OK);

	if (initParams->Size != sizeof(PhotinoInitParams))
	{
		wchar_t msg[200];
		swprintf(msg, 200, L"Initial parameters passed are %i bytes, but expected %I64i bytes.", initParams->Size, sizeof(PhotinoInitParams));
		MessageBox(nullptr, msg, L"Native Initialization Failed", MB_OK);
		exit(0);
	}

	_windowTitle = new wchar_t[256];

	if (initParams->TitleWide != NULL)
	{
		WinToast::instance()->setAppName(initParams->TitleWide);
		WinToast::instance()->setAppUserModelId(initParams->TitleWide);
		wcscpy(_windowTitle, initParams->TitleWide);
	}
	else
		_windowTitle[0] = 0;

	_startUrl = NULL;
	if (initParams->StartUrlWide != NULL)
	{
		_startUrl = new wchar_t[2048];
		if (_startUrl == NULL) exit(0);
		wcscpy(_startUrl, initParams->StartUrlWide);
	}

	_startString = NULL;
	if (initParams->StartStringWide != NULL)
	{
		_startString = new wchar_t[wcslen(initParams->StartStringWide) + 1];
		if (_startString == NULL) exit(0);
		wcscpy(_startString, initParams->StartStringWide);
	}

	_temporaryFilesPath = NULL;
	if (initParams->TemporaryFilesPathWide != NULL)
	{
		_temporaryFilesPath = new wchar_t[256];
		if (_temporaryFilesPath == NULL) exit(0);
		wcscpy(_temporaryFilesPath, initParams->TemporaryFilesPathWide);

	}

	_userAgent = NULL;
	if (initParams->UserAgentWide != NULL)
	{
		_userAgent = new wchar_t[wcslen(initParams->UserAgentWide) + 1];
		if (_userAgent == NULL) exit(0);
		wcscpy(_userAgent, initParams->UserAgentWide);
	}

	_browserControlInitParameters = NULL;
	if (initParams->BrowserControlInitParametersWide != NULL)
	{
		_browserControlInitParameters = new wchar_t[wcslen(initParams->BrowserControlInitParametersWide) + 1];
		if (_browserControlInitParameters == NULL) exit(0);
		wcscpy(_browserControlInitParameters, initParams->BrowserControlInitParametersWide);
	}


	_contextMenuEnabled = initParams->ContextMenuEnabled;
	_devToolsEnabled = initParams->DevToolsEnabled;
	_grantBrowserPermissions = initParams->GrantBrowserPermissions;
	_mediaAutoplayEnabled = initParams->MediaAutoplayEnabled;
	_fileSystemAccessEnabled = initParams->FileSystemAccessEnabled;
	_webSecurityEnabled = initParams->WebSecurityEnabled;
	_javascriptClipboardAccessEnabled = initParams->JavascriptClipboardAccessEnabled;
	_mediaStreamEnabled = initParams->MediaStreamEnabled;
	_smoothScrollingEnabled = initParams->SmoothScrollingEnabled;

	_zoom = initParams->Zoom;
	_minWidth = initParams->MinWidth;
	_minHeight = initParams->MinHeight;
	_maxWidth = initParams->MaxWidth;
	_maxHeight = initParams->MaxHeight;

	//these handlers are ALWAYS hooked up
	_webMessageReceivedCallback = (WebMessageReceivedCallback)initParams->WebMessageReceivedHandler;
	_resizedCallback = (ResizedCallback)initParams->ResizedHandler;
	_maximizedCallback = (MaximizedCallback)initParams->MaximizedHandler;
	_restoredCallback = (RestoredCallback)initParams->RestoredHandler;
	_minimizedCallback = (MinimizedCallback)initParams->MinimizedHandler;
	_movedCallback = (MovedCallback)initParams->MovedHandler;
	_closingCallback = (ClosingCallback)initParams->ClosingHandler;
	_focusInCallback = (FocusInCallback)initParams->FocusInHandler;
	_focusOutCallback = (FocusOutCallback)initParams->FocusOutHandler;
	_customSchemeCallback = (WebResourceRequestedCallback)initParams->CustomSchemeHandler;

	//copy strings from the fixed size array passed, but only if they have a value.
	for (int i = 0; i < 16; ++i)
	{
		if (initParams->CustomSchemeNamesWide[i] != NULL)
		{
			wchar_t* name = new wchar_t[50];
			wcscpy(name, initParams->CustomSchemeNamesWide[i]);
			_customSchemeNames.push_back(name);
		}
	}

	_parent = initParams->ParentInstance;

	//wchar_t msg[50];
	//swprintf(msg, 50, L"Height: %i  Width: %i  Left: %d  Top: %d", initParams->Height, initParams->Width, initParams->Left, initParams->Top);
	//MessageBox(nullptr, msg, L"", MB_OK);


	if (initParams->UseOsDefaultSize)
	{
		initParams->Width = CW_USEDEFAULT;
		initParams->Height = CW_USEDEFAULT;
	}
	else
	{
		if (initParams->Width < 0) initParams->Width = CW_USEDEFAULT;
		if (initParams->Height < 0) initParams->Height = CW_USEDEFAULT;
	}

	if (initParams->UseOsDefaultLocation)
	{
		initParams->Left = CW_USEDEFAULT;
		initParams->Top = CW_USEDEFAULT;
	}

	if (initParams->FullScreen == true)
	{
		initParams->Left = 0;
		initParams->Top = 0;
		initParams->Width = GetSystemMetrics(SM_CXSCREEN);
		initParams->Height = GetSystemMetrics(SM_CYSCREEN);
	}

	if (initParams->Chromeless)
	{
		//CW_USEDEFAULT CAN NOT BE USED ON POPUP WINDOWS
		if (initParams->Left == CW_USEDEFAULT && initParams->Top == CW_USEDEFAULT) initParams->CenterOnInitialize = true;
		if (initParams->Left == CW_USEDEFAULT) initParams->Left = 0;
		if (initParams->Top == CW_USEDEFAULT) initParams->Top = 0;
		if (initParams->Height == CW_USEDEFAULT) initParams->Height = 600;
		if (initParams->Width == CW_USEDEFAULT) initParams->Width = 800;
	}

	if (initParams->Height > initParams->MaxHeight) initParams->Height = initParams->MaxHeight;
	if (initParams->Height < initParams->MinHeight) initParams->Height = initParams->MinHeight;
	if (initParams->Width > initParams->MaxWidth) initParams->Width = initParams->MaxWidth;
	if (initParams->Width < initParams->MinWidth) initParams->Width = initParams->MinWidth;

	//Create the window
	_hWnd = CreateWindowEx(
		0, //WS_EX_OVERLAPPEDWINDOW, //An optional extended window style.
		CLASS_NAME,             //Window class
		initParams->TitleWide,		//Window text
		initParams->Chromeless || initParams->FullScreen ? WS_POPUP : WS_OVERLAPPEDWINDOW,	//Window style

		// Size and position
		initParams->Left, initParams->Top, initParams->Width, initParams->Height,

		nullptr,    //Parent window handle
		nullptr,    //Menu
		_hInstance, //Instance handle
		this        //Additional application data
	);
	hwndToPhotino[_hWnd] = this;

	if (initParams->WindowIconFileWide != NULL && initParams->WindowIconFileWide != L"")
		Photino::SetIconFile(initParams->WindowIconFileWide);

	if (initParams->CenterOnInitialize)
		Photino::Center();

	if (initParams->Minimized)
		SetMinimized(true);

	if (initParams->Maximized)
		SetMaximized(true);

	if (initParams->Resizable == false)
		SetResizable(false);

	if (initParams->Topmost)
		SetTopmost(true);

	this->_toastHandler = new WinToastHandler(this);
	WinToast::instance()->initialize();
	_dialog = new PhotinoDialog(this);
	Photino::Show();
}

Photino::~Photino()
{
	if (_startUrl != NULL) delete[]_startUrl;
	if (_startString != NULL) delete[]_startString;
	if (_temporaryFilesPath != NULL) delete[]_temporaryFilesPath;
	if (_windowTitle != NULL) delete[]_windowTitle;
	if (_toastHandler != NULL) delete _toastHandler;
}

HWND Photino::getHwnd()
{
	return _hWnd;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE: 
	{
		EnableDarkMode(hwnd, true);
		if (IsDarkModeEnabled()) 
		{
			RefreshNonClientArea(hwnd);
		}
		break;
	}
	case WM_SETTINGCHANGE: 
	{
		if (IsColorSchemeChange(lParam))
			SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
		break;
	}
	case WM_THEMECHANGED:
	{
		EnableDarkMode(hwnd, IsDarkModeEnabled());
		RefreshNonClientArea(hwnd);
		break;
	}
	case WM_ACTIVATE:
	{
		Photino* Photino = hwndToPhotino[hwnd];
		if (LOWORD(wParam) == WA_INACTIVE) 
		{
			Photino->InvokeFocusOut();
		}
		else 
		{
			Photino->FocusWebView2();
			Photino->InvokeFocusIn();

			return 0;
		}
		break;
	}
	case WM_CLOSE:
	{
		Photino* Photino = hwndToPhotino[hwnd];
		if (Photino)
		{
			bool doNotClose = Photino->InvokeClose();

			if (!doNotClose)
				DestroyWindow(hwnd);
		}

		return 0;
	}
	case WM_DESTROY:
	{
		// Only terminate the message loop if the window being closed is the one that
		// started the message loop
		hwndToPhotino.erase(hwnd);
		if (hwnd == messageLoopRootWindowHandle)
			PostQuitMessage(0);

		return 0;
	}
	case WM_USER_INVOKE:
	{
		ACTION callback = (ACTION)wParam;
		callback();
		InvokeWaitInfo* waitInfo = (InvokeWaitInfo*)lParam;
		{
			std::lock_guard<std::mutex> guard(invokeLockMutex);
			waitInfo->isCompleted = true;
		}
		waitInfo->completionNotifier.notify_one();
		//delete waitInfo; ?
		return 0;
	}
	case WM_GETMINMAXINFO:
	{
		Photino* Photino = hwndToPhotino[hwnd];
		if (Photino == NULL)
			return 0;

		MINMAXINFO* mmi = (MINMAXINFO*)lParam;
		if (Photino->_minWidth > 0)
			mmi->ptMinTrackSize.x = Photino->_minWidth;
		if (Photino->_minHeight > 0)
			mmi->ptMinTrackSize.y = Photino->_minHeight;	
		if (Photino->_maxWidth < INT_MAX)
			mmi->ptMaxTrackSize.x = Photino->_maxWidth;
		if (Photino->_maxHeight < INT_MAX)
			mmi->ptMaxTrackSize.y = Photino->_maxHeight;
		return 0;
	}
	case WM_SIZE:
	{
		Photino* Photino = hwndToPhotino[hwnd];
		if (Photino)
		{
			Photino->RefitContent();
			int width, height;
			Photino->GetSize(&width, &height);
			Photino->InvokeResize(width, height);

			if (LOWORD(wParam) == SIZE_MAXIMIZED) {
				Photino->InvokeMaximized();
			}
			else if (LOWORD(wParam) == SIZE_RESTORED) {
				Photino->InvokeRestored();
			}
			else if (LOWORD(wParam) == SIZE_MINIMIZED) {
				Photino->InvokeMinimized();
			}
		}
		return 0;
	}
	case WM_MOVE:
	{
		Photino* Photino = hwndToPhotino[hwnd];
		if (Photino)
		{
			int x, y;
			Photino->GetPosition(&x, &y);
			Photino->InvokeMove(x, y);
		}
		return 0;
	}
	break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}





void Photino::Center()
{
	int screenDpi = GetDpiForWindow(_hWnd);
	int screenHeight = GetSystemMetricsForDpi(SM_CYSCREEN, screenDpi);
	int screenWidth = GetSystemMetricsForDpi(SM_CXSCREEN, screenDpi);

	RECT windowRect = {};
	GetWindowRect(_hWnd, &windowRect);
	int windowHeight = windowRect.bottom - windowRect.top;
	int windowWidth = windowRect.right - windowRect.left;

	int left = (screenWidth / 2) - (windowWidth / 2);
	int top = (screenHeight / 2) - (windowHeight / 2);

	//wchar_t msg[500];
	//swprintf(msg, 500, L"Screen DPI: %i  Screen Height: %i  ScreenWidth: %i  Window Height: %i  Window Width: %i  Left: %d  Top: %d", screenDpi, screenHeight, screenWidth, windowHeight, windowWidth, left, top);
	//MessageBox(nullptr, msg, L"", MB_OK);

	SetPosition(left, top);
}

void Photino::Close()
{
	PostMessage(_hWnd, WM_CLOSE, NULL, NULL);
}



void Photino::GetContextMenuEnabled(bool* enabled)
{
	ICoreWebView2Settings* settings;
	HRESULT r = _webviewWindow->get_Settings(&settings);
	settings->get_AreDefaultContextMenusEnabled((BOOL*)enabled);
}

void Photino::GetDevToolsEnabled(bool* enabled)
{
	ICoreWebView2Settings* settings;
	HRESULT r = _webviewWindow->get_Settings(&settings);
	settings->get_AreDevToolsEnabled((BOOL*)enabled);
}

void Photino::GetFullScreen(bool* fullScreen)
{
	LONG lStyles = GetWindowLong(_hWnd, GWL_STYLE);
	if (lStyles & WS_POPUP) *fullScreen = true;
}

void Photino::GetGrantBrowserPermissions(bool* grant)
{
	*grant = _grantBrowserPermissions;
}

AutoString Photino::GetUserAgent()
{
	return this->_userAgent;
}

void Photino::GetMediaAutoplayEnabled(bool* enabled)
{
	*enabled = this->_mediaAutoplayEnabled;
}

void Photino::GetFileSystemAccessEnabled(bool* enabled)
{
	*enabled = this->_fileSystemAccessEnabled;
}

void Photino::GetWebSecurityEnabled(bool* enabled)
{
	*enabled = this->_webSecurityEnabled;
}

void Photino::GetJavascriptClipboardAccessEnabled(bool* enabled)
{
	*enabled = this->_javascriptClipboardAccessEnabled;
}

void Photino::GetMediaStreamEnabled(bool* enabled)
{
	*enabled = this->_mediaStreamEnabled;
}

void Photino::GetSmoothScrollingEnabled(bool* enabled)
{
	*enabled = this->_smoothScrollingEnabled;
}

AutoString Photino::GetIconFileName()
{
	return this->_iconFileName;
}

void Photino::GetMaximized(bool* isMaximized)
{
	LONG lStyles = GetWindowLong(_hWnd, GWL_STYLE);
	if (lStyles & WS_MAXIMIZE) *isMaximized = true;
}

void Photino::GetMinimized(bool* isMinimized)
{
	LONG lStyles = GetWindowLong(_hWnd, GWL_STYLE);
	if (lStyles & WS_MINIMIZE) *isMinimized = true;
}

void Photino::GetPosition(int* x, int* y)
{
	RECT rect = {};
	GetWindowRect(_hWnd, &rect);
	if (x) *x = rect.left;
	if (y) *y = rect.top;
}

void Photino::GetResizable(bool* resizable)
{
	LONG lStyles = GetWindowLong(_hWnd, GWL_STYLE);
	if (lStyles & WS_THICKFRAME) *resizable = true;
}

unsigned int Photino::GetScreenDpi()
{
	return GetDpiForWindow(_hWnd);
}

void Photino::GetSize(int* width, int* height)
{
	RECT rect = {};
	GetWindowRect(_hWnd, &rect);
	if (width) *width = rect.right - rect.left;
	if (height) *height = rect.bottom - rect.top;
}

AutoString Photino::GetTitle()
{
	//int titleLength = GetWindowTextLength(_hWnd) + 1;
	//wchar_t* title = new wchar_t[titleLength];
	//GetWindowText(_hWnd, title, titleLength);
	//MessageBox(nullptr, title, L"", MB_OK);
	return _windowTitle;
}

void Photino::GetTopmost(bool* topmost)
{
	LONG lStyles = GetWindowLong(_hWnd, GWL_STYLE);
	if (lStyles & WS_EX_TOPMOST) *topmost = true;
}

void Photino::GetZoom(int* zoom)
{
	double rawValue = 0;
	_webviewController->get_ZoomFactor(&rawValue);
	rawValue = (rawValue * 100.0) + 0.5;		//account for rounding issues
	*zoom = (int)rawValue;
}



void Photino::NavigateToString(AutoString content)
{
	_webviewWindow->NavigateToString(content);
}

void Photino::NavigateToUrl(AutoString url)
{
	_webviewWindow->Navigate(url);
}

void Photino::Restore()
{
	ShowWindow(_hWnd, SW_RESTORE);
}

void Photino::SendWebMessage(AutoString message)
{
	_webviewWindow->PostWebMessageAsString(message);
}


void Photino::SetContextMenuEnabled(bool enabled)
{
	ICoreWebView2Settings* settings;
	HRESULT r = _webviewWindow->get_Settings(&settings);
	settings->put_AreDefaultContextMenusEnabled(enabled);
	_webviewWindow->Reload();
}

void Photino::SetDevToolsEnabled(bool enabled)
{
	ICoreWebView2Settings* settings;
	HRESULT r = _webviewWindow->get_Settings(&settings);
	settings->put_AreDevToolsEnabled(enabled);
	_webviewWindow->Reload();
}

void Photino::SetFullScreen(bool fullScreen)
{
	LONG_PTR style = GetWindowLongPtr(_hWnd, GWL_STYLE);
	if (fullScreen)
	{
		style |= WS_POPUP;
		style &= (~WS_OVERLAPPEDWINDOW);
		SetPosition(0, 0);
		SetSize(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	}
	else
	{
		style |= WS_OVERLAPPEDWINDOW;
		style &= (~WS_POPUP);
	}
	SetWindowLongPtr(_hWnd, GWL_STYLE, style);
}

void Photino::SetIconFile(AutoString filename)
{
	HICON iconSmall = (HICON)LoadImage(NULL, filename, IMAGE_ICON, 16, 16, LR_LOADFROMFILE | LR_LOADTRANSPARENT | LR_SHARED);
	HICON iconBig = (HICON)LoadImage(NULL, filename, IMAGE_ICON, 32, 32, LR_LOADFROMFILE | LR_LOADTRANSPARENT | LR_SHARED);

	if (iconSmall && iconBig)
	{
		SendMessage(_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)iconSmall);
		SendMessage(_hWnd, WM_SETICON, ICON_BIG, (LPARAM)iconBig);
	}

	this->_iconFileName = filename;
}

void Photino::SetMinimized(bool minimized)
{
	if (minimized)
		ShowWindow(_hWnd, SW_MINIMIZE);
	else
		ShowWindow(_hWnd, SW_NORMAL);
}

void Photino::SetMinSize(int width, int height)
{
	_minWidth = width;
	_minHeight = height;

	int currWidth, currHeight;
	GetSize(&currWidth, &currHeight);
	if (currWidth < _minWidth)
		SetSize(_minWidth, currHeight);
	if (currHeight < _minHeight)
		SetSize(currWidth, _minHeight);
}

void Photino::SetMaximized(bool maximized)
{
	if (maximized)
		ShowWindow(_hWnd, SW_MAXIMIZE);
	else
		ShowWindow(_hWnd, SW_NORMAL);
}

void Photino::SetMaxSize(int width, int height)
{
	_maxWidth = width;
	_maxHeight = height;

	int currWidth, currHeight;
	GetSize(&currWidth, &currHeight);
	if (currWidth > _maxWidth)
		SetSize(_maxWidth, currHeight);
	if (currWidth > _maxHeight)
		SetSize(currWidth, _maxHeight);
}

void Photino::SetPosition(int x, int y)
{
	SetWindowPos(_hWnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void Photino::SetResizable(bool resizable)
{
	LONG_PTR style = GetWindowLongPtr(_hWnd, GWL_STYLE);
	if (resizable) style |= WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	else style &= (~WS_THICKFRAME) & (~WS_MINIMIZEBOX) & (~WS_MAXIMIZEBOX);
	SetWindowLongPtr(_hWnd, GWL_STYLE, style);
}

void Photino::SetSize(int width, int height)
{
	SetWindowPos(_hWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

void Photino::SetTitle(AutoString title)
{
	if (wcslen(title) > 255)
	{
		for (int i = 0; i < 256; i++)
			_windowTitle[i] = title[i];
		_windowTitle[255] = 0;
	}
	else
		wcscpy(_windowTitle, title);
	SetWindowText(_hWnd, title);
	WinToast::instance()->setAppName(title);
	WinToast::instance()->setAppUserModelId(title);
}

void Photino::SetTopmost(bool topmost)
{
	LONG_PTR style = GetWindowLongPtr(_hWnd, GWL_STYLE);
	if (topmost) style |= WS_EX_TOPMOST;
	else style &= (~WS_EX_TOPMOST);
	SetWindowLongPtr(_hWnd, GWL_STYLE, style);
	SetWindowPos(_hWnd, topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void Photino::SetZoom(int zoom)
{
	double newZoom = zoom / 100.0;
	HRESULT r = _webviewController->put_ZoomFactor(newZoom);
	//wchar_t msg[50];
	//swprintf(msg, 50, L"newZoom: %f", newZoom);
	//MessageBox(nullptr, msg, L"Setter", MB_OK);
}



void Photino::ShowNotification(AutoString title, AutoString body)
{
	if (WinToast::isCompatible())
	{
		WinToastTemplate toast = WinToastTemplate(WinToastTemplate::ImageAndText02);
		toast.setTextField(title, WinToastTemplate::FirstLine);
		toast.setTextField(body, WinToastTemplate::SecondLine);
		toast.setImagePath(this->_iconFileName);
		WinToast::instance()->showToast(toast, _toastHandler);
	}
}

void Photino::WaitForExit()
{
	messageLoopRootWindowHandle = _hWnd;

	// Run the message loop
	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}






//Callbacks
BOOL MonitorEnum(HMONITOR monitor, HDC, LPRECT, LPARAM arg)
{
	auto callback = (GetAllMonitorsCallback)arg;
	MONITORINFO info = {};
	info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(monitor, &info);
	Monitor props = {};
	props.monitor.x = info.rcMonitor.left;
	props.monitor.y = info.rcMonitor.top;
	props.monitor.width = info.rcMonitor.right - info.rcMonitor.left;
	props.monitor.height = info.rcMonitor.bottom - info.rcMonitor.top;
	props.work.x = info.rcWork.left;
	props.work.y = info.rcWork.top;
	props.work.width = info.rcWork.right - info.rcWork.left;
	props.work.height = info.rcWork.bottom - info.rcWork.top;
	return callback(&props) ? TRUE : FALSE;
}

void Photino::GetAllMonitors(GetAllMonitorsCallback callback)
{
	if (callback)
	{
		EnumDisplayMonitors(NULL, NULL, (MONITORENUMPROC) MonitorEnum, (LPARAM)callback);
	}
}

void Photino::Invoke(ACTION callback)
{
	InvokeWaitInfo waitInfo = {};
	PostMessage(_hWnd, WM_USER_INVOKE, (WPARAM)callback, (LPARAM)&waitInfo);

	// Block until the callback is actually executed and completed
	// TODO: Add return values, exception handling, etc.
	std::unique_lock<std::mutex> uLock(invokeLockMutex);
	waitInfo.completionNotifier.wait(uLock, [&] { return waitInfo.isCompleted; });
}





//private methods

void Photino::AttachWebView()
{
	size_t runtimePathLen = wcsnlen(_webview2RuntimePath, _countof(_webview2RuntimePath));
	PCWSTR runtimePath = runtimePathLen > 0 ? &_webview2RuntimePath[0] : nullptr;

	//TODO: Implement special startup strings.
	//https://peter.sh/experiments/chromium-command-line-switches/
	//https://learn.microsoft.com/en-us/dotnet/api/microsoft.web.webview2.core.corewebview2environmentoptions.additionalbrowserarguments?view=webview2-dotnet-1.0.1938.49&viewFallbackFrom=webview2-dotnet-1.0.1901.177view%3Dwebview2-1.0.1901.177
	//https://www.chromium.org/developers/how-tos/run-chromium-with-flags/
	//Add together all 7 special startup strings, plus the generic one passed by the user to make one big string. Try not to duplicate anything. Separate with spaces.
	
	std::wstring startupString = L"";
	if (_userAgent != NULL && wcslen(_userAgent) > 0)
		startupString += L"--user-agent=\"" + std::wstring(_userAgent) + L"\" ";
	if (_mediaAutoplayEnabled) 
		startupString += L"--autoplay-policy=no-user-gesture-required ";
	if (_fileSystemAccessEnabled) 
		startupString += L"--allow-file-access-from-files ";
	if (!_webSecurityEnabled)
		startupString += L"--disable-web-security ";
	if (_javascriptClipboardAccessEnabled)
		startupString += L"--enable-javascript-clipboard-access ";
	if (_mediaStreamEnabled)
		startupString += L"--enable-usermedia-screen-capturing ";
	if (!_smoothScrollingEnabled)
		startupString += L"--disable-smooth-scrolling ";
	if (_browserControlInitParameters != NULL)
		startupString += _browserControlInitParameters;	//e.g.--hide-scrollbars

	auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
	if (startupString.length() > 0)
		options->put_AdditionalBrowserArguments(startupString.c_str());

	HRESULT envResult = CreateCoreWebView2EnvironmentWithOptions(runtimePath, _temporaryFilesPath, options.Get(),
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[&](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
				if (result != S_OK) { return result; }
				HRESULT envResult = env->QueryInterface(&_webviewEnvironment);
				if (envResult != S_OK) { return envResult; }

				env->CreateCoreWebView2Controller(_hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					[&](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {

						if (result != S_OK) { return result; }

						HRESULT envResult = controller->QueryInterface(&_webviewController);
						if (envResult != S_OK) { return envResult; }
						_webviewController->get_CoreWebView2(&_webviewWindow);

						ICoreWebView2Settings* Settings;
						_webviewWindow->get_Settings(&Settings);
						Settings->put_AreHostObjectsAllowed(TRUE);
						Settings->put_IsScriptEnabled(TRUE);
						Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
						Settings->put_IsWebMessageEnabled(TRUE);

						EventRegistrationToken webMessageToken;
						_webviewWindow->AddScriptToExecuteOnDocumentCreated(L"window.external = { sendMessage: function(message) { window.chrome.webview.postMessage(message); }, receiveMessage: function(callback) { window.chrome.webview.addEventListener(\'message\', function(e) { callback(e.data); }); } };", nullptr);
						_webviewWindow->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
							[&](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
								wil::unique_cotaskmem_string message;
								args->TryGetWebMessageAsString(&message);
								_webMessageReceivedCallback(message.get());
								return S_OK;
							}).Get(), &webMessageToken);

						EventRegistrationToken webResourceRequestedToken;
						_webviewWindow->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
						_webviewWindow->add_WebResourceRequested(Callback<ICoreWebView2WebResourceRequestedEventHandler>(
							[&](ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args)
							{
								ICoreWebView2WebResourceRequest* req;
								args->get_Request(&req);

								wil::unique_cotaskmem_string uri;
								req->get_Uri(&uri);
								std::wstring uriString = uri.get();
								size_t colonPos = uriString.find(L':', 0);
								if (colonPos > 0)
								{
									std::wstring scheme = uriString.substr(0, colonPos);
									std::vector<wchar_t*>::iterator it = std::find(_customSchemeNames.begin(), _customSchemeNames.end(), scheme);

									if (it != _customSchemeNames.end() && _customSchemeCallback != NULL)
									{
										int numBytes;
										AutoString contentType;
										wil::unique_cotaskmem dotNetResponse(_customSchemeCallback((AutoString)uriString.c_str(), &numBytes, &contentType));

										if (dotNetResponse != nullptr && contentType != nullptr)
										{
											std::wstring contentTypeWS = contentType;

											IStream* dataStream = SHCreateMemStream((BYTE*)dotNetResponse.get(), numBytes);
											wil::com_ptr<ICoreWebView2WebResourceResponse> response;
											_webviewEnvironment->CreateWebResourceResponse(
												dataStream, 200, L"OK", (L"Content-Type: " + contentTypeWS).c_str(),
												&response);
											args->put_Response(response.get());
										}
									}
								}

								return S_OK;
							}
						).Get(), &webResourceRequestedToken);

						EventRegistrationToken permissionRequestedToken;
						_webviewWindow->add_PermissionRequested(
							Callback<ICoreWebView2PermissionRequestedEventHandler>(
								[&](ICoreWebView2* sender, ICoreWebView2PermissionRequestedEventArgs* args)	-> HRESULT {
									if (_grantBrowserPermissions)
										args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
									return S_OK;
								})
							.Get(),
									&permissionRequestedToken);

						if (_startUrl != NULL)
							NavigateToUrl(_startUrl);
						else if (_startString != NULL)
							NavigateToString(_startString);
						else
						{
							MessageBox(nullptr, L"Neither StartUrl nor StartString was specified", L"Native Initialization Failed", MB_OK);
							exit(0);
						}

						if (_contextMenuEnabled == false)
							SetContextMenuEnabled(false);

						if (_devToolsEnabled == false)
							SetDevToolsEnabled(false);

						if (_zoom != 100)
							SetZoom(_zoom);

						RefitContent();

						FocusWebView2();

						return S_OK;
					}).Get());
				return S_OK;
			}).Get());

	if (envResult != S_OK)
	{
		_com_error err(envResult);
		LPCTSTR errMsg = err.ErrorMessage();
		MessageBox(_hWnd, errMsg, L"Error instantiating webview", MB_OK);
	}
}


bool Photino::EnsureWebViewIsInstalled()
{
	LPWSTR* versionInfo = new wchar_t* [100];
	HRESULT ensureInstalledResult = GetAvailableCoreWebView2BrowserVersionString(nullptr, versionInfo);

	if (ensureInstalledResult != S_OK)
		return InstallWebView2();

	return true;
}

bool Photino::InstallWebView2()
{
	const wchar_t* srcURL = L"https://go.microsoft.com/fwlink/p/?LinkId=2124703";
	const wchar_t* destFile = L"MicrosoftEdgeWebview2Setup.exe";

	if (S_OK == URLDownloadToFile(NULL, srcURL, destFile, 0, NULL))
	{
		LPWSTR command = new wchar_t[100]{ L"MicrosoftEdgeWebview2Setup.exe\0" };	//add these switches? /silent /install

		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		bool success = CreateProcess(
			NULL,		// No module name (use command line)
			command,	// Command line
			NULL,       // Process handle not inheritable
			NULL,       // Thread handle not inheritable
			FALSE,      // Set handle inheritance to FALSE
			0,          // No creation flags
			NULL,       // Use parent's environment block
			NULL,       // Use parent's starting directory
			&si,        // Pointer to STARTUPINFO structure
			&pi);		// Pointer to PROCESS_INFORMATION structure

		if(success)
		{
			// wait for the installation to complete
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}

		return success;
	}

	return false;
}

void Photino::RefitContent()
{
	if (_webviewController)
	{
		RECT bounds;
		GetClientRect(_hWnd, &bounds);
		_webviewController->put_Bounds(bounds);
	}
}

void Photino::FocusWebView2()
{
	if (_webviewController)
	{
		_webviewController->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
	}
}

void Photino::ClearBrowserAutoFill()
{
	if (!_webviewWindow)
		return;

	auto webview15 = _webviewWindow.try_query<ICoreWebView2_15>();
	if (webview15)
	{
		wil::com_ptr<ICoreWebView2Profile> profile;
		webview15->get_Profile(&profile);
		auto profile2 = profile.try_query<ICoreWebView2Profile2>();

		if (profile2)
		{
			COREWEBVIEW2_BROWSING_DATA_KINDS dataKinds =
				(COREWEBVIEW2_BROWSING_DATA_KINDS)
				(COREWEBVIEW2_BROWSING_DATA_KINDS_GENERAL_AUTOFILL |
					COREWEBVIEW2_BROWSING_DATA_KINDS_PASSWORD_AUTOSAVE);

			profile2->ClearBrowsingData(
				dataKinds,
				Callback<ICoreWebView2ClearBrowsingDataCompletedHandler>(
					[this](HRESULT error)
					-> HRESULT {
						return S_OK;
					})
				.Get());
		}
	}
}

void Photino::SetWebView2RuntimePath(AutoString pathToWebView2)
{
	if (pathToWebView2 != NULL)
	{
		wcsncpy(_webview2RuntimePath, pathToWebView2, _countof(_webview2RuntimePath));
	}
}

void Photino::Show()
{
	ShowWindow(_hWnd, SW_SHOWDEFAULT);
	UpdateWindow(_hWnd);

	// Strangely, it only works to create the webview2 *after* the window has been shown,
	// so defer it until here. This unfortunately means you can't call the Navigate methods
	// until the window is shown.
	if (!_webviewController)
	{
		if (wcsnlen(_webview2RuntimePath, _countof(_webview2RuntimePath)) > 0 || Photino::EnsureWebViewIsInstalled())
			Photino::AttachWebView();
		else
			exit(0);
	}
}
