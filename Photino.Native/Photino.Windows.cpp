#include "Photino.h"
#include <mutex>
#include <condition_variable>
#include <comdef.h>
#include <atomic>
#include <Shlwapi.h>
#include <wrl.h>
#include <windows.h>
#include <cstdio>
#include <algorithm>
#pragma comment(lib, "Urlmon.lib")

#define WM_USER_SHOWMESSAGE (WM_USER + 0x0001)
#define WM_USER_INVOKE (WM_USER + 0x0002)

using namespace Microsoft::WRL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LPCWSTR CLASS_NAME = L"Photino";
std::mutex invokeLockMutex;
HINSTANCE Photino::_hInstance;
HWND messageLoopRootWindowHandle;
std::map<HWND, Photino*> hwndToPhotino;

struct InvokeWaitInfo
{
	std::condition_variable completionNotifier;
	bool isCompleted;
};

struct ShowMessageParams
{
	std::wstring title;
	std::wstring body;
	UINT type;
};

void Photino::Register(HINSTANCE hInstance)
{
	_hInstance = hInstance;

	// Register the window class	
	WNDCLASSW wc = { };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);

	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
}


Photino::Photino(PhotinoInitParams* initParams)
{	
	if (initParams->Size != sizeof(PhotinoInitParams))
	{
		wchar_t msg[200];
		swprintf(msg, 200, L"Initial parameters passed are %i bytes, but expected %i bytes.", initParams->Size, sizeof(PhotinoInitParams));
		throw msg;
	}

	_startUrl = initParams->StartUrl;
	_startString = initParams->StartString;
	_zoom = initParams->Zoom;

	//these handlers are ALWAYS hooked up
	_webMessageReceivedCallback = (WebMessageReceivedCallback)initParams->WebMessageReceivedHandler;
	_resizedCallback = (ResizedCallback)initParams->ResizedHandler;
	_movedCallback = (MovedCallback)initParams->MovedHandler;
	_closingCallback = (ClosingCallback)initParams->ClosingHandler;
	_customSchemeCallback = (WebResourceRequestedCallback)initParams->CustomSchemeHandler;

	//copy strings from the fixed size array passed, but only if they have a value.
	int i = 0;
	while (i < 16)
	{
		if (initParams->CustomSchemeNames[i] != NULL)
			_customSchemeNames.push_back(initParams->CustomSchemeNames[i]);
		i++;
	}


	_parenthWnd = initParams->ParentInstance;

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

	//Create the window
	_hWnd = CreateWindowEx(
		0,                      //Optional window styles.
		CLASS_NAME,             //Window class
		initParams->Title,		//Window text
		initParams->Chromeless || initParams->FullScreen ? WS_POPUP : WS_OVERLAPPEDWINDOW,	//Window style

		// Size and position
		initParams->Left, initParams->Top, initParams->Width, initParams->Height,

		NULL,		//initParams.ParentHandle == nullptr ? initParams.ParentHandle : NULL,   //Parent window handle
		NULL,       //Menu
		_hInstance, //Instance handle
		this        //Additional application data
	);
	hwndToPhotino[_hWnd] = this;

	if (initParams->WindowIconFile != NULL && initParams->WindowIconFile != L"")
		Photino::SetIconFile(initParams->WindowIconFile);

	if (initParams->CenterOnInitialize)
		Photino::Center();

	if (initParams->Minimized)
		Minimize();

	if (initParams->Maximized)
		Maximize();

	if (initParams->Resizable == false)
		SetResizable(false);

	if (initParams->Topmost)
		SetTopmost(true);

	Photino::Show();
}

// Needn't to release the handles.
Photino::~Photino() {}

HWND Photino::getHwnd()
{
	return _hWnd;
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
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
	case WM_USER_SHOWMESSAGE:
	{
		ShowMessageParams* params = (ShowMessageParams*)wParam;
		MessageBox(hwnd, params->body.c_str(), params->title.c_str(), params->type);
		delete params;
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
	case WM_SIZE:
	{
		Photino* Photino = hwndToPhotino[hwnd];
		if (Photino)
		{
			Photino->RefitContent();
			int width, height;
			Photino->GetSize(&width, &height);
			Photino->InvokeResize(width, height);
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
	//InvokeWaitInfo waitInfo = {};
	//SendMessage(_hWnd, WM_CLOSE, 0, (LPARAM)&waitInfo);

	//// Block until the callback is actually executed and completed
	//// TODO: Add return values, exception handling, etc.
	//std::unique_lock<std::mutex> uLock(invokeLockMutex);
	//waitInfo.completionNotifier.wait(uLock, [&] { return waitInfo.isCompleted; });
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

void Photino::GetTitle(AutoString windowTitle)
{
	int titleLength = GetWindowTextLength(_hWnd) + 1;
	wchar_t* title = new wchar_t[titleLength];
	GetWindowText(_hWnd, title, titleLength);
	windowTitle = title;
}

void Photino::GetZoom(int* zoom)
{
	double rawValue = 0;
	_webviewController->get_ZoomFactor(&rawValue);
	rawValue = (rawValue * 100) + 0.5;		//account for rounding issues
	*zoom = (int)rawValue;
}

void Photino::Minimize()
{
	ShowWindow(_hWnd, SW_MINIMIZE);
}

void Photino::Maximize()
{
	ShowWindow(_hWnd, SW_MAXIMIZE);
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

void Photino::SetIconFile(AutoString filename)
{
	HICON iconSmall = (HICON)LoadImage(NULL, filename, IMAGE_ICON, 16, 16, LR_LOADFROMFILE | LR_LOADTRANSPARENT);
	HICON iconBig = (HICON)LoadImage(NULL, filename, IMAGE_ICON, 32, 32, LR_LOADFROMFILE | LR_LOADTRANSPARENT);

	if (iconSmall && iconBig)
	{
		SendMessage(_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)iconSmall);
		SendMessage(_hWnd, WM_SETICON, ICON_BIG, (LPARAM)iconBig);
	}
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
	SetWindowText(_hWnd, title);
}

void Photino::SetTopmost(bool topmost)
{
	SetWindowPos(_hWnd, topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void Photino::SetZoom(int zoom)
{
	double newZoom = (double)zoom / 100;
	HRESULT r = _webviewController->put_ZoomFactor(newZoom);
	//wchar_t msg[50];
	//swprintf(msg, 50, L"newZoom: %f", newZoom);
	//MessageBox(nullptr, msg, L"Setter", MB_OK);
}

void Photino::ShowMessage(AutoString title, AutoString body, UINT type)
{
	ShowMessageParams* params = new ShowMessageParams;
	params->title = title;
	params->body = body;
	params->type = type;
	PostMessage(_hWnd, WM_USER_SHOWMESSAGE, (WPARAM)params, 0);
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
		EnumDisplayMonitors(NULL, NULL, MonitorEnum, (LPARAM)callback);
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
void Photino::RefitContent()
{
	if (_webviewController)
	{
		RECT bounds;
		GetClientRect(_hWnd, &bounds);
		_webviewController->put_Bounds(bounds);
	}
}

void Photino::Show()
{
	ShowWindow(_hWnd, SW_SHOWDEFAULT);

	// Strangely, it only works to create the webview2 *after* the window has been shown,
	// so defer it until here. This unfortunately means you can't call the Navigate methods
	// until the window is shown.
	if (!_webviewController)
	{
		if (Photino::EnsureWebViewIsInstalled())
			Photino::AttachWebView();
		else
			exit(0);
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

		bool installed = CreateProcess(
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

		return installed;
	}

	return false;
}

void Photino::AttachWebView()
{
	bool isUrl = false;
	wchar_t startUrl[4096];
	if (_startUrl != NULL)
	{
		isUrl = true;
		wcscpy_s(startUrl, _startUrl);
	}

	bool isString = false;
	wchar_t startString[65536];
	if (_startString != NULL)
	{
		isString = true;
		wcscpy_s(startString, _startString);
	}

	int Zoom = _zoom;

	HRESULT envResult = CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[&, Zoom](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
				if (result != S_OK) { return result; }
				HRESULT envResult = env->QueryInterface(&_webviewEnvironment);
				if (envResult != S_OK) { return envResult; }

				env->CreateCoreWebView2Controller(_hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					[&, Zoom](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {

						if (result != S_OK) { return result; }

						HRESULT envResult = controller->QueryInterface(&_webviewController);
						if (envResult != S_OK) { return envResult; }
						_webviewController->get_CoreWebView2(&_webviewWindow);
						
						ICoreWebView2Settings* Settings;
						_webviewWindow->get_Settings(&Settings);
						Settings->put_IsScriptEnabled(TRUE);
						Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
						Settings->put_IsWebMessageEnabled(TRUE);

						// Register interop APIs
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
									std::vector<AutoString>::iterator it = std::find(_customSchemeNames.begin(), _customSchemeNames.end(), scheme);

									//wchar_t* url = _wcsdup(uriString.c_str());
									//wchar_t msg[100];
									//swprintf(msg, 100, L"it: %i  .end(): %i,  _customSchemeCallback: %p", it, _customSchemeNames.end(), _customSchemeCallback);
									//MessageBox(nullptr, msg, L"", MB_OK);

									if (it != _customSchemeNames.end() && _customSchemeCallback != NULL)
									{
										int numBytes;
										AutoString contentType;
										wil::unique_cotaskmem dotNetResponse(_customSchemeCallback(uriString.c_str(), &numBytes, &contentType));

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

						
						if (isUrl)
							NavigateToUrl(startUrl);
						else if (isString)
							NavigateToString(startString);
						else
							throw "Neither StartUrl nor StartString was specified";

						SetZoom(_zoom);

						RefitContent();

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
