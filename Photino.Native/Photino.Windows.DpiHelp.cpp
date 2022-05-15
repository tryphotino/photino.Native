#include "Photino.Windows.DpiHelp.h"

// =============================================
//
// Types
//
// ===================
typedef UINT(WINAPI *_GetDpiForWindowFunc)(HWND hwnd);
typedef DPI_AWARENESS_CONTEXT(WINAPI *_SetThreadDpiAwarenessContextFunc)(DPI_AWARENESS_CONTEXT dpiContext);
typedef int(WINAPI *_GetSystemMetricsForDpiFunc)(int nIndex, UINT dpi);

// =============================================
//
// Forward declarations
//
// ===================
UINT FallbackGetDpiForWindow(HWND hwnd);
DPI_AWARENESS_CONTEXT FallbackSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT dpiContext);
int FallbackGetSystemMetricsForDpiFunc(int nIndex, UINT dpi);

// =============================================
//
// Statics
//
// ===================
static HMODULE s_user32 = nullptr;

static _GetDpiForWindowFunc _getDpiForWindow = FallbackGetDpiForWindow;
static _GetSystemMetricsForDpiFunc _getSystemMetricsForDpi = FallbackGetSystemMetricsForDpiFunc;
static _SetThreadDpiAwarenessContextFunc _setThreadDpiAwarenessContext = FallbackSetThreadDpiAwarenessContext;

// =============================================
//
// Helpers
//
// ===================
static UINT FallbackGetDpiForWindow(HWND hwnd)
{
	return 96;
}

static DPI_AWARENESS_CONTEXT FallbackSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT dpiContext)
{
	return nullptr;
}

static int FallbackGetSystemMetricsForDpiFunc(int nIndex, UINT dpi)
{
	return GetSystemMetrics(nIndex);
}

static void SetFallbacks()
{
	_getDpiForWindow = FallbackGetDpiForWindow;
	_getSystemMetricsForDpi = FallbackGetSystemMetricsForDpiFunc;
	_setThreadDpiAwarenessContext = FallbackSetThreadDpiAwarenessContext;
}

// =============================================	
//
// Exports
//
// ===================
void CloseDpiHelper()
{
	SetFallbacks();

	if (s_user32 != nullptr)
	{
		FreeLibrary(s_user32);
		s_user32 = nullptr;
	}
}

int GetScreenHeight(int dpi)
{
	return _getSystemMetricsForDpi(SM_CYSCREEN, dpi);
}

int GetScreenWidth(int dpi)
{
	return _getSystemMetricsForDpi(SM_CXSCREEN, dpi);
}

unsigned int GetWindowDpi(HWND hwnd)
{
	return _getDpiForWindow(hwnd);
}

void InitDpiHelper()
{
	SetFallbacks();
	
	s_user32 = LoadLibrary(L"User32.dll");
	if (s_user32 == nullptr)
	{
		return;
	}

	auto win10GetDpiForWindow = (_GetDpiForWindowFunc)GetProcAddress(s_user32, "GetDpiForWindow");
	if (win10GetDpiForWindow != nullptr)
	{
		_getDpiForWindow = win10GetDpiForWindow;
	}

	auto win10GetSystemMetricsForDpi = (_GetSystemMetricsForDpiFunc)GetProcAddress(s_user32, "GetSystemMetricsForDpi");
	if (win10GetSystemMetricsForDpi != nullptr)
	{
		_getSystemMetricsForDpi = win10GetSystemMetricsForDpi;
	}

	auto win10SetThreadDpiAwarenessContext = (_SetThreadDpiAwarenessContextFunc)GetProcAddress(s_user32, "SetThreadDpiAwarenessContext");
	if (win10SetThreadDpiAwarenessContext != nullptr)
	{
		auto oldVal = win10SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
		if (oldVal == nullptr)
		{
			CloseDpiHelper(); // failed init of the win10+ apis
			return;
		}

		_setThreadDpiAwarenessContext = win10SetThreadDpiAwarenessContext;
	}
}
