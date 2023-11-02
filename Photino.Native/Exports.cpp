#include "Photino.Dialog.h"
#include "Photino.h"

#ifdef _WIN32
#define EXPORTED __declspec(dllexport)
#else
#define EXPORTED
#endif

extern "C"
{
#ifdef _WIN32
	EXPORTED void Photino_register_win32(HINSTANCE hInstance)
	{
		Photino::Register(hInstance);
	}

	EXPORTED HWND Photino_getHwnd_win32(Photino* instance)
	{
		return instance->getHwnd();
	}

	EXPORTED void Photino_setWebView2RuntimePath_win32(Photino* instance, AutoString webView2RuntimePath)
	{
		Photino::SetWebView2RuntimePath(webView2RuntimePath);
	}
#elif __APPLE__
	EXPORTED void Photino_register_mac()
	{
		Photino::Register();
	}
#endif

	EXPORTED Photino* Photino_ctor(PhotinoInitParams* initParams)
	{
		return new Photino(initParams);
	}

	EXPORTED void Photino_dtor(Photino* instance)
	{
		delete instance;
	}

	EXPORTED void Photino_Center(Photino* instance)
	{
		instance->Center();
	}

	EXPORTED void Photino_ClearBrowserAutoFill(Photino* instance)
	{
		instance->ClearBrowserAutoFill();
	}

	EXPORTED void Photino_Close(Photino* instance)
	{
		instance->Close();
	}

	EXPORTED void Photino_GetContextMenuEnabled(Photino* instance, bool* enabled)
	{
		instance->GetContextMenuEnabled(enabled);
	}

	EXPORTED void Photino_GetDevToolsEnabled(Photino* instance, bool* enabled)
	{
		instance->GetDevToolsEnabled(enabled);
	}

	EXPORTED void Photino_GetFullScreen(Photino* instance, bool* fullScreen)
	{
		instance->GetFullScreen(fullScreen);
	}

	EXPORTED void Photino_GetGrantBrowserPermissions(Photino* instance, bool* grant)
	{
		instance->GetGrantBrowserPermissions(grant);
	}

	EXPORTED AutoString Photino_GetUserAgent(Photino* instance)
	{
		return instance->GetUserAgent();
	}

	EXPORTED void Photino_GetMediaAutoplayEnabled(Photino* instance, bool* enabled)
	{
		instance->GetMediaAutoplayEnabled(enabled);
	}

	EXPORTED void Photino_GetFileSystemAccessEnabled(Photino* instance, bool* enabled)
	{
		instance->GetFileSystemAccessEnabled(enabled);
	}

	EXPORTED void Photino_GetWebSecurityEnabled(Photino* instance, bool* enabled)
	{
		instance->GetWebSecurityEnabled(enabled);
	}

	EXPORTED void Photino_GetJavascriptClipboardAccessEnabled(Photino* instance, bool* enabled)
	{
		instance->GetJavascriptClipboardAccessEnabled(enabled);
	}

	EXPORTED void Photino_GetMediaStreamEnabled(Photino* instance, bool* enabled)
	{
		instance->GetMediaStreamEnabled(enabled);
	}

	EXPORTED void Photino_GetSmoothScrollingEnabled(Photino* instance, bool* enabled)
	{
		instance->GetSmoothScrollingEnabled(enabled);
	}

	EXPORTED void Photino_GetMaximized(Photino* instance, bool* isMaximized)
	{
		instance->GetMaximized(isMaximized);
	}

	EXPORTED void Photino_GetMinimized(Photino* instance, bool* isMinimized)
	{
		instance->GetMinimized(isMinimized);
	}

	EXPORTED void Photino_GetPosition(Photino* instance, int* x, int* y)
	{
		instance->GetPosition(x, y);
	}

	EXPORTED void Photino_GetResizable(Photino* instance, bool* resizable)
	{
		instance->GetResizable(resizable);
	}

	EXPORTED unsigned int Photino_GetScreenDpi(Photino* instance)
	{
		return instance->GetScreenDpi();
	}

	EXPORTED void Photino_GetSize(Photino* instance, int* width, int* height)
	{
		instance->GetSize(width, height);
	}

	EXPORTED AutoString Photino_GetTitle(Photino* instance)
	{
		return instance->GetTitle();
	}

	EXPORTED void Photino_GetTopmost(Photino* instance, bool* topmost)
	{
		instance->GetTopmost(topmost);
	}

	EXPORTED void Photino_GetZoom(Photino* instance, int* zoom)
	{
		instance->GetZoom(zoom);
	}

	EXPORTED void Photino_NavigateToString(Photino* instance, AutoString content)
	{
		instance->NavigateToString(content);
	}

	EXPORTED void Photino_NavigateToUrl(Photino* instance, AutoString url)
	{
		instance->NavigateToUrl(url);
	}

	EXPORTED void Photino_Restore(Photino* instance)
	{
		instance->Restore();
	}

	EXPORTED void Photino_SendWebMessage(Photino* instance, AutoString message)
	{
		instance->SendWebMessage(message);
	}

	EXPORTED void Photino_SetContextMenuEnabled(Photino* instance, bool enabled)
	{
		instance->SetContextMenuEnabled(enabled);
	}

	EXPORTED void Photino_SetDevToolsEnabled(Photino* instance, bool enabled)
	{
		instance->SetDevToolsEnabled(enabled);
	}

	EXPORTED void Photino_SetFullScreen(Photino* instance, bool fullScreen)
	{
		instance->SetFullScreen(fullScreen);
	}

	EXPORTED void Photino_SetIconFile(Photino* instance, AutoString filename)
	{
		instance->SetIconFile(filename);
	}

	EXPORTED void Photino_SetMaximized(Photino* instance, bool maximized)
	{
		instance->SetMaximized(maximized);
	}

	EXPORTED void Photino_SetMaxSize(Photino* instance, int width, int height)
	{
		instance->SetMaxSize(width, height);
	}

	EXPORTED void Photino_SetMinimized(Photino* instance, bool minimized)
	{
		instance->SetMinimized(minimized);
	}

	EXPORTED void Photino_SetMinSize(Photino* instance, int width, int height)
	{
		instance->SetMinSize(width, height);
	}

	EXPORTED void Photino_SetPosition(Photino* instance, int x, int y)
	{
		instance->SetPosition(x, y);
	}

	EXPORTED void Photino_SetResizable(Photino* instance, bool resizable)
	{
		instance->SetResizable(resizable);
	}

	EXPORTED void Photino_SetSize(Photino* instance, int width, int height)
	{
		instance->SetSize(width, height);
	}

	EXPORTED void Photino_SetTitle(Photino* instance, AutoString title)
	{
		instance->SetTitle(title);
	}

	EXPORTED void Photino_SetTopmost(Photino* instance, bool topmost)
	{
		instance->SetTopmost(topmost);
	}

	EXPORTED void Photino_SetZoom(Photino* instance, int zoom)
	{
		instance->SetZoom(zoom);
	}
	
	EXPORTED void Photino_ShowNotification(Photino* instance, AutoString title, AutoString body)
	{
		instance->ShowNotification(title, body);
	}

	EXPORTED void Photino_WaitForExit(Photino* instance)
	{
		instance->WaitForExit();
	}



	//Dialog
	EXPORTED AutoString* Photino_ShowOpenFile(Photino* inst, AutoString title, AutoString defaultPath, bool multiSelect, AutoString* filters, int filterCount, int* resultCount) {
		return inst->GetDialog()->ShowOpenFile(title, defaultPath, multiSelect, filters, filterCount, resultCount);
	}
	EXPORTED AutoString* Photino_ShowOpenFolder(Photino* inst, AutoString title, AutoString defaultPath, bool multiSelect, int* resultCount) {
		return inst->GetDialog()->ShowOpenFolder(title, defaultPath, multiSelect, resultCount);
	}
	EXPORTED AutoString Photino_ShowSaveFile(Photino* inst, AutoString title, AutoString defaultPath, AutoString* filters, int filterCount) {
		return inst->GetDialog()->ShowSaveFile(title, defaultPath, filters, filterCount);
	}
	EXPORTED DialogResult Photino_ShowMessage(Photino* inst, AutoString title, AutoString text, DialogButtons buttons, DialogIcon icon) {
		return inst->GetDialog()->ShowMessage(title, text, buttons, icon);
	}



	//Callbacks
	EXPORTED void Photino_AddCustomSchemeName(Photino* instance, AutoString scheme)
	{
		instance->AddCustomSchemeName(scheme);
	}

	EXPORTED void Photino_GetAllMonitors(Photino* instance, GetAllMonitorsCallback callback)
	{
		instance->GetAllMonitors(callback);
	}

	EXPORTED void Photino_SetClosingCallback(Photino* instance, ClosingCallback callback)
	{
		instance->SetClosingCallback(callback);
	}

	EXPORTED void Photino_SetFocusInCallback(Photino* instance, FocusInCallback callback)
	{
		instance->SetFocusInCallback(callback);
	}

	EXPORTED void Photino_SetFocusOutCallback(Photino* instance, FocusOutCallback callback)
	{
		instance->SetFocusOutCallback(callback);
	}

	EXPORTED void Photino_SetMovedCallback(Photino* instance, MovedCallback callback)
	{
		instance->SetMovedCallback(callback);
	}

	EXPORTED void Photino_SetResizedCallback(Photino* instance, ResizedCallback callback)
	{
		instance->SetResizedCallback(callback);
	}

	EXPORTED void Photino_Invoke(Photino* instance, ACTION callback)
	{
		instance->Invoke(callback);
	}
}
