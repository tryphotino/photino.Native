#include "Photino.h"
#include <stdio.h>
#include <map>
#include <mutex>
#include <condition_variable>
#include <comdef.h>
#include <atomic>
#include <Shlwapi.h>
#include <wrl.h>
#include <windows.h>
#include <cstdio>
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

Photino::Photino(
	AutoString title, 
	AutoString starturl,
	Photino* parent, 
	WebMessageReceivedCallback webMessageReceivedCallback, 
	bool fullscreen, 
	int x = CW_USEDEFAULT, 
	int y = CW_USEDEFAULT, 
	int width = CW_USEDEFAULT, 
	int height = CW_USEDEFAULT,
	AutoString windowIconFile = L"")
{
	// Create the window
	_startUrl = starturl;
	_webMessageReceivedCallback = webMessageReceivedCallback;
	_parent = parent;

	if (fullscreen)
	{
		x = 0;
		y = 0;
		width = GetSystemMetrics(SM_CXSCREEN);
		height = GetSystemMetrics(SM_CYSCREEN);
	}

	_hWnd = CreateWindowEx(
		0,                              // Optional window styles.
		CLASS_NAME,                     // Window class
		title,							// Window text
		fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW,	// Window style

		// Size and position
		x, y, width, height,

		parent ? parent->_hWnd : NULL,       // Parent window
		NULL,       // Menu
		_hInstance, // Instance handle
		this        // Additional application data
	);
	hwndToPhotino[_hWnd] = this;

	if (windowIconFile != NULL && windowIconFile != L"")
		Photino::SetIconFile(windowIconFile);

	Show();
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
	//case WM_CLOSE:
	//{
	//	//if (MessageBox(hwnd, L"Really quit?", L"My application", MB_OKCANCEL) == IDOK)
	//	//{
	//	Photino* Photino = hwndToPhotino[hwnd];
	//	if (Photino)
	//	{
	//		Photino->InvokeClosing();
	//		InvokeWaitInfo* waitInfo = (InvokeWaitInfo*)lParam;
	//		{
	//			std::lock_guard<std::mutex> guard(invokeLockMutex);
	//			waitInfo->isCompleted = true;
	//		}
	//		waitInfo->completionNotifier.notify_one();
	//		DestroyWindow(hwnd);
	//	}
	//	//}
	//	return 0;
	//}
	case WM_DESTROY:
	{
		// Only terminate the message loop if the window being closed is the one that
		// started the message loop
		hwndToPhotino.erase(hwnd);
		if (hwnd == messageLoopRootWindowHandle)
		{
			PostQuitMessage(0);
		}
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
			Photino->InvokeResized(width, height);
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
			Photino->InvokeMoved(x, y);
		}
		return 0;
	}
	break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
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

void Photino::SetTitle(AutoString title)
{
	SetWindowText(_hWnd, title);
}

void Photino::Show()
{
	ShowWindow(_hWnd, SW_SHOWDEFAULT);

	// Strangely, it only works to create the webview2 *after* the window has been shown,
	// so defer it until here. This unfortunately means you can't call the Navigate methods
	// until the window is shown.
	if (!_webviewController)
	{
		if (EnsureWebViewIsInstalled())
			AttachWebView();
		else
			exit(0);
	}
}

void Photino::Minimize()
{
	ShowWindow(_hWnd, SW_MINIMIZE);
}

void Photino::GetMinimized(bool* isMinimized)
{
	LONG lStyles = GetWindowLong(_hWnd, GWL_STYLE);
	if (lStyles & WS_MINIMIZE) *isMinimized = true;
}

void Photino::Maximize()
{
	ShowWindow(_hWnd, SW_MAXIMIZE);
}

void Photino::GetMaximized(bool* isMaximized)
{
	LONG lStyles = GetWindowLong(_hWnd, GWL_STYLE);
	if (lStyles & WS_MAXIMIZE) *isMaximized = true;
}

void Photino::Restore()
{
	ShowWindow(_hWnd, SW_RESTORE);
}

void Photino::Close()
{
	InvokeWaitInfo waitInfo = {};
	SendMessage(_hWnd, WM_CLOSE, 0, (LPARAM)&waitInfo);

	// Block until the callback is actually executed and completed
	// TODO: Add return values, exception handling, etc.
	std::unique_lock<std::mutex> uLock(invokeLockMutex);
	waitInfo.completionNotifier.wait(uLock, [&] { return waitInfo.isCompleted; });
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

void Photino::ShowMessage(AutoString title, AutoString body, UINT type)
{
	ShowMessageParams* params = new ShowMessageParams;
	params->title = title;
	params->body = body;
	params->type = type;
	PostMessage(_hWnd, WM_USER_SHOWMESSAGE, (WPARAM)params, 0);
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

bool Photino::EnsureWebViewIsInstalled()
{
	LPWSTR* versionInfo = new wchar_t*[100] ;
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
	HRESULT envResult = CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[&, this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
				if (result != S_OK) { return result; }
				HRESULT envResult = env->QueryInterface(&_webviewEnvironment);
				if (envResult != S_OK) { return envResult; }

				env->CreateCoreWebView2Controller(_hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					[&, this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {

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
							[this](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
								wil::unique_cotaskmem_string message;
								args->TryGetWebMessageAsString(&message);
								_webMessageReceivedCallback(message.get());
								return S_OK;
							}).Get(), &webMessageToken);

						EventRegistrationToken webResourceRequestedToken;
						_webviewWindow->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
						_webviewWindow->add_WebResourceRequested(Callback<ICoreWebView2WebResourceRequestedEventHandler>(
							[this](ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args)
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
									WebResourceRequestedCallback handler = _schemeToRequestHandler[scheme];
									if (handler != NULL)
									{
										int numBytes;
										AutoString contentType;
										wil::unique_cotaskmem dotNetResponse(handler(uriString.c_str(), &numBytes, &contentType));

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

						NavigateToUrl(_startUrl);		//TODO: What if it needs to NavigateToString()?

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

void Photino::NavigateToUrl(AutoString url)
{
	_webviewWindow->Navigate(url);
}

void Photino::NavigateToString(AutoString content)
{
	_webviewWindow->NavigateToString(content);
}

void Photino::SendWebMessage(AutoString message)
{
	_webviewWindow->PostWebMessageAsString(message);
}

void Photino::AddCustomScheme(AutoString scheme, WebResourceRequestedCallback requestHandler)
{
	_schemeToRequestHandler[scheme] = requestHandler;
}

void Photino::SetResizable(bool resizable)
{
	LONG_PTR style = GetWindowLongPtr(_hWnd, GWL_STYLE);
	if (resizable) style |= WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	else style &= (~WS_THICKFRAME) & (~WS_MINIMIZEBOX) & (~WS_MAXIMIZEBOX);
	SetWindowLongPtr(_hWnd, GWL_STYLE, style);
}

void Photino::GetSize(int* width, int* height)
{
	RECT rect = {};
	GetWindowRect(_hWnd, &rect);
	if (width) *width = rect.right - rect.left;
	if (height) *height = rect.bottom - rect.top;
}

void Photino::SetSize(int width, int height)
{
	SetWindowPos(_hWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

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

unsigned int Photino::GetScreenDpi()
{
	return GetDpiForWindow(_hWnd);
}

void Photino::GetPosition(int* x, int* y)
{
	RECT rect = {};
	GetWindowRect(_hWnd, &rect);
	if (x) *x = rect.left;
	if (y) *y = rect.top;
}

void Photino::SetPosition(int x, int y)
{
	SetWindowPos(_hWnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void Photino::SetTopmost(bool topmost)
{
	SetWindowPos(_hWnd, topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
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