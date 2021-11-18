#ifdef __APPLE__
#include "Photino.h"
#include "Photino.Mac.AppDelegate.h"
#include "Photino.Mac.UiDelegate.h"
#include "Photino.Mac.UrlSchemeHandler.h"
#include "Photino.Mac.NSWindowBorderless.h"
#include <vector>

using namespace std;

//Creates an instance of the 'application' under which, all windows will run
//Only called once!
void Photino::Register()
{
    [NSAutoreleasePool new];

    AppDelegate *appDelegate = [[[AppDelegate alloc] init] autorelease];

    NSApplication *application = [NSApplication sharedApplication];
    [application setDelegate: appDelegate];
    [application setActivationPolicy: NSApplicationActivationPolicyRegular];

    NSString *appName = [[NSProcessInfo processInfo] processName];

    NSString *quitTitle = [@"Quit " stringByAppendingString: appName];
    NSMenuItem *quitMenuItem = [[
        [NSMenuItem alloc]
        initWithTitle: quitTitle
        action: @selector(terminate:)
        keyEquivalent: @"q"
    ] autorelease];
    
    NSMenu *mainMenu = [[NSMenu new] autorelease];
    NSMenuItem *mainMenuItem = [[NSMenuItem new] autorelease];
    NSMenu *subMenu = [[NSMenu new] autorelease];

    [mainMenu addItem: mainMenuItem];
    [mainMenuItem setSubmenu: subMenu];
    [subMenu addItem: quitMenuItem];

    [NSApp setMainMenu: mainMenu];
}

Photino::Photino(PhotinoInitParams* initParams)
{
	_windowTitle = new char[256];
	if (initParams->Title != NULL)
		strcpy(_windowTitle, initParams->Title);
	else
		_windowTitle[0] = 0;

	_startUrl = NULL;
	if (initParams->StartUrl != NULL)
	{
		_startUrl = new char[2048];
		if (_startUrl == NULL) exit(0);
		strcpy(_startUrl, initParams->StartUrl);
	}

	_startString = NULL;
	if (initParams->StartString != NULL)
	{
		_startString = new char[strlen(initParams->StartString) + 1];
		if (_startString == NULL) exit(0);
		strcpy(_startString, initParams->StartString);
	}

	_temporaryFilesPath = NULL;
	if (initParams->TemporaryFilesPath != NULL)
	{
		_temporaryFilesPath = new char[256];
		if (_temporaryFilesPath == NULL) exit(0);
		strcpy(_temporaryFilesPath, initParams->TemporaryFilesPath);
	}

	_contextMenuEnabled = true; //not configurable on mac //initParams->ContextMenuEnabled;
	_devToolsEnabled = initParams->DevToolsEnabled;
	_grantBrowserPermissions = initParams->GrantBrowserPermissions;

	_zoom = initParams->Zoom;

	//these handlers are ALWAYS hooked up
	_webMessageReceivedCallback = (WebMessageReceivedCallback)initParams->WebMessageReceivedHandler;
	_resizedCallback = (ResizedCallback)initParams->ResizedHandler;
	_movedCallback = (MovedCallback)initParams->MovedHandler;
	_closingCallback = (ClosingCallback)initParams->ClosingHandler;
    _focusInCallback = (FocusInCallback)initParams->FocusInHandler;
	_focusOutCallback = (FocusOutCallback)initParams->FocusOutHandler;
    _maximizedCallback = (MaximizedCallback)initParams->MaximizedHandler;
	_minimizedCallback = (MinimizedCallback)initParams->MinimizedHandler;
	_restoredCallback = (RestoredCallback)initParams->RestoredHandler;
	_customSchemeCallback = (WebResourceRequestedCallback)initParams->CustomSchemeHandler;

	//copy strings from the fixed size array passed, but only if they have a value.
	for (int i = 0; i < 16; ++i)
	{
		if (initParams->CustomSchemeNames[i] != NULL)
		{
			char* name = new char[50];
			strcpy(name, initParams->CustomSchemeNames[i]);
			_customSchemeNames.push_back(name);
		}
	}

	_parent = initParams->ParentInstance;
    
    if (initParams->UseOsDefaultSize)
	{
		initParams->Width = 800; //CW_USEDEFAULT;
		initParams->Height = 600; //CW_USEDEFAULT;
	}
	else
	{
		if (initParams->Width < 0) initParams->Width = 800; //CW_USEDEFAULT;
		if (initParams->Height < 0) initParams->Height = 600; //CW_USEDEFAULT;
	}

	if (initParams->UseOsDefaultLocation)
	{
		initParams->Left = 0; //CW_USEDEFAULT;
		initParams->Top = 0; //CW_USEDEFAULT;
	}

    // Create Window
    NSRect frame = NSMakeRect(0, 0, 0, 0);

    _chromeless = initParams->Chromeless;
    if (initParams->Chromeless)
    {
        // For MouseMoved events, Photino.Mac.NSWindowBorderless.mm
        // https://stackoverflow.com/questions/2520127/getting-a-borderless-window-to-receive-mousemoved-events-cocoa-osx
        _window = [[NSWindowBorderless alloc]
            initWithContentRect: frame
            styleMask: NSWindowStyleMaskBorderless
                | NSWindowStyleMaskClosable
                | NSWindowStyleMaskResizable
                | NSWindowStyleMaskMiniaturizable
            backing: NSBackingStoreBuffered
            defer: true];
    }
    else
    {
        _window = [[NSWindow alloc]
            initWithContentRect: frame
            styleMask: NSWindowStyleMaskTitled
                | NSWindowStyleMaskClosable
                | NSWindowStyleMaskResizable
                | NSWindowStyleMaskMiniaturizable
            backing: NSBackingStoreBuffered
            defer: true];
    }
    
    SetTitle(_windowTitle);
    SetPosition(initParams->Left, initParams->Top);
    SetSize(initParams->Width, initParams->Height);

    if (initParams->WindowIconFile != NULL && initParams->WindowIconFile[0] != '\0')
		Photino::SetIconFile(initParams->WindowIconFile);

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
  
    _webviewConfiguration = [[WKWebViewConfiguration alloc] init];
    if (_contextMenuEnabled) {} //Not configurable on mac
    if (_devToolsEnabled)
        [_webviewConfiguration.preferences setValue: @YES forKey: @"developerExtrasEnabled"];

    //wire up custom url schemes
    for (auto & scheme : _customSchemeNames)
    {
        AddCustomScheme(scheme, _customSchemeCallback);
    }

    _webview = nil;
    AttachWebView();

    if (_grantBrowserPermissions)
        SetGrantBrowserPermissions(_grantBrowserPermissions);

    Show();
    
    if (initParams->FullScreen)
        SetFullScreen(true);
}

Photino::~Photino()
{
	if (_startUrl != NULL) delete[]_startUrl;
	if (_startString != NULL) delete[]_startString;
	if (_temporaryFilesPath != NULL) delete[]_temporaryFilesPath;
	if (_windowTitle != NULL) delete[]_windowTitle;

    [_webviewConfiguration release];
    [_webview release];
    [_window performClose: _window];
    //[NSApp release];
}

void Photino::Center()
{
    [_window center];
    [_window makeKeyAndOrderFront: _window];

    //NSRect screen = [[_window screen] visibleFrame];
    //NSRect window = [_window frame];
    //CGFloat xPos = NSWidth(screen) / 2 + screen.origin.x - NSWidth(window) / 2;
    //CGFloat yPos = NSHeight(screen) / 2 + screen.origin.y - NSHeight(window) / 2;
    //[_window setFrame: NSMakeRect(xPos, yPos, NSWidth(window), NSHeight(window)) display:YES];
}

void Photino::Close()
{
    if (_chromeless)
    {
        // Can't use performClose because frame has no title area and close button
        [_window close];
    }
    else
    {
        // Simulates user clicking the close button
    	[_window performClose: _window];
    }
}

void Photino::GetContextMenuEnabled(bool* enabled)
{
    *enabled = _contextMenuEnabled;
}

void Photino::GetDevToolsEnabled(bool* enabled)
{
    *enabled = _devToolsEnabled;
}

void Photino::GetFullScreen(bool* fullScreen)
{
    *fullScreen = ([_window.contentView isInFullScreenMode]);
}

void Photino::GetGrantBrowserPermissions(bool* grant)
{
    *grant = _grantBrowserPermissions;
}

void Photino::GetMaximized(bool* isMaximized)
{
    *isMaximized = (([_window styleMask] & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen);
}

void Photino::GetMinimized(bool* isMinimized)
{
	*isMinimized = [_window isMiniaturized];
}

void Photino::GetPosition(int* x, int* y)
{
    NSRect frame = [_window frame];

    std::vector<Monitor*> monitors = GetMonitors();
    Monitor monitor = *monitors[0];

    int height = (int)roundf(frame.size.height);

    *x = (int)roundf(frame.origin.x);
    *y = (int)(monitor.monitor.height - ((int)roundf(frame.origin.y) + height)); // Assuming window is on monitor 0
 }

void Photino::GetResizable(bool* resizable)
{
   *resizable = (([_window styleMask] & NSWindowStyleMaskResizable) == NSWindowStyleMaskResizable);
}

unsigned int Photino::GetScreenDpi()
{
    //not supported on macOS - _window's devices collection does have dpi
	return 72;  //https://stackoverflow.com/questions/2621439/hot-to-get-screen-dpi-linux-mac-programaticaly
}

void Photino::GetSize(int* width, int* height)
{
    NSSize size = [_window frame].size;
    if (width) *width = (int)roundf(size.width);
    if (height) *height = (int)roundf(size.height);
}

AutoString Photino::GetTitle()
{
    return _windowTitle;
}

void Photino::GetTopmost(bool* topmost)
{
    *topmost = ([_window level] & NSFloatingWindowLevel) == NSFloatingWindowLevel;
}

void Photino::GetZoom(int* zoom)
{
	CGFloat rawValue = [_webview magnification];
	rawValue = (rawValue * 100.0) + 0.5;
	*zoom = (int)rawValue;
}

void Photino::NavigateToString(AutoString content)
{
    [_webview loadHTMLString: [NSString stringWithUTF8String: content] baseURL: nil];
}

void Photino::NavigateToUrl(AutoString url)
{
    NSString* nsurlstring = [NSString stringWithUTF8String: url];
    NSURL *nsurl= [NSURL URLWithString: nsurlstring];
    NSURLRequest *nsrequest= [NSURLRequest requestWithURL: nsurl];
    [_webview loadRequest: nsrequest];
}

void Photino::Restore()
{
    bool minimized;
    bool maximized;
    GetMinimized(&minimized);
    GetMaximized(&maximized);
	if (minimized) SetMinimized(false);
    if (maximized) SetMaximized(false);
}

void Photino::SendWebMessage(AutoString message)
{
    //NSString *msg = [NSString stringWithUTF8String: message];
    //NSAlert *alert = [[[NSAlert alloc] init] autorelease];
    //[alert setMessageText:msg];
    //[alert runModal];

    // JSON-encode the message
    NSString* nsmessage = [NSString stringWithUTF8String: message];

    NSData* data = [
        NSJSONSerialization
        dataWithJSONObject: @[nsmessage]
        options: 0
        error: nil];

    NSString *nsmessageJson = [[
        [NSString alloc]
        initWithData: data
        encoding: NSUTF8StringEncoding] autorelease];

    // Remove curly braces?
    nsmessageJson = [
        [nsmessageJson substringToIndex: ([nsmessageJson length] - 1)]
        substringFromIndex: 1
    ];

    NSString *javaScriptToEval = [NSString stringWithFormat: @"__dispatchMessageCallback(%@)", nsmessageJson];

    [_webview evaluateJavaScript: javaScriptToEval completionHandler: nil];
}

void Photino::SetContextMenuEnabled(bool enabled)
{
    //Can't be changed after initialization on mac
    //_contextMenuEnabled = enabled;
    //TODO:
}

void Photino::SetDevToolsEnabled(bool enabled)
{
    //Can't be changed after initialization on mac
    //_devToolsEnabled = enabled;
    //[_webview configuration: preferences: enableDevExtras: YES];
}

void Photino::SetFullScreen(bool fullScreen)
{
    if (fullScreen)
        [_window.contentView enterFullScreenMode: [NSScreen mainScreen] withOptions: nil];
    else
        [_window.contentView exitFullScreenModeWithOptions: nil];
}

void Photino::SetGrantBrowserPermissions(bool grant)
{
    //not available until macOS 11.3 aka 10.15.?
    //_grantBrowserPermissions = grant;
    //[_webview setCameraCaptureState: WKMediaCaptureStateActive];
    //[_webview setMicrophoneCaptureState: WKMediaCaptureStateActive];  
}

void Photino::SetIconFile(AutoString filename)
{
	NSString* path = [NSString stringWithUTF8String: filename];
    NSImage* icon = [[NSImage alloc] initWithContentsOfFile: path];
    if (icon != nil)
        [[_window standardWindowButton: NSWindowDocumentIconButton] setImage:icon];
}

void Photino::SetMinimized(bool minimized)
{
    if (_window.isMiniaturized == minimized) return;

    if (minimized)
        [_window miniaturize: NULL];
    else
	    [_window deminiaturize: NULL];
}

void Photino::SetMaximized(bool maximized)
{
    [_window toggleFullScreen: NULL];
}

void Photino::SetPosition(int x, int y)
{
    NSRect frame = [_window frame];
    std::vector<Monitor*> monitors = GetMonitors();
    Monitor monitor = *monitors[0];
    int height = (int)roundf(frame.size.height);
    CGFloat left = (CGFloat)x;
    CGFloat top = (CGFloat)(monitor.monitor.height - (y + height)); // Assuming window is on monitor 0
    CGPoint position = CGPointMake(left, top);
    [_window setFrameOrigin: position];
}

void Photino::SetResizable(bool resizable)
{
    if (resizable)
        _window.styleMask |= NSWindowStyleMaskResizable;
    else
        _window.styleMask &= ~NSWindowStyleMaskResizable;
}

void Photino::SetSize(int width, int height)
{
    NSRect frame = [_window frame];
    CGFloat fw = (CGFloat)width;
    CGFloat fh = (CGFloat)height;
    CGFloat oldHeight = frame.size.height;
    frame.size = CGSizeMake(fw, fh);
    frame.origin.y -= fh - oldHeight;
    [_window setFrame: frame display: true];
}

void Photino::SetTitle(AutoString title)
{
    strcpy(_windowTitle, title);
    [_window setTitle: [NSString stringWithUTF8String:title]];
}

void Photino::SetTopmost(bool topmost)
{
    if (topmost) [_window setLevel: NSFloatingWindowLevel];
    else [_window setLevel: NSNormalWindowLevel];
}

void Photino::SetZoom(int zoom)
{
    CGFloat newZoom = zoom / 100.0;
	[_webview setMagnification: newZoom];
}

void EnsureInvoke(dispatch_block_t block)
{
    if ([NSThread isMainThread])
        block();
    else
        dispatch_async(dispatch_get_main_queue(), block);
}

void Photino::ShowMessage(AutoString title, AutoString body, unsigned int type)
{
    EnsureInvoke(^{
        NSAlert *alert = [[NSAlert alloc] init];

        NSString *nstitle = [NSString stringWithUTF8String: title];
        NSString *nsbody= [NSString stringWithUTF8String: body];

        [alert setMessageText: nstitle];
        [alert setInformativeText: nsbody];
        [alert addButtonWithTitle: @"OK"];

        [alert
            beginSheetModalForWindow: _window
            completionHandler: ^void (NSModalResponse response) {
                [alert release];
            }];
    });
}

void Photino::ShowNotification(AutoString title, AutoString body)
{
    UNMutableNotificationContent *objNotificationContent = [[UNMutableNotificationContent alloc] init];
    objNotificationContent.title = [[NSString stringWithUTF8String:title] autorelease];
    objNotificationContent.body = [[NSString stringWithUTF8String:body] autorelease];
    objNotificationContent.sound = [UNNotificationSound defaultSound];
    UNTimeIntervalNotificationTrigger *trigger = [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:0.3 repeats:NO];
    UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:@"three" content:objNotificationContent trigger:trigger];
    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    [center addNotificationRequest:request withCompletionHandler:^(NSError * _Nullable error) {}];
}

void Photino::WaitForExit()
{
    [NSApp run];
}

//Callbacks
void Photino::GetAllMonitors(GetAllMonitorsCallback callback)
{
    if (callback)
    {
        for (NSScreen* screen in [NSScreen screens])
        {
            Monitor props = {};

            NSRect frame = [screen frame];
            props.monitor.x = (int)roundf(frame.origin.x);
            props.monitor.y = (int)roundf(frame.origin.y);
            props.monitor.width = (int)roundf(frame.size.width);
            props.monitor.height = (int)roundf(frame.size.height);

            NSRect vframe = [screen visibleFrame];
            props.work.x = (int)roundf(vframe.origin.x);
            props.work.y = (int)roundf(vframe.origin.y);
            props.work.width = (int)roundf(vframe.size.width);
            props.work.height = (int)roundf(vframe.size.height);

            callback(&props);
        }
    }
}

std::vector<Monitor *> Photino::GetMonitors()
{
    std::vector<Monitor *> monitors;

    for (NSScreen *screen : [NSScreen screens])
    {
        NSRect monitorFrame = [screen frame];
        Monitor::MonitorRect monitorArea;
        monitorArea.x = (int)roundf(monitorFrame.origin.x);
        monitorArea.y = (int)roundf(monitorFrame.origin.y);
        monitorArea.width = (int)roundf(monitorFrame.size.width);
        monitorArea.height = (int)roundf(monitorFrame.size.height);

        NSRect workFrame = [screen visibleFrame];
        Monitor::MonitorRect workArea;
        workArea.x = (int)roundf(workFrame.origin.x);
        workArea.y = (int)roundf(workFrame.origin.y);
        workArea.width = (int)roundf(workFrame.size.width);
        workArea.height = (int)roundf(workFrame.size.height);

        Monitor *monitor = new Monitor();
        monitor->monitor = monitorArea;
        monitor->work = workArea;

        monitors.push_back(monitor);
    }

    return monitors;
}

void Photino::Invoke(ACTION callback)
{
    dispatch_sync(dispatch_get_main_queue(), ^(void){
        callback();
    });
}

//private methods
void Photino::AddCustomScheme(AutoString scheme, WebResourceRequestedCallback requestHandler)
{
    // Note that this can only be done *before* the WKWebView is instantiated, so we only let this
    // get called from the options callback in the constructor
    UrlSchemeHandler* schemeHandler = [[[UrlSchemeHandler alloc] init] autorelease];
    schemeHandler->requestHandler = requestHandler;

    [_webviewConfiguration
        setURLSchemeHandler: schemeHandler
        forURLScheme: [NSString stringWithUTF8String: scheme]];
}

void Photino::AttachWebView()
{
    NSString *initScriptSource = @"window.__receiveMessageCallbacks = [];"
			"window.__dispatchMessageCallback = function(message) {"
			"	window.__receiveMessageCallbacks.forEach(function(callback) { callback(message); });"
			"};"
			"window.external = {"
			"	sendMessage: function(message) {"
			"		window.webkit.messageHandlers.photinointerop.postMessage(message);"
			"	},"
			"	receiveMessage: function(callback) {"
			"		window.__receiveMessageCallbacks.push(callback);"
			"	}"
			"};";

    WKUserScript *initScript = [
        [WKUserScript alloc]
        initWithSource: initScriptSource
        injectionTime: WKUserScriptInjectionTimeAtDocumentStart
        forMainFrameOnly: true];

    WKUserContentController *userContentController = [WKUserContentController new];
    [userContentController addUserScript:initScript];
    _webviewConfiguration.userContentController = userContentController;

    _webview = [
        [WKWebView alloc]
        initWithFrame: _window.contentView.frame
        configuration: _webviewConfiguration];

    [_webview setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];
    [_window.contentView addSubview: _webview];
    [_window.contentView setAutoresizesSubviews: true];

    UiDelegate *uiDelegate = [[[UiDelegate alloc] init] autorelease];
    uiDelegate->photino = this;
    uiDelegate->window = _window;
    uiDelegate->webMessageReceivedCallback = _webMessageReceivedCallback;

    [userContentController
        addScriptMessageHandler: uiDelegate
        name:@"photinointerop"];

    _webview.UIDelegate = uiDelegate;

    // TODO: Replace with WindowDelegate
    [[NSNotificationCenter defaultCenter]
        addObserver: uiDelegate
        selector: @selector(windowDidResize:)
        name: NSWindowDidResizeNotification
        object: _window];
    
    [[NSNotificationCenter defaultCenter]
        addObserver: uiDelegate
        selector: @selector(windowDidMove:)
        name: NSWindowDidMoveNotification
        object: _window];

    if (_startUrl != NULL)
        NavigateToUrl(_startUrl);
    else if (_startString != NULL)
        NavigateToString(_startString);
    else
    {    
        NSAlert *alert = [[[NSAlert alloc] init] autorelease];
        [alert setMessageText:@"Neither StartUrl nor StartString was specified"];
        [alert runModal];
    }
}

void Photino::Show()
{
    if (_webview == nil)
        AttachWebView();

    [_window makeKeyAndOrderFront: _window];
    [_window orderFrontRegardless];
}
#endif