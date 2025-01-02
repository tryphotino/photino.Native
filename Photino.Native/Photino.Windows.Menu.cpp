#include <algorithm>
#include <codecvt>
#include <locale>
#include <string>
#include <functional>
#include "Photino.Windows.Menu.h"

// -------------------------------------------------------------------------------------------------
// Variables
// -------------------------------------------------------------------------------------------------

static std::atomic<int> lastMenuItemId;
static std::atomic<int> lastWindowClassId;
MenuRenderer menuRenderer{};

// -------------------------------------------------------------------------------------------------
// Utilities
// -------------------------------------------------------------------------------------------------

static std::wstring GenerateClassName()
{
	return L"Win32Window" + std::to_wstring(lastWindowClassId++);
}

static std::unique_ptr<std::remove_pointer<HMENU>::type, Win32MenuDeleter> Win32_CreateMenu()
{
	auto result = CreatePopupMenu();

	if (!result)
	{
		throw Win32Exception{};
	}

	return std::unique_ptr<std::remove_pointer<HMENU>::type, Win32MenuDeleter>(result);
}

static std::unique_ptr<std::remove_pointer<HWND>::type, Win32WindowDeleter> Win32_CreateWindow(
	const ATOM windowClass)
{
	auto result = CreateWindow(
		reinterpret_cast<LPCWSTR>(windowClass),
		nullptr,
		0,
		0,
		0,
		0,
		0,
		nullptr,
		nullptr,
		GetModuleHandle(nullptr),
		nullptr);

	if (!result)
	{
		throw Win32Exception{};
	}

	return std::unique_ptr<std::remove_pointer<HWND>::type, Win32WindowDeleter>(result);
}

static std::unique_ptr<std::remove_pointer<LPCWSTR>::type, Win32WindowClassDeleter>
Win32_CreateWindowClass(const WNDPROC wndProc)
{
	std::wstring className = GenerateClassName();

	WNDCLASSEX wndClass;

	wndClass.cbSize = sizeof(wndClass);
	wndClass.style = 0;
	wndClass.lpfnWndProc = wndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = GetModuleHandle(nullptr);
	wndClass.hIcon = nullptr;
	wndClass.hCursor = LoadIcon(nullptr, IDC_ARROW);
	wndClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = className.c_str();
	wndClass.hIconSm = nullptr;

	auto result = RegisterClassEx(&wndClass);

	if (!result)
	{
		throw Win32Exception{};
	}

	return std::unique_ptr<std::remove_pointer<LPCWSTR>::type, Win32WindowClassDeleter>(
		reinterpret_cast<LPCWSTR>(result),
		Win32WindowClassDeleter(wndClass.hInstance));
}

static std::string ToUTF8(const std::wstring& value)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(value);
}

static std::wstring ToUTF16(const std::string& value)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(value);
}

static std::string ToLastErrorString(const DWORD lastError)
{
	LPTSTR errorText = nullptr;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER
		| FORMAT_MESSAGE_FROM_SYSTEM
		| FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		lastError,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		reinterpret_cast<LPTSTR>(&errorText),
		0,
		nullptr);

	if (errorText == nullptr)
	{
		return "Failed to format win32 error.";
	}

	std::wstring errorString(errorText);
	LocalFree(errorText);
	return ToUTF8(errorString);
}

// -------------------------------------------------------------------------------------------------
// MenuRenderer
// -------------------------------------------------------------------------------------------------

static LRESULT MenuRendererWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	switch (msg)
	{
		case WM_CLOSE:
		{
			auto menuRenderer = reinterpret_cast<MenuRenderer*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));

			if (menuRenderer == nullptr)
			{
				break; // TODO: Log this error.
			}

			menuRenderer->_hWnd = nullptr;
			PostQuitMessage(0);
			return 0;
		}
		case WM_USER:
		{
			auto menuRenderer = reinterpret_cast<MenuRenderer*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));

			if (menuRenderer == nullptr)
			{
				break; // TODO: Log this error.
			}

			try
			{
				auto command = reinterpret_cast<std::function<void(HWND)>*>(lParam);
				(*command)(hWnd);
				delete command;
			}
			catch (std::exception)
			{
				// TODO: Log this error.
			}

			break;
		}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

MenuRenderer::MenuRenderer()
	: _activeMenu(NULL)
	, _thread(MenuRenderer::PumpMessages, this)
{
}

MenuRenderer::~MenuRenderer()
{
	{
		std::lock_guard<std::mutex> lock(_hWndMutex);
		PostMessage(_hWnd.get(), WM_CLOSE, 0, 0);
	}

	_thread.join();
}

void MenuRenderer::Destroy(Menu* menu)
{
	// First, post `DestroyMenuCommand` since `TrackPopupMenuEx` begins a
	// modal window and cannot process this message until the menu is
	// dismissed.

	PostMessage(
		_hWnd.get(),
		WM_USER,
		0,
		reinterpret_cast<LPARAM>(new std::function<void(HWND)>([=](HWND hwnd){ delete menu; })));

	// Then, once the message is queued, attempt to hide the menu if
	// it is currently being tracked. This frees up the event loop to
	// process the queued `DestroyMenuCommand`.

	Hide(menu);
}

void MenuRenderer::Hide(const Menu* menu)
{
	std::lock_guard<std::mutex> lock(_activeMenuMutex);

	if (_activeMenu == menu)
	{
		PostMessage(_hWnd.get(), WM_CANCELMODE, 0, 0);
	}
}

void MenuRenderer::PumpMessages(MenuRenderer* renderer) noexcept
{
	try
	{
		{
			std::lock_guard<std::mutex> lock(renderer->_hWndMutex);
			auto windowClass = Win32_CreateWindowClass(MenuRendererWndProc);
			auto window = Win32_CreateWindow(reinterpret_cast<ATOM>(windowClass.get()));
			SetWindowLongPtr(window.get(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(renderer));
			renderer->_hWnd = std::move(window);
		}

		while (true)
		{
			MSG msg;

			switch (GetMessage(&msg, nullptr, 0, 0))
			{
				case -1:
				{
					return; // TODO: Log this error.
				}
				case 0:
				{
					return;
				}
				default:
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
					break;
				}
			}
		}
	}
	catch (std::exception)
	{
		// TODO: Log this error.
	}
}

void MenuRenderer::Show(Menu* menu, const int x, const int y) const
{
	PostMessage(
		_hWnd.get(),
		WM_USER,
		0,
		reinterpret_cast<LPARAM>(new std::function<void(HWND)>([=](HWND hwnd) {
			{
				std::lock_guard<std::mutex> lock(menuRenderer._activeMenuMutex);
				menuRenderer._activeMenu = menu;
			}

			// Since our commands run on a different thread than the main window, we must bring our
			// menu thread to the foreground before tracking the popup menu to ensure that dismissal
			// works correctly.

			if (!SetForegroundWindow(hwnd))
			{
				// TODO: Log this error.
			}

			SetLastError(0);

			auto itemId = TrackPopupMenuEx(
				menu->_menu.get(),
				TPM_LEFTALIGN | TPM_RETURNCMD | TPM_TOPALIGN,
				x,
				y,
				hwnd,
				nullptr);

			{
				std::lock_guard<std::mutex> lock(menuRenderer._activeMenuMutex);
				menuRenderer._activeMenu = nullptr;
			}

			if (!itemId)
			{
				auto lastError = GetLastError();

				if (lastError)
				{
					throw Win32Exception(lastError);
				}

				menu->NotifyClicked(nullptr);
			}
			else
			{
				MENUITEMINFO info = {};

				info.cbSize = sizeof(info);
				info.fMask = MIIM_DATA;

				if (!GetMenuItemInfo(menu->_menu.get(), itemId, false, &info))
				{
					throw Win32Exception();
				}

				menu->NotifyClicked(reinterpret_cast<MenuItem*>(info.dwItemData));
			}
		})));
}

// -------------------------------------------------------------------------------------------------
// Menu
// -------------------------------------------------------------------------------------------------

Menu::Menu()
	: MenuNode()
	, _menu(Win32_CreateMenu())
{
}

void Menu::Add(MenuNode* node)
{
	MenuNode::Add(node);
	node->AddTo(_menu.get());
}

void Menu::AddTo(HMENU hmenu)
{
	throw std::logic_error("Menu is a top-level MenuNode.");
}

void Menu::AddOnClicked(const OnClickedCallback onClicked)
{
	const std::lock_guard<std::mutex> lock(_onClickedMutex);
	_onClicked.push_back(onClicked);
}

void Menu::Hide() const
{
	menuRenderer.Hide(this);
}

void Menu::NotifyClicked(MenuItem* menuItem)
{
	std::vector<OnClickedCallback> copied;

	{
		const std::lock_guard<std::mutex> lock(_onClickedMutex);
		copied = std::vector<OnClickedCallback>(_onClicked);
	}

	for (auto onClicked : copied)
	{
		onClicked(menuItem);
	}
}

void Menu::RemoveOnClicked(const OnClickedCallback onClicked)
{
	const std::lock_guard<std::mutex> lock(_onClickedMutex);
	auto index = std::find(_onClicked.begin(), _onClicked.end(), onClicked);

	if (index != _onClicked.end())
	{
		_onClicked.erase(index);
	}
}

void Menu::Show(const Photino* photino, const int x, const int y)
{
	POINT point =
	{
		x,
		y
	};

	if (!ClientToScreen(photino->getHwnd(), &point))
	{
		throw Win32Exception{};
	}

	menuRenderer.Show(this, point.x, point.y);
}

// -------------------------------------------------------------------------------------------------
// MenuItem
// -------------------------------------------------------------------------------------------------

MenuItem::MenuItem(MenuItemOptions options)
	: MenuNode()
	, _id(++lastMenuItemId)
	, _label(options.Label == nullptr
		? std::wstring()
		: ToUTF16(options.Label))
{
}

void MenuItem::Add(MenuNode* node)
{
	MenuNode::Add(node);

	if (_menu == nullptr)
	{
		_menu = Win32_CreateMenu();

		MENUITEMINFO info;

		info.cbSize = sizeof(info);
		info.fMask = MIIM_SUBMENU;
		info.hSubMenu = _menu.get();

		SetMenuItemInfo(
			_parentMenu,
			_id,
			false,
			&info);
	}

	node->AddTo(_menu.get());
}

void MenuItem::AddTo(HMENU hMenu)
{
	_parentMenu = hMenu;

	UINT uFlags = MF_STRING;
	UINT_PTR uIDNewItem = _id;
	LPCWSTR lpNewItem = const_cast<wchar_t*>(_label.c_str());

	if (!AppendMenu(hMenu, uFlags, uIDNewItem, lpNewItem))
	{
		throw Win32Exception();
	}

	MENUITEMINFO info = {};

	info.cbSize = sizeof(info);
	info.dwItemData = reinterpret_cast<ULONG_PTR>(this);
	info.fMask = MIIM_DATA | MIIM_SUBMENU;
	info.hSubMenu = _menu.get();

	SetMenuItemInfo(hMenu, _id, false, &info);
}

// -------------------------------------------------------------------------------------------------
// MenuNode
// -------------------------------------------------------------------------------------------------

MenuNode::MenuNode()
	: parent(nullptr)
{
}

void MenuNode::Add(MenuNode* node)
{
	node->parent = this;
	node->children.push_back(node);
}

void MenuNode::NotifyOnClicked(MenuItem* item)
{
}

// -------------------------------------------------------------------------------------------------
// MenuSeparator
// -------------------------------------------------------------------------------------------------

void MenuSeparator::Add(MenuNode* node)
{
	throw std::logic_error("Cannot add a MenuNode to a MenuSeparator.");
}

void MenuSeparator::AddTo(HMENU hMenu)
{
	if (!AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr))
	{
		throw Win32Exception();
	}
}

// -------------------------------------------------------------------------------------------------
// Win32BitmapDeleter
// -------------------------------------------------------------------------------------------------

void Win32BitmapDeleter::operator()(HBITMAP hBitmap) const noexcept
{
	if (hBitmap == nullptr)
	{
		return;
	}

	if (!DeleteObject(hBitmap))
	{
		// TODO: Log this error.
	}
}

// -------------------------------------------------------------------------------------------------
// Win32Exception
// -------------------------------------------------------------------------------------------------

Win32Exception::Win32Exception()
	: std::runtime_error(ToLastErrorString(GetLastError()))
{
}

Win32Exception::Win32Exception(DWORD lastError)
	: std::runtime_error(ToLastErrorString(lastError))
{
}

// -------------------------------------------------------------------------------------------------
// Win32MenuDeleter
// -------------------------------------------------------------------------------------------------

void Win32MenuDeleter::operator()(HMENU hMenu) const noexcept
{
	if (hMenu == nullptr)
	{
		return;
	}

	if (!DestroyMenu(hMenu))
	{
		// TODO: Log this error.
	}
}

// -------------------------------------------------------------------------------------------------
// Win32WindowDeleter
// -------------------------------------------------------------------------------------------------

void Win32WindowDeleter::operator()(HWND hWnd) const noexcept
{
	if (hWnd == nullptr)
	{
		return;
	}

	if (!DestroyWindow(hWnd))
	{
		// TODO: Log this error.
	}
}

// -------------------------------------------------------------------------------------------------
// Win32WindowClassDeleter
// -------------------------------------------------------------------------------------------------

Win32WindowClassDeleter::Win32WindowClassDeleter(HINSTANCE hInstance) noexcept
	: _hInstance(hInstance)
{
}

void Win32WindowClassDeleter::operator()(LPCWSTR atom) const noexcept
{
	if (atom == 0)
	{
		return;
	}

	if (!UnregisterClass(reinterpret_cast<LPCWSTR>(atom), _hInstance))
	{
		// TODO: Log this error.
	}
}