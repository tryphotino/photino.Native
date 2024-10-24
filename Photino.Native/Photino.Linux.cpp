// For this to build on WSL (Ubuntu 18.04) you need to:
//  sudo apt-get install libgtk-3-dev libwebkit2gtk-4.0-dev
#ifdef __linux__
#include "Photino.h"
#include "Photino.Dialog.h"
#include <mutex>
#include <condition_variable>
#include <X11/Xlib.h>
#include <webkit2/webkit2.h>
#include <JavaScriptCore/JavaScript.h>
#include <sstream>
#include <iomanip>
#include <libnotify/notify.h>
#include <dlfcn.h>	//for dynamically calling functions from shared libraries
#include "json.hpp"
using json = nlohmann::json;

/* --- PRINTF_BINARY_FORMAT macro's --- */
// #define FMT_BUF_SIZE (CHAR_BIT*sizeof(uintmax_t)+1)
//
// char *binary_fmt(uintmax_t x, char buf[FMT_BUF_SIZE])
//{
//     char *s = buf + FMT_BUF_SIZE;
//     *--s = 0;
//     if (!x) *--s = '0';
//     for (; x; x /= 2) *--s = '0' + x%2;
//     return s;
// }
/* --- end macro --- */

std::mutex invokeLockMutex;

struct InvokeWaitInfo
{
	ACTION callback;
	std::condition_variable completionNotifier;
	bool isCompleted;
};

struct InvokeJSWaitInfo
{
	bool isCompleted;
};

// window size or position changed
gboolean on_configure_event(GtkWidget *widget, GdkEvent *event, gpointer self);
gboolean on_window_state_event(GtkWidget *widget, GdkEventWindowState *event, gpointer self);
gboolean on_widget_deleted(GtkWidget *widget, GdkEvent *event, gpointer self);
gboolean on_focus_in_event(GtkWidget *widget, GdkEvent *event, gpointer self);
gboolean on_focus_out_event(GtkWidget *widget, GdkEvent *event, gpointer self);
gboolean on_webview_context_menu(WebKitWebView *web_view,
								 GtkWidget *default_menu,
								 WebKitHitTestResult *hit_test_result,
								 gboolean triggered_with_keyboard,
								 gpointer user_data);
gboolean on_permission_request(WebKitWebView *web_view, WebKitPermissionRequest *request, gpointer user_data);

Photino::Photino(PhotinoInitParams *initParams) : _webview(nullptr)
{
	// It makes xlib thread safe.
	// Needed for get_position.
	XInitThreads();
	gtk_init(0, NULL);
	notify_init(initParams->Title);

	if (initParams->Size != sizeof(PhotinoInitParams))
	{
		GtkWidget *dialog = gtk_message_dialog_new(
			nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Initial parameters passed are %i bytes, but expected %lu bytes.", initParams->Size, sizeof(PhotinoInitParams));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		exit(0);
	}

	_windowTitle = new char[256];
	if (initParams->Title != NULL)
		strcpy(_windowTitle, initParams->Title);
	else
		_windowTitle[0] = 0;

	_startUrl = NULL;
	if (initParams->StartUrl != NULL)
	{
		_startUrl = new char[2048];
		if (_startUrl == NULL)
			exit(0);
		strcpy(_startUrl, initParams->StartUrl);
	}

	_startString = NULL;
	if (initParams->StartString != NULL)
	{
		_startString = new char[strlen(initParams->StartString) + 1];
		if (_startString == NULL)
			exit(0);
		strcpy(_startString, initParams->StartString);
	}

	_temporaryFilesPath = NULL;
	if (initParams->TemporaryFilesPath != NULL)
	{
		_temporaryFilesPath = new char[256];
		if (_temporaryFilesPath == NULL)
			exit(0);
		strcpy(_temporaryFilesPath, initParams->TemporaryFilesPath);
	}

	_userAgent = NULL;
	if (initParams->UserAgent != NULL)
	{
		_userAgent = new char[strlen(initParams->UserAgent) + 1];
		if (_userAgent == NULL)
			exit(0);
		strcpy(_userAgent, initParams->UserAgent);
	}

	_browserControlInitParameters = NULL;
	if (initParams->BrowserControlInitParameters != NULL)
	{
		_browserControlInitParameters = new char[strlen(initParams->BrowserControlInitParameters) + 1];
		if (_browserControlInitParameters == NULL)
			exit(0);
		strcpy(_browserControlInitParameters, initParams->BrowserControlInitParameters);
	}

	_transparentEnabled = initParams->Transparent;
	_contextMenuEnabled = initParams->ContextMenuEnabled;
	_devToolsEnabled = initParams->DevToolsEnabled;
	_grantBrowserPermissions = initParams->GrantBrowserPermissions;
	_mediaAutoplayEnabled = initParams->MediaAutoplayEnabled;
	_fileSystemAccessEnabled = initParams->FileSystemAccessEnabled;
	_webSecurityEnabled = initParams->WebSecurityEnabled;
	_javascriptClipboardAccessEnabled = initParams->JavascriptClipboardAccessEnabled;
	_mediaStreamEnabled = initParams->MediaStreamEnabled;
	_smoothScrollingEnabled = initParams->SmoothScrollingEnabled;
	_ignoreCertificateErrorsEnabled = initParams->IgnoreCertificateErrorsEnabled;
	_isFullScreen = initParams->FullScreen;

	_zoom = initParams->Zoom;
	_minWidth = initParams->MinWidth;
	_minHeight = initParams->MinHeight;
	_maxWidth = initParams->MaxWidth;
	_maxHeight = initParams->MaxHeight;

	// these handlers are ALWAYS hooked up
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

	// copy strings from the fixed size array passed, but only if they have a value.
	for (int i = 0; i < 16; ++i)
	{
		if (initParams->CustomSchemeNames[i] != NULL)
		{
			char *name = new char[50];
			strcpy(name, initParams->CustomSchemeNames[i]);
			_customSchemeNames.push_back(name);
		}
	}

	_parent = initParams->ParentInstance;

	_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	_dialog = new PhotinoDialog();

	if (initParams->FullScreen)
		SetFullScreen(true);
	else
	{
		// Ensure that the default size does not exceed any set min/max dimension
		if (initParams->Width > initParams->MaxWidth) initParams->Width = initParams->MaxWidth;
		if (initParams->Height > initParams->MaxHeight) initParams->Height = initParams->MaxHeight;
		if (initParams->Width < initParams->MinWidth) initParams->Width = initParams->MinWidth;
		if (initParams->Height < initParams->MinHeight) initParams->Height = initParams->MinHeight;

		if (initParams->UseOsDefaultSize)
			gtk_window_set_default_size(GTK_WINDOW(_window), -1, -1);
		else
			gtk_window_set_default_size(GTK_WINDOW(_window), initParams->Width, initParams->Height);

		SetMinSize(initParams->MinWidth, initParams->MinHeight); // Defaults to 0,0
		SetMaxSize(initParams->MaxWidth, initParams->MaxHeight); // Defaults to max int, max int

		if (initParams->UseOsDefaultLocation)
			gtk_window_set_position(GTK_WINDOW(_window), GTK_WIN_POS_NONE);
		else if (initParams->CenterOnInitialize && !initParams->FullScreen)
			gtk_window_set_position(GTK_WINDOW(_window), GTK_WIN_POS_CENTER);
		else
			gtk_window_move(GTK_WINDOW(_window), initParams->Left, initParams->Top);
	}

	SetTitle(_windowTitle);

	if (initParams->Chromeless)
		gtk_window_set_decorated(GTK_WINDOW(_window), false);

	if (initParams->WindowIconFile != NULL && strlen(initParams->WindowIconFile) > 0)
		Photino::SetIconFile(initParams->WindowIconFile);

	if (initParams->CenterOnInitialize)
		Photino::Center();

	if (initParams->Minimized)
		Photino::SetMinimized(true);

	if (initParams->Maximized)
		Photino::SetMaximized(true);

	if (!initParams->Resizable)
		Photino::SetResizable(false);

	if (initParams->Topmost)
		Photino::SetTopmost(true);

	if (_parent == NULL)
	{
		g_signal_connect(G_OBJECT(_window), "destroy",
						 G_CALLBACK(+[](GtkWidget *w, gpointer arg)
									{ gtk_main_quit(); }),
						 this);
	}

	// g_signal_connect(G_OBJECT(_window), "size-allocate",
	//	G_CALLBACK(on_size_allocate),
	//	this);

	g_signal_connect(G_OBJECT(_window), "configure-event",
					 G_CALLBACK(on_configure_event),
					 this);

	g_signal_connect(G_OBJECT(_window), "window-state-event",
					 G_CALLBACK(on_window_state_event),
					 this);

	g_signal_connect(G_OBJECT(_window), "delete-event",
					 G_CALLBACK(on_widget_deleted),
					 this);

	//if (initParams->Transparent)
	//{
	//	GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(_window));
	//	GdkVisual *rgba_visual = gdk_screen_get_rgba_visual(screen);

	//	if (rgba_visual)
	//	{
	//		gtk_widget_set_visual(GTK_WIDGET(_window), rgba_visual);
	//		gtk_widget_set_app_paintable(GTK_WIDGET(_window), true);
	//	}
	//}

	Photino::Show(false);

	g_signal_connect(G_OBJECT(_window), "focus-in-event",
					 G_CALLBACK(on_focus_in_event),
					 this);

	g_signal_connect(G_OBJECT(_window), "focus-out-event",
					 G_CALLBACK(on_focus_out_event),
					 this);

	// These must be called after the webview control is initialized.
	g_signal_connect(G_OBJECT(_webview), "context-menu",
					 G_CALLBACK(on_webview_context_menu),
					 this);

	g_signal_connect(G_OBJECT(_webview), "permission-request",
					 G_CALLBACK(on_permission_request),
					 this);

	Photino::AddCustomSchemeHandlers();

	if (initParams->Transparent)
		Photino::SetTransparentEnabled(true);

	if (_zoom != 100.0)
		SetZoom(_zoom);

	//gchar* webkitVer = g_strconcat(g_strdup_printf("%d", webkit_get_major_version()), ".", g_strdup_printf("%d", webkit_get_minor_version()), ".", g_strdup_printf("%d", webkit_get_micro_version()), NULL);
	//Photino::ShowNotification("Web Kit Version", webkitVer);
}

Photino::~Photino()
{
	notify_uninit();
	gtk_widget_destroy(_window);
}

void Photino::Center()
{
	gint windowWidth, windowHeight;
	gtk_window_get_size(GTK_WINDOW(_window), &windowWidth, &windowHeight);

	GdkRectangle screen = {0};

	GdkDisplay *d = gdk_display_get_default();
	if (d == NULL)
	{
		GtkWidget *dialog = gtk_message_dialog_new(
			nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "gdk_display_get_default() returned NULL");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}

	GdkMonitor *m = gdk_display_get_primary_monitor(d);
	if (m == NULL)
	{
		m = gdk_display_get_monitor(d, 0); // Attempt to get the first monitor
        if (m == NULL)
        {
			GtkWidget *dialog = gtk_message_dialog_new(
				nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "gdk_display_get_primary_monitor() returned NULL");
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			return;
		}
	}

	gdk_monitor_get_geometry(m, &screen);

	gtk_window_move(GTK_WINDOW(_window),
					(screen.width - windowWidth) / 2,
					(screen.height - windowHeight) / 2);
}

void Photino::ClearBrowserAutoFill()
{
	// TODO
}

void Photino::Close()
{
	gtk_window_close(GTK_WINDOW(_window));
}

void Photino::GetTransparentEnabled(bool *enabled)
{
	*enabled = _transparentEnabled;
}

void Photino::GetContextMenuEnabled(bool *enabled)
{
	*enabled = _contextMenuEnabled;
}

void Photino::GetDevToolsEnabled(bool *enabled)
{
	WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(_webview));
	_devToolsEnabled = webkit_settings_get_enable_developer_extras(settings);
	*enabled = _devToolsEnabled;
}

void Photino::GetFullScreen(bool *fullScreen)
{
	*fullScreen = _isFullScreen;
}

void Photino::GetGrantBrowserPermissions(bool *grant)
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

void Photino::GetIgnoreCertificateErrorsEnabled(bool* enabled)
{
	*enabled = this->_ignoreCertificateErrorsEnabled;
}

void Photino::GetMaximized(bool *isMaximized)
{
	//gboolean maximized = gtk_window_is_maximized(GTK_WINDOW(_window));  //this method doesn't work
	//*isMaximized = maximized;
	GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(_window));
	GdkWindowState flags = gdk_window_get_state(gdk_window);
	*isMaximized = flags & GDK_WINDOW_STATE_MAXIMIZED;
}

void Photino::GetMinimized(bool *isMinimized)
{
	GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(_window));
	GdkWindowState flags = gdk_window_get_state(gdk_window);
	*isMinimized = flags & GDK_WINDOW_STATE_ICONIFIED;
}

void Photino::GetPosition(int *x, int *y)
{
	gtk_window_get_position(GTK_WINDOW(_window), x, y);
}

void Photino::GetResizable(bool *resizable)
{
	*resizable = gtk_window_get_resizable(GTK_WINDOW(_window));
}

unsigned int Photino::GetScreenDpi()
{
	GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(_window));
	gdouble dpi = gdk_screen_get_resolution(screen);
	if (dpi < 0)
		return 96;
	else
		return (unsigned int)dpi;
}

void Photino::GetSize(int *width, int *height)
{
	gtk_window_get_size(GTK_WINDOW(_window), width, height);

	// TODO: When calling set height, then set width...
	// calling set size works fine.
	// Uncomment this and it works properly. Commented, it only changes width.
	// GtkWidget* dialog = gtk_message_dialog_new(
	// 	nullptr
	// 	, GTK_DIALOG_DESTROY_WITH_PARENT
	// 	, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE
	// 	, "width: %i bytes, height %i"
	// 	, *width
	// 	, *height);
	// gtk_dialog_run(GTK_DIALOG(dialog));
	// gtk_widget_destroy(dialog);
}

AutoString Photino::GetTitle()
{
	return (AutoString)gtk_window_get_title(GTK_WINDOW(_window));
}

void Photino::GetTopmost(bool *topmost)
{
	// TODO: This flag is not set in GDK3. WebKit does not support GTK5 yet.
	GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(_window));
	GdkWindowState flags = gdk_window_get_state(gdk_window);
	*topmost = flags & GDK_WINDOW_STATE_ABOVE;

	// char tmp1[FMT_BUF_SIZE];
	// char tmp2[FMT_BUF_SIZE];
	// char tmp3[FMT_BUF_SIZE];
	// GtkWidget* dialog = gtk_message_dialog_new(
	//	nullptr
	//	, GTK_DIALOG_DESTROY_WITH_PARENT
	//	, GTK_MESSAGE_ERROR
	//	, GTK_BUTTONS_CLOSE
	//	, "flags: %s \n above: %s \n and: %s \n topmost: %s"
	//	, binary_fmt(flags, tmp1)
	//	, binary_fmt(GDK_WINDOW_STATE_ABOVE, tmp2)
	//	, binary_fmt(flags & GDK_WINDOW_STATE_ABOVE, tmp3)
	//	, *topmost ? "T" : "F");
	// gtk_dialog_run(GTK_DIALOG(dialog));
	// gtk_widget_destroy(dialog);
}

void Photino::GetZoom(int *zoom)
{
	double rawValue = 0;
	rawValue = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(_webview));
	rawValue = (rawValue * 100.0) + 0.5;
	*zoom = (int)rawValue;
}

void Photino::NavigateToString(AutoString content)
{
	webkit_web_view_load_html(WEBKIT_WEB_VIEW(_webview), content, NULL);
}

void Photino::NavigateToUrl(AutoString url)
{
	webkit_web_view_load_uri(WEBKIT_WEB_VIEW(_webview), url);
}

void Photino::Restore()
{
	gtk_window_present(GTK_WINDOW(_window));
}

// From https://stackoverflow.com/a/33799784
std::string escape_json(const std::string &s)
{
	std::ostringstream o;
	for (auto c = s.cbegin(); c != s.cend(); c++)
	{
		switch (*c)
		{
		case '"':
			o << "\\\"";
			break;
		case '\\':
			o << "\\\\";
			break;
		case '\b':
			o << "\\b";
			break;
		case '\f':
			o << "\\f";
			break;
		case '\n':
			o << "\\n";
			break;
		case '\r':
			o << "\\r";
			break;
		case '\t':
			o << "\\t";
			break;
		default:
			if ('\x00' <= *c && *c <= '\x1f')
			{
				o << "\\u"
				  << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
			}
			else
			{
				o << *c;
			}
		}
	}
	return o.str();
}

static void webview_eval_finished(GObject *object, GAsyncResult *result, gpointer userdata)
{
	InvokeJSWaitInfo *waitInfo = (InvokeJSWaitInfo *)userdata;
	waitInfo->isCompleted = true;
}

void Photino::SendWebMessage(AutoString message)
{
	std::string js;
	js.append("__dispatchMessageCallback(\"");
	js.append(escape_json(message));
	js.append("\")");

	InvokeJSWaitInfo invokeJsWaitInfo = {};

	webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(_webview), js.c_str(), NULL, webview_eval_finished, &invokeJsWaitInfo);
	// Todo: Replace deprecated webkit_web_view_run_javascript webkit_web_view_evaluate_javascript

	while (!invokeJsWaitInfo.isCompleted)
	{
		g_main_context_iteration(NULL, TRUE);
	}
}

void Photino::SetContextMenuEnabled(bool enabled)
{
	_contextMenuEnabled = enabled;
}

void Photino::SetDevToolsEnabled(bool enabled)
{
	_devToolsEnabled = enabled;
	WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(_webview));
	webkit_settings_set_enable_developer_extras(settings, _devToolsEnabled);
}

void Photino::SetFullScreen(bool fullScreen)
{
	if (fullScreen)
		gtk_window_fullscreen(GTK_WINDOW(_window));
	else
		gtk_window_unfullscreen(GTK_WINDOW(_window));

	_isFullScreen = fullScreen;
}

void Photino::SetIconFile(AutoString filename)
{
	gtk_window_set_icon_from_file(GTK_WINDOW(_window), filename, NULL);
}

void Photino::SetMinimized(bool minimized)
{
	if (minimized)
		gtk_window_iconify(GTK_WINDOW(_window));
	else
		gtk_window_deiconify(GTK_WINDOW(_window));
}

void Photino::SetMaximized(bool maximized)
{
	_isFullScreen = maximized;
	if (maximized)
		gtk_window_maximize(GTK_WINDOW(_window));
	else
		gtk_window_unmaximize(GTK_WINDOW(_window));
}

void Photino::SetPosition(int x, int y)
{
	gtk_window_move(GTK_WINDOW(_window), x, y);
}

void Photino::SetResizable(bool resizable)
{
	gtk_window_set_resizable(GTK_WINDOW(_window), resizable);
}

void Photino::SetMinSize(int width, int height)
{
    _hints.min_width = width;
    _hints.min_height = height;

    gtk_window_set_geometry_hints(
		GTK_WINDOW(_window),
		NULL,
		&_hints,
		(GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
}

void Photino::SetMaxSize(int width, int height)
{	
    _hints.max_width = width;
    _hints.max_height = height;

    gtk_window_set_geometry_hints(
		GTK_WINDOW(_window),
		NULL,
		&_hints,
		(GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
}

void Photino::SetSize(int width, int height)
{
	gtk_window_resize(GTK_WINDOW(_window), width, height);
}

void Photino::SetTitle(AutoString title)
{
	gtk_window_set_title(GTK_WINDOW(_window), title);
}

void Photino::SetTopmost(bool topmost)
{
	gtk_window_set_keep_above(GTK_WINDOW(_window), topmost);
}

void Photino::SetZoom(int zoom)
{
	double newZoom = zoom / 100.0;
	webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(_webview), newZoom);
}

void Photino::SetTransparentEnabled(bool enabled)
{
	_transparentEnabled = enabled;

	gtk_window_set_decorated(GTK_WINDOW(_window), !enabled);	//hide/show window chrome

	GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(_window));
	GdkVisual* rgba_visual = gdk_screen_get_rgba_visual(screen);
	if (rgba_visual)
	{
		gtk_widget_set_visual(GTK_WIDGET(_window), rgba_visual);
		gtk_widget_set_app_paintable(GTK_WIDGET(_window), true);

		GdkRGBA color;
		webkit_web_view_get_background_color(WEBKIT_WEB_VIEW(_webview), &color);
		if (enabled)
			color.alpha = 0;
		else
			color.alpha = 1;

		webkit_web_view_set_background_color(WEBKIT_WEB_VIEW(_webview), &color);
	}
}

void Photino::ShowNotification(AutoString title, AutoString message)
{
	NotifyNotification *notification = notify_notification_new(title, message, nullptr);
	notify_notification_set_icon_from_pixbuf(notification, gtk_window_get_icon(GTK_WINDOW(_window)));
	notify_notification_show(notification, NULL);
	g_object_unref(G_OBJECT(notification));
}

void Photino::WaitForExit()
{
	gtk_main();
}

// Callbacks
void Photino::GetAllMonitors(GetAllMonitorsCallback callback)
{
	if (callback)
	{
		GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(_window));
		GdkDisplay *display = gdk_screen_get_display(screen);
		int n = gdk_display_get_n_monitors(display);
		for (int i = 0; i < n; i++)
		{
			GdkMonitor *monitor = gdk_display_get_monitor(display, i);
			Monitor props = {};
			gdk_monitor_get_geometry(monitor, (GdkRectangle *)&props.monitor);
			gdk_monitor_get_workarea(monitor, (GdkRectangle *)&props.work);
			props.scale = gdk_monitor_get_scale_factor(monitor); // TODO: fractional scaling
			if (!callback(&props))
				break;
		}
	}
}

static gboolean invokeCallback(gpointer data)
{
	InvokeWaitInfo *waitInfo = (InvokeWaitInfo *)data;
	waitInfo->callback();
	{
		std::lock_guard<std::mutex> guard(invokeLockMutex);
		waitInfo->isCompleted = true;
	}
	waitInfo->completionNotifier.notify_one();
	return false;
}

void Photino::Invoke(ACTION callback)
{
	InvokeWaitInfo waitInfo = {};
	waitInfo.callback = callback;
	gdk_threads_add_idle(invokeCallback, &waitInfo);

	// Block until the callback is actually executed and completed
	// TODO: Add return values, exception handling, etc.
	std::unique_lock<std::mutex> uLock(invokeLockMutex);
	waitInfo.completionNotifier.wait(uLock, [&]
									 { return waitInfo.isCompleted; });
}

// Private methods
void HandleWebMessage(WebKitUserContentManager *contentManager, WebKitJavascriptResult *jsResult, gpointer arg)
{
	JSCValue *jsValue = webkit_javascript_result_get_js_value(jsResult);
	if (jsc_value_is_string(jsValue))
	{
		AutoString str_value = jsc_value_to_string(jsValue);

		WebMessageReceivedCallback callback = (WebMessageReceivedCallback)arg;
		callback(str_value);
		g_free(str_value);
	}

	webkit_javascript_result_unref(jsResult);
}

void Photino::Show(bool isAlreadyShown)
{
	if (!_webview)
	{
		struct sigaction old_action;
		sigaction(SIGCHLD, NULL, &old_action);
		WebKitUserContentManager *contentManager = webkit_user_content_manager_new();
		_webview = webkit_web_view_new_with_user_content_manager(contentManager);

		Photino::set_webkit_settings();

		// this may or may not work
		// g_object_set(G_OBJECT(settings), "enable-auto-fill-form", TRUE, NULL);

		gtk_container_add(GTK_CONTAINER(_window), _webview);
		
		WebKitUserScript *script = webkit_user_script_new(
			"window.__receiveMessageCallbacks = [];"
			"window.__dispatchMessageCallback = function(message) {"
			"	window.__receiveMessageCallbacks.forEach(function(callback) { callback(message); });"
			"};"
			"window.external = {"
			"	sendMessage: function(message) {"
			"		window.webkit.messageHandlers.Photinointerop.postMessage(message);"
			"	},"
			"	receiveMessage: function(callback) {"
			"		window.__receiveMessageCallbacks.push(callback);"
			"	}"
			"};",
			WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES, WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, NULL, NULL);
		webkit_user_content_manager_add_script(contentManager, script);
		webkit_user_script_unref(script);

		g_signal_connect(contentManager, "script-message-received::Photinointerop",
						 G_CALLBACK(HandleWebMessage), (void *)_webMessageReceivedCallback);
		webkit_user_content_manager_register_script_message_handler(contentManager, "Photinointerop");

		if (_startUrl != NULL)
			Photino::NavigateToUrl(_startUrl);
		else if (_startString != NULL)
			Photino::NavigateToString(_startString);
		else
		{
			GtkWidget *dialog = gtk_message_dialog_new(
				nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Neither StartUrl not StartString was specified");
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			exit(0);
		}
		sigaction(SIGCHLD, &old_action, NULL);
	}

	gtk_widget_show_all(_window);
}

void Photino::set_webkit_settings()
{
	// Rely on webkit_settings_new_with_settings to set the default settings
	// instead of using the webkit2gtk API to set the properties.
	// https://webkitgtk.org/reference/webkit2gtk/2.40.1/ctor.Settings.new_with_settings.html
	WebKitSettings* settings = webkit_settings_new_with_settings(
		// Set Photino-specific default settings
		"allow_modal_dialogs", TRUE,											// default: FALSE
		"allow_top_navigation_to_data_urls", TRUE,								// default: FALSE
		"allow_universal_access_from_file_urls", TRUE,							// default: FALSE
		"enable_back_forward_navigation_gestures", TRUE,						// default: FALSE
		"enable_media_capabilities", TRUE,										// default: FALSE
		"enable_mock_capture_devices", TRUE,									// default: FALSE
		"enable_page_cache", TRUE,												// default: FALSE
		"enable_webrtc", TRUE,													// default: FALSE
		"javascript_can_open_windows_automatically", TRUE,						// default: FALSE
		
		// Set user-defined settings
		"allow_file_access_from_file_urls", _fileSystemAccessEnabled,			// default: FALSE
		"disable_web_security", !_webSecurityEnabled,							// default: FALSE
		"enable_developer_extras", _devToolsEnabled,							// default: FALSE
		"enable_media_stream", _mediaStreamEnabled,								// default: FALSE
		"enable_smooth_scrolling", _smoothScrollingEnabled, 					// default: TRUE
		"javascript_can_access_clipboard", _javascriptClipboardAccessEnabled,	// default: FALSE
		"media_playback_requires_user_gesture", _mediaAutoplayEnabled,			// default: FALSE
		"user_agent", _userAgent,												// default: None
		
		// Other available settings for reference
		// "default_charset", "iso-8859-1",										// default: iso-8859-1
		// "cursive_font_family", "serif",										// default: serif
		// "default_font_family", "sans-serif",									// default: sans-serif	
		// "fantasy_font_family", "serif",										// default: serif
		// "monospace_font_family", "monospace",								// default: monospace
		// "pictograph_font_family", "serif",									// default: serif
		// "sans_serif_font_family", "sans-serif",								// default: sans-serif
		// "minimum_font_size", 0,												// default: 0
		// "default_font_size", 16,												// default: 16
		// "default_monospace_font_size", 13,									// default: 13
		// "auto_load_images", TRUE,											// default: TRUE
		// "enable_fullscreen", TRUE,											// default: TRUE
		// "enable_html5_database", TRUE,										// default: TRUE
		// "enable_html5_local_storage", TRUE,									// default: TRUE
		// "enable_hyperlink_auditing", TRUE,									// default: TRUE
		// "enable_javascript", TRUE,											// default: TRUE
		// "enable_javascript_markup", TRUE,									// default: TRUE
		// "enable_media", TRUE,												// default: TRUE
		// "enable_mediasource", TRUE,											// default: TRUE
		// "enable_offline_web_application_cache", TRUE,						// default: TRUE
		// "enable_resizable_text_areas", TRUE,									// default: TRUE
		// "enable_site_specific_quirks", TRUE,									// default: TRUE
		// "enable_tabs_to_links", TRUE,										// default: TRUE
		// "enable_webaudio", TRUE,												// default: TRUE
		// "enable_webgl", TRUE,												// default: TRUE
		// "enable_xss_auditor", TRUE,											// default: TRUE
		// "media_playback_allows_inline", TRUE,								// default: TRUE
		// "print_backgrounds", TRUE,											// default: TRUE
		// "draw_compositing_indicators", FALSE,								// default: FALSE
		// "enable_accelerated_2d_canvas", FALSE,								// default: FALSE
		// "enable_caret_browsing", FALSE,										// default: FALSE
		// "enable_dns_prefetching", FALSE,										// default: FALSE
		// "enable_encrypted_media", FALSE,										// default: FALSE
		// "enable_frame_flattening", FALSE,									// default: FALSE
		// "enable_java", FALSE,												// default: FALSE
		// "enable_plugins", FALSE,												// default: FALSE
		// "enable_private_browsing", FALSE,									// default: FALSE
		// "enable_spatial_navigation", FALSE,									// default: FALSE
		// "enable_write_console_messages_to_stdout", FALSE,					// default: FALSE
		// "load_icons_ignoring_image_load_setting", FALSE,						// default: FALSE
		// "zoom_text_only", FALSE, 											// default: FALSE
		// "media_content_types_requiring_hardware_support", None,				// default: None
		// "hardware_acceleration_policy", WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS,	// default: WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS
		NULL); // NULL terminates the list

	if (_browserControlInitParameters != NULL && strlen(_browserControlInitParameters) > 0)
		Photino::set_webkit_customsettings(settings);		//if any custom init parameters were passed, set them now.

	WebKitWebsiteDataManager* manager = webkit_web_view_get_website_data_manager(WEBKIT_WEB_VIEW(_webview));
	if (_ignoreCertificateErrorsEnabled)
		webkit_website_data_manager_set_tls_errors_policy(manager, WEBKIT_TLS_ERRORS_POLICY_IGNORE);
	else
		webkit_website_data_manager_set_tls_errors_policy(manager, WEBKIT_TLS_ERRORS_POLICY_FAIL);

	webkit_web_view_set_settings(WEBKIT_WEB_VIEW(_webview), settings);			//apply the settings to the webview
}

void Photino::set_webkit_customsettings(WebKitSettings* settings)
{
	//parse the JSON out of _browserControlInitParameters
	json data = json::parse(_browserControlInitParameters);
	for (auto it = data.begin(); it != data.end(); ++it)
	{
		json key = it.key();
		json value = it.value();

		// Use g_object_set_property to set the property on the settings object
		// instead of relying on the webkit2gtk API to set the properties.
		// https://docs.gtk.org/gobject/method.Object.set_property.html
        gchar* propertyName = g_strdup(key.get<std::string>().c_str());
        GValue* propertyValue = g_new0(GValue, 1);

        if (value.is_string())
		{
            g_value_init(propertyValue, G_TYPE_STRING);
            g_value_set_string(propertyValue, value.get<std::string>().c_str());
		}
		else if (value.is_boolean())
		{
            g_value_init(propertyValue, G_TYPE_BOOLEAN);
            g_value_set_boolean(propertyValue, value.get<bool>());
		}
		else if (value.is_number_integer())
		{
            g_value_init(propertyValue, G_TYPE_INT);
            g_value_set_int(propertyValue, value.get<int>());
		}
		else if (value.is_number_float())
		{
            g_value_init(propertyValue, G_TYPE_BOOLEAN);
            g_value_set_double(propertyValue, value.get<double>());
		}
		else
		{
			// Throw an error
			GtkWidget* dialog = gtk_message_dialog_new(
				nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Invalid value type for key: %s", propertyName);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			exit(0);
		}

    	g_object_set_property(G_OBJECT(settings), propertyName, propertyValue);

        g_value_unset(propertyValue);
        g_free(propertyValue);
        g_free(propertyName);
	}
}

gboolean on_configure_event(GtkWidget *widget, GdkEvent *event, gpointer self)
{
	if (event->type == GDK_CONFIGURE)
	{
		Photino *instance = ((Photino *)self);

		if (instance->_lastLeft != event->configure.x || instance->_lastTop != event->configure.y)
		{
			instance->InvokeMove(event->configure.x, event->configure.y);
			instance->_lastLeft = event->configure.x;
			instance->_lastTop = event->configure.y;
		}

		if (instance->_lastHeight != event->configure.height || instance->_lastWidth != event->configure.width)
		{
			instance->InvokeResize(event->configure.width, event->configure.height);
			instance->_lastWidth = event->configure.width;
			instance->_lastHeight = event->configure.height;
		}
	}
	return FALSE;
}

gboolean on_window_state_event(GtkWidget *widget, GdkEventWindowState *event, gpointer self)
{
	Photino *instance = ((Photino *)self);
	if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)
	{
		instance->InvokeMaximized();
	}
	else if ((event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) || !gtk_widget_get_mapped(instance->_window))
	{
		instance->InvokeMinimized();
	}
	else if (!(event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) && !(event->new_window_state & GDK_WINDOW_STATE_ICONIFIED))
	{
		instance->InvokeRestored();
	}
	return TRUE;
}

gboolean on_widget_deleted(GtkWidget *widget, GdkEvent *event, gpointer self)
{
	Photino *instance = ((Photino *)self);
	return instance->InvokeClose();
}

gboolean on_focus_in_event(GtkWidget *widget, GdkEvent *event, gpointer self)
{
	Photino *instance = ((Photino *)self);
	instance->InvokeFocusIn();
	return FALSE;
}

gboolean on_focus_out_event(GtkWidget *widget, GdkEvent *event, gpointer self)
{
	Photino *instance = ((Photino *)self);
	instance->InvokeFocusOut();
	return FALSE;
}

gboolean on_webview_context_menu(WebKitWebView *web_view, GtkWidget *default_menu,
								 WebKitHitTestResult *hit_test_result, gboolean triggered_with_keyboard, gpointer self)
{
	Photino *instance = ((Photino *)self);
	return !instance->_contextMenuEnabled;
}

gboolean on_permission_request(WebKitWebView *web_view, WebKitPermissionRequest *request, gpointer user_data)
{
	//GtkWidget *dialog = gtk_message_dialog_new(
	//	nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Permission Requested - Allowing!");
	//gtk_dialog_run(GTK_DIALOG(dialog));
	// gtk_widget_destroy(dialog);

	webkit_permission_request_allow(request);
	return FALSE;
}

void HandleCustomSchemeRequest(WebKitURISchemeRequest *request, gpointer user_data)
{
	WebResourceRequestedCallback webResourceRequestedCallback = (WebResourceRequestedCallback)user_data;

	const gchar *uri = webkit_uri_scheme_request_get_uri(request);
	int numBytes;
	AutoString contentType;
	void *dotNetResponse = webResourceRequestedCallback((AutoString)uri, &numBytes, &contentType);
	GInputStream *stream = g_memory_input_stream_new_from_data(dotNetResponse, numBytes, NULL);
	webkit_uri_scheme_request_finish(request, (GInputStream *)stream, -1, contentType);
	g_object_unref(stream);
	delete[] contentType;
}

void Photino::AddCustomSchemeHandlers()
{
	WebKitWebContext *context = webkit_web_context_get_default();
	for (const auto &value : _customSchemeNames)
	{
		webkit_web_context_register_uri_scheme(
			context, value, (WebKitURISchemeRequestCallback)HandleCustomSchemeRequest, (void *)_customSchemeCallback, NULL);
	}
}

#endif
