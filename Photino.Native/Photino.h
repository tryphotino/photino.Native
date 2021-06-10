#pragma once

#ifdef _WIN32
#include <Windows.h>
#include <wil/com.h>
#include <WebView2.h>
typedef const wchar_t* AutoString;
#else
// AutoString for macOS/Linux
typedef char* AutoString;
#endif

#ifdef __APPLE__
#include <Cocoa/Cocoa.h>
#include <WebKit/WebKit.h>
#endif

#ifdef __linux__
#include <gtk/gtk.h>
#endif

#include <map>
#include <string>

struct Monitor
{
	struct MonitorRect
	{
		int x, y;
		int width, height;
	} monitor, work;
};

typedef void (*ACTION)();
typedef void (*WebMessageReceivedCallback)(AutoString message);
typedef void* (*WebResourceRequestedCallback)(AutoString url, int* outNumBytes, AutoString* outContentType);
typedef int (*GetAllMonitorsCallback)(const Monitor* monitor);
typedef void (*ResizedCallback)(int width, int height);
typedef void (*MovedCallback)(int x, int y);
typedef void (*ClosingCallback)();

class Photino;
struct PhotinoInitParams
{
	AutoString StartString;
	AutoString StartUrl;
	AutoString Title;
	AutoString WindowIconFile;

	Photino* ParentInstance;

	ClosingCallback* ClosingHandler;
	ResizedCallback* ResizedHandler;
	MovedCallback* MovedHandler;
	WebMessageReceivedCallback* WebMessageReceivedHandler;
	
	AutoString CustomSchemaNames[32];
	WebResourceRequestedCallback CustomSchemaHandlers[32];

	int Left;
	int Top;
	int Width;
	int Height;
	int Zoom;

	bool CenterOnInitialize;
	bool Chromeless;
	bool FullScreen;
	bool Maximized;
	bool Minimized;
	bool Resizable;
	bool Topmost;
	bool UseOsDefaultLocation;
	bool UseOsDefaultSize;
};

class Photino
{
private:
	WebMessageReceivedCallback _webMessageReceivedCallback;
	MovedCallback _movedCallback;
	ResizedCallback _resizedCallback;
	ClosingCallback _closingCallback;
	AutoString _startUrl;
	AutoString _startString;
	double _zoom;
#ifdef _WIN32
	static HINSTANCE _hInstance;
	HWND _hWnd;
	Photino* _parenthWnd;
	wil::com_ptr<ICoreWebView2Environment> _webviewEnvironment;
	wil::com_ptr<ICoreWebView2> _webviewWindow;
	wil::com_ptr<ICoreWebView2Controller> _webviewController;
	std::map<std::wstring, WebResourceRequestedCallback> _schemeToRequestHandler;
	bool EnsureWebViewIsInstalled();
	bool InstallWebView2();
	void Show();
	void AttachWebView();
#elif __linux__
	GtkWidget* _window;
	GtkWidget* _webview;
#elif __APPLE__
    // NSApplication *_app;
    NSWindow *_window;
    WKWebView *_webview;
	WKWebViewConfiguration *_webviewConfiguration;
    std::vector<Monitor *> GetMonitors();
#endif

public:
#ifdef _WIN32
	static void Register(HINSTANCE hInstance);
	HWND getHwnd();
	void RefitContent();
#elif __APPLE__
	static void Register();
#endif

	Photino(PhotinoInitParams* initParams);
	~Photino();

	void Center();
	void Close();
	void GetMaximized(bool* isMaximized);
	void GetMinimized(bool* isMinimized);
	void GetPosition(int* x, int* y);
	unsigned int GetScreenDpi();
	void GetSize(int* width, int* height);
	void GetZoom(int* zoom);
	void Maximize();
	void Minimize();
	void NavigateToString(AutoString content);
	void NavigateToUrl(AutoString url);
	void Restore();
	void SendWebMessage(AutoString message);
	void SetIconFile(AutoString filename);
	void SetPosition(int x, int y);
	void SetResizable(bool resizable);
	void SetSize(int width, int height);
	void SetTitle(AutoString title);
	void SetTopmost(bool topmost);
	void SetZoom(int zoom);
	void ShowMessage(AutoString title, AutoString body, unsigned int type);
	void WaitForExit();

	//Callbacks
	void AddCustomScheme(AutoString scheme, WebResourceRequestedCallback requestHandler);
	void GetAllMonitors(GetAllMonitorsCallback callback);
	void SetClosingCallback(ClosingCallback callback) { _closingCallback = callback; }
	void SetMovedCallback(MovedCallback callback) { _movedCallback = callback; }
	void SetResizedCallback(ResizedCallback callback) { _resizedCallback = callback; }

	void Invoke(ACTION callback);
	void InvokeClosing() { if (_closingCallback) _closingCallback(); }
	void InvokeMoved(int x, int y) { if (_movedCallback) _movedCallback(x, y); }
	void InvokeResized(int width, int height) { if (_resizedCallback) _resizedCallback(width, height); }
};
