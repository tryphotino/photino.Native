#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>
#include "Photino.h"

class Menu;
class MenuItem;
class MenuItemOptions;
class MenuNode;
class MenuRenderer;
class MenuSeparator;
class Win32Exception;

typedef void (*OnClickedCallback)(MenuItem* menuItem);

struct Win32BitmapDeleter final
{
	void operator()(HBITMAP hBitmap) const noexcept;
};

struct Win32MenuDeleter final
{
	void operator()(HMENU hMenu) const noexcept;
};

struct Win32WindowClassDeleter final
{
private:
	HINSTANCE _hInstance;
public:
	Win32WindowClassDeleter(HINSTANCE hInstance) noexcept;
	void operator()(LPCWSTR atom) const noexcept;
};

struct Win32WindowDeleter final
{
	void operator()(HWND hWnd) const noexcept;
};

struct MenuNode
{
	std::vector<MenuNode*> children;
	MenuNode* parent;
	MenuNode();
	virtual ~MenuNode() = default;
	virtual void Add(MenuNode* node);
	virtual void AddTo(HMENU hMenu) = 0;
	virtual void NotifyOnClicked(MenuItem* item);
};

class Menu final : public MenuNode
{
private:
	std::vector<MenuNode> _children;
	std::vector<OnClickedCallback> _onClicked;
	std::mutex _onClickedMutex;
public:
	std::unique_ptr<std::remove_pointer<HMENU>::type, Win32MenuDeleter> _menu;
	Menu();
	void Add(MenuNode* node) override;
	void AddTo(HMENU hmenu) override;
	void AddOnClicked(const OnClickedCallback onClicked);
	void Hide() const;
	void NotifyClicked(MenuItem* menuItem);
	void RemoveOnClicked(const OnClickedCallback onClicked);
	void Show(const Photino* window, const int x, const int y);
};

class MenuItem final : public MenuNode
{
private:
	int _id;
	std::wstring _label;
	std::unique_ptr<std::remove_pointer<HMENU>::type, Win32MenuDeleter> _menu;
	HMENU _parentMenu;
public:
	MenuItem(MenuItemOptions options);
	void Add(MenuNode* node) override;
	void AddTo(HMENU hMenu) override;
};

struct MenuItemOptions final
{
	char* Label;
};

class MenuRenderer final
{
	friend LRESULT MenuRendererWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
private:
	Menu* _activeMenu;
	std::mutex _activeMenuMutex;
	std::unique_ptr<std::remove_pointer<HWND>::type, Win32WindowDeleter> _hWnd;
	std::mutex _hWndMutex;
	std::thread _thread;
	static void PumpMessages(MenuRenderer* renderer) noexcept;
public:
	MenuRenderer();
	~MenuRenderer();
	void Destroy(Menu* menu);
	void Hide(const Menu* menu);
	void Show(Menu* menu, const int x, const int y) const;
};

struct MenuSeparator final : public MenuNode
{
	void Add(MenuNode* node) override;
	void AddTo(HMENU hMenu) override;
};

struct Win32Exception final : public std::runtime_error
{
	Win32Exception();
	Win32Exception(DWORD lastError);
};

extern MenuRenderer menuRenderer;