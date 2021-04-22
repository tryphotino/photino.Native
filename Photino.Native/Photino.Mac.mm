#include "Photino.h"
#include "Photino.Mac.AppDelegate.h"
#include "Photino.Mac.UiDelegate.h"
#include "Photino.Mac.UrlSchemeHandler.h"
#include <vector>

using namespace std;

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

Photino::Photino(
    AutoString title,
    AutoString starturl,
    Photino* parent, 
    WebMessageReceivedCallback webMessageReceivedCallback, 
    bool fullscreen, 
    int x, 
    int y, 
    int width, 
    int height, 
    AutoString windowIconFile,
    bool chromeless)
{
    _startUrl = starturl;
    _webMessageReceivedCallback = webMessageReceivedCallback;
    
    // Create Window
    NSRect frame = NSMakeRect(0, 0, 0, 0);

    NSWindowStyleMask style;
    if (chromeless) {
        style = NSWindowStyleMaskBorderless;
    }
    else {
        style = NSWindowStyleMaskTitled
              | NSWindowStyleMaskClosable
              | NSWindowStyleMaskResizable
              | NSWindowStyleMaskMiniaturizable;
    }

    _window = [[NSWindow alloc]
        initWithContentRect: frame
        styleMask: style
        backing: NSBackingStoreBuffered
        defer: true];
    
    SetTitle(title);
    SetPosition(x, y);
    SetSize(width, height);

    _webviewConfiguration = [[WKWebViewConfiguration alloc] init];
    [_webviewConfiguration.preferences
        setValue: @YES
        forKey: @"developerExtrasEnabled"];

    _webview = nil;
}

Photino::~Photino()
{
    [_webviewConfiguration release];
    [_webview release];
    [_window close];
    [NSApp release];
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
}

void Photino::Show()
{
    if (_webview == nil) {
        AttachWebView();
    }

    [_window makeKeyAndOrderFront: _window];
    [_window orderFrontRegardless];
}

void Photino::Minimize()
{
	//???
}

void Photino::GetMinimized(bool* isMinimized)
{
	//???
}

void Photino::Maximize()
{
	//???
}

void Photino::GetMaximized(bool* isMaximized)
{
	//???
}

void Photino::Restore()
{
	//???
}

void Photino::Close()
{
	[_window performClose: _window];
}

void Photino::SetTitle(AutoString title)
{
    [_window setTitle: [NSString stringWithUTF8String:title]];
}

void Photino::WaitForExit()
{
    [NSApp run];
}

void Photino::Invoke(ACTION callback)
{
    dispatch_sync(dispatch_get_main_queue(), ^(void){
        callback();
    });
}
void EnsureInvoke(dispatch_block_t block)
{
    if ([NSThread isMainThread])
    {
        block();
    }
    else
    {
        dispatch_async(dispatch_get_main_queue(), block);
    }
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

void Photino::NavigateToString(AutoString content)
{
    [_webview
        loadHTMLString: [NSString stringWithUTF8String: content]
        baseURL: nil];
}

void Photino::NavigateToUrl(AutoString url)
{
    NSString* nsurlstring = [NSString stringWithUTF8String: url];
    NSURL *nsurl= [NSURL URLWithString: nsurlstring];
    NSURLRequest *nsrequest= [NSURLRequest requestWithURL: nsurl];
    
    [_webview loadRequest: nsrequest];
}

void Photino::SendWebMessage(AutoString message)
{
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

    [_webview
        evaluateJavaScript: javaScriptToEval
        completionHandler: nil];
}

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

void Photino::SetResizable(bool resizable)
{
    if (resizable)
    {
        _window.styleMask |= NSWindowStyleMaskResizable;
    }
    else
    {
        _window.styleMask &= ~NSWindowStyleMaskResizable;
    }
}

void Photino::GetSize(int* width, int* height)
{
    NSSize size = [_window frame].size;
    if (width) *width = (int)roundf(size.width);
    if (height) *height = (int)roundf(size.height);
}

void Photino::SetSize(int width, int height)
{
    NSRect frame = [_window frame];
    
    CGFloat fw = (CGFloat)width;
    CGFloat fh = (CGFloat)height;
    
    CGFloat oldHeight = frame.size.height;

    frame.size = CGSizeMake(fw, fh);
    frame.origin.y -= fh - oldHeight;

    [_window
        setFrame: frame
        display: true];
}

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

unsigned int Photino::GetScreenDpi()
{
	return 72;
}

void Photino::GetPosition(int* x, int* y)
{
    NSRect frame = [_window frame];

    std::vector<Monitor*> monitors = GetMonitors();
    Monitor monitor = *monitors[0];

    int height = (int)roundf(frame.size.height);

    *x = (int)roundf(frame.origin.x);
    *y = (int)(monitor.work.height - ((int)roundf(frame.origin.y) + height)); // Assuming window is on monitor 0
}

void Photino::SetPosition(int x, int y)
{
    NSRect frame = [_window frame];

    std::vector<Monitor*> monitors = GetMonitors();
    Monitor monitor = *monitors[0];

    int height = (int)roundf(frame.size.height);

    CGFloat left = (CGFloat)x;
    CGFloat top = (CGFloat)(monitor.work.height - (y + height)); // Assuming window is on monitor 0

    CGPoint position = CGPointMake(left, top);

    [_window setFrameOrigin: position];
}

void Photino::SetTopmost(bool topmost)
{
    if (topmost) [_window setLevel: NSFloatingWindowLevel];
    else [_window setLevel: NSNormalWindowLevel];
}

void Photino::SetIconFile(AutoString filename)
{
	NSString* path = [NSString stringWithUTF8String: filename];
    NSImage* icon = [[NSImage alloc] initWithContentsOfFile: path];

    if (icon != nil)
    {
        [[_window standardWindowButton: NSWindowDocumentIconButton] setImage:icon];
    }
}