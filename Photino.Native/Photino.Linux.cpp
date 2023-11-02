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

	Photino::Show();

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
	gdk_monitor_get_geometry(gdk_display_get_primary_monitor(gdk_display_get_default()), &screen);

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

void Photino::GetContextMenuEnabled(bool *enabled)
{
	if (_contextMenuEnabled)
		*enabled = true;
}

void Photino::GetDevToolsEnabled(bool *enabled)
{
	WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(_webview));
	_devToolsEnabled = webkit_settings_get_enable_developer_extras(settings);
	if (_devToolsEnabled)
		*enabled = true;
}

void Photino::GetFullScreen(bool *fullScreen)
{
	*fullScreen = _isFullScreen;
}

void Photino::GetGrantBrowserPermissions(bool *grant)
{
	if (_grantBrowserPermissions)
		*grant = true;
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

void Photino::GetMaximized(bool *isMaximized)
{
	*isMaximized = gtk_window_is_maximized(GTK_WINDOW(_window));
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

	// TODO: Uncomment this and it works properly. Commented, it only changes width.
	// GtkWidget* dialog = gtk_message_dialog_new(
	//	nullptr
	//	, GTK_DIALOG_DESTROY_WITH_PARENT
	//	, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE
	//	, "width: %i bytes, height %i"
	//	, *width
	//	, *height);
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
	webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(_webview),
								   js.c_str(), NULL, webview_eval_finished, &invokeJsWaitInfo);
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
	GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(_window));
	gdk_window_set_keep_above(gdk_window, topmost);
}

void Photino::SetZoom(int zoom)
{
	double newZoom = zoom / 100.0;
	webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(_webview), newZoom);
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

void Photino::Show()
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
	//https://webkitgtk.org/reference/webkit2gtk/2.5.1/WebKitSettings.html
	//https://lazka.github.io/pgi-docs/WebKit2-4.0/classes/Settings.html
	WebKitSettings* settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(_webview));

	webkit_settings_set_allow_file_access_from_file_urls(settings, _fileSystemAccessEnabled);//webkit default: False
	webkit_settings_set_allow_modal_dialogs(settings, TRUE);							//webkit default: False
	webkit_settings_set_allow_top_navigation_to_data_urls(settings, TRUE);				//webkit default: False
	webkit_settings_set_allow_universal_access_from_file_urls(settings, TRUE);			//webkit default: False
	//webkit_settings_set_auto_load_images(settings, TRUE);							//webkit default: True
	//webkit_settings_set_cursive_font_family(settings, "serif");					//webkit default: "serif"
	//webkit_settings_set_default_charset(settings, "iso-8859-1");					//webkit default: "iso-8859-1"
	//webkit_settings_set_default_font_family(settings, "sans-serif");				//webkit default: "sans-serif"	
	//webkit_settings_set_default_font_size(settings, 16);							//webkit default: 16
	//webkit_settings_set_default_monospace_font_size(settings, 13);					//webkit default: 13
	webkit_settings_set_disable_web_security(settings, !_webSecurityEnabled);			//webkit default: False
	//webkit_settings_set_draw_compositing_indicators(settings, FALSE);				//webkit default: False
	//webkit_settings_set_enable_accelerated_2d_canvas(settings, FALSE);				//webkit default: False
	webkit_settings_set_enable_back_forward_navigation_gestures(settings, TRUE);		//webkit default: False
	//webkit_settings_set_enable_caret_browsing(settings, FALSE);						//webkit default: False
	webkit_settings_set_enable_developer_extras(settings, _devToolsEnabled);			//webkit default: False
	//webkit_settings_set_enable_dns_prefetching(settings, FALSE);						//webkit default: False
	//webkit_settings_set_enable_encrypted_media(settings, FALSE);					//webkit default: False
	//webkit_settings_set_enable_frame_flattening(settings, FALSE);					//webkit default: False
	//webkit_settings_set_enable_fullscreen(settings, TRUE);							//webkit default: True
	//webkit_settings_set_enable_html5_database(settings, TRUE);						//webkit default: True
	//webkit_settings_set_enable_html5_local_storage(settings, TRUE);				//webkit default: True
	//webkit_settings_set_enable_hyperlink_auditing(settings, TRUE);					//webkit default: True
	//webkit_settings_set_enable_java(settings, FALSE);								//webkit default: False
	//webkit_settings_set_enable_javascript(settings, TRUE);							//webkit default: True
	//webkit_settings_set_enable_javascript_markup(settings, TRUE);					//webkit default: True
	webkit_settings_set_enable_media(settings, TRUE);								//webkit default: True
	webkit_settings_set_enable_media_capabilities(settings, TRUE);						//webkit default: False
	webkit_settings_set_enable_media_stream(settings, _mediaStreamEnabled);				//webkit default: False
	webkit_settings_set_enable_mediasource(settings, TRUE);						//webkit default: True
	//webkit_settings_set_enable_mock_capture_devices(settings, TRUE);				//webkit default: False
	//webkit_settings_set_enable_offline_web_application_cache(settings, TRUE);		//webkit default: True
	//webkit_settings_set_enable_page_cache(settings, TRUE);							//webkit default: False
	//webkit_settings_set_enable_plugins(settings, FALSE);							//webkit default: False
	//webkit_settings_set_enable_private_browsing(settings, FALSE);					//webkit default: False
	//webkit_settings_set_enable_resizable_text_areas(settings, TRUE);				//webkit default: True
	//webkit_settings_set_enable_site_specific_quirks(settings, TRUE);				//webkit default: True
	webkit_settings_set_enable_smooth_scrolling(settings, _smoothScrollingEnabled);		//webkit default: True
	//webkit_settings_set_enable_spatial_navigation(settings, FALSE);				//webkit default: False
	//webkit_settings_set_enable_tabs_to_links(settings, TRUE);						//webkit default: True
	//webkit_settings_set_enable_webaudio(settings, TRUE);							//webkit default: True
	//webkit_settings_set_enable_webgl(settings, TRUE);								//webkit default: True
	webkit_settings_set_enable_webrtc(settings, TRUE);							//webkit default: False
	//webkit_settings_set_enable_write_console_messages_to_stdout(settings, FALSE);	//webkit default: False
	//webkit_settings_set_enable_xss_auditor(settings, TRUE);						//webkit default: True
	//webkit_settings_set_fantasy_font_family(settings, "serif");					//webkit default: "serif"
	//webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS);//webkit default: WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS
	webkit_settings_set_javascript_can_access_clipboard(settings, _javascriptClipboardAccessEnabled);//webkit default: False
	webkit_settings_set_javascript_can_open_windows_automatically(settings, TRUE);		//webkit default: False
	//webkit_settings_set_load_icons_ignoring_image_load_setting(settings, FALSE);	//webkit default: False
	//webkit_settings_set_media_content_types_requiring_hardware_support(settings, None);//webkit default: None
	webkit_settings_set_media_playback_allows_inline(settings, TRUE);				//webkit default: True
	webkit_settings_set_media_playback_requires_user_gesture(settings, _mediaAutoplayEnabled);//webkit default: False
	//webkit_settings_set_minimum_font_size(settings, 0);							//webkit default: 0
	//webkit_settings_set_monospace_font_family(settings, "monospace");				//webkit default: "monospace"
	//webkit_settings_set_pictograph_font_family(settings, "serif");					//webkit default: "serif"
	//webkit_settings_set_print_backgrounds(settings, TRUE);							//webkit default: True
	//webkit_settings_set_sans_serif_font_family(settings, "sans-serif");			//webkit default: "sans-serif"
	webkit_settings_set_user_agent(settings, _userAgent);								//webkit default: None
	//webkit_settings_set_zoom_text_only(settings, FALSE);								//webkit default: False

	if (_browserControlInitParameters != NULL && strlen(_browserControlInitParameters) > 0)
		Photino::set_webkit_customsettings(settings);		//if any custom init parameters were passed, set them now.
}

void Photino::set_webkit_customsettings(WebKitSettings* settings)
{
	//open the webkit2gtk library dynamically
	void* handle = dlopen("libwebkit2gtk-4.1.so", RTLD_LAZY);
	if (handle == NULL) {
		GtkWidget* dialog = gtk_message_dialog_new(
			nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Could not load libwebkit2gtk-4.1.so library.");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
	
	//parse the JSON out of _browserControlInitParameters
	json data = json::parse(_browserControlInitParameters);
	for (auto it = data.begin(); it != data.end(); ++it)
	{
		json key = it.key();
		std::string fName = (std::string)"webkit_settings_" + (std::string)key;
		char* functionName = (char*)fName.c_str();

		json value = it.value();

		if (value.is_boolean())
		{
			bool boolValue = value;

			void (*example_function)(WebKitSettings*, bool) = (void (*)(WebKitSettings*, bool))dlsym(handle, functionName);
			if (example_function == NULL) {
				GtkWidget* dialog = gtk_message_dialog_new(
					nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Could not dynamically load function %s.", functionName);
				gtk_dialog_run(GTK_DIALOG(dialog));
				gtk_widget_destroy(dialog);
			}

			example_function(settings, boolValue);
		}
		else if (value.is_number())
		{
			int intValue = value;

			void (*example_function)(WebKitSettings*, int) = (void (*)(WebKitSettings*, int))dlsym(handle, functionName);
			if (example_function == NULL) {
				GtkWidget* dialog = gtk_message_dialog_new(
					nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Could not dynamically load function %s.", functionName);
				gtk_dialog_run(GTK_DIALOG(dialog));
				gtk_widget_destroy(dialog);
			}

			example_function(settings, intValue);
		}
		else if (value.is_string())
		{
			char* stringValue = (char*)value.get<std::string>().c_str();

			//GtkWidget* dialog = gtk_message_dialog_new(
			//	nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Setting: %s  Value: %s", functionName, stringValue);
			//gtk_dialog_run(GTK_DIALOG(dialog));
			//gtk_widget_destroy(dialog);

			void (*example_function)(WebKitSettings*, char*) = (void (*)(WebKitSettings*, char*))dlsym(handle, functionName);
			if (example_function == NULL) {
				GtkWidget* dialog = gtk_message_dialog_new(
					nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Could not dynamically load function %s.", functionName);
				gtk_dialog_run(GTK_DIALOG(dialog));
				gtk_widget_destroy(dialog);
			}

			example_function(settings, stringValue);
		}
	}

	dlclose(handle);
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
