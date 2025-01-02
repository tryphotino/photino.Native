#pragma once
#include <AppKit/AppKit.h>
#include <memory>
#include <string>
#include "Photino.h"

class Menu;
class MenuItem;
class MenuItemOptions;
class MenuNode;
class MenuRenderer;
class MenuSeparator;

typedef void (*OnClickedCallback)(MenuItem* menuItem);

@interface MenuItemHandler : NSObject
- (instancetype)initWithCallback:(std::function<void()>) callback;
- (void)menuItemAction:(id)sender;
@end

struct MenuItemHandlerDeleter
{
    void operator()(MenuItemHandler* handler);
};

struct NSMenuDeleter
{
    void operator()(NSMenu* menu);
};

struct NSMenuItemDeleter
{
    void operator()(NSMenuItem* menuItem);
};

class MenuNode
{
public:
    std::vector<MenuNode*> children;
    MenuNode* parent;
    MenuNode();
    virtual ~MenuNode() = default;
    virtual void Add(MenuNode* node);
    virtual void AddTo(NSMenu* menu) = 0;
    virtual void NotifyOnClicked(MenuItem* item);
};

class Menu final : public MenuNode
{
protected:
    std::unique_ptr<NSMenu, NSMenuDeleter> _menu;
    std::vector<OnClickedCallback> _onClicked;
public:
    Menu();
    void Add(MenuNode* child) override;
    void AddOnClicked(const OnClickedCallback onClicked);
    void AddTo(NSMenu* menu) override;
    void Hide() const;
    void NotifyOnClicked(MenuItem* item) override;
    void RemoveOnClicked(const OnClickedCallback onClicked);
    void Show(const Photino* window, const int x, const int y) const;
};

struct MenuItemOptions final
{
	char* Label;
};

class MenuItem final : public MenuNode
{
private:
    std::unique_ptr<NSMenu, NSMenuDeleter> _menu;
    std::unique_ptr<NSMenuItem, NSMenuItemDeleter> _menuItem;
    std::unique_ptr<MenuItemHandler, MenuItemHandlerDeleter> _menuItemHandler;
public:
    MenuItem(MenuItemOptions options);
    void Add(MenuNode* child) override;
    void AddTo(NSMenu* menu) override;
    void NotifyOnClicked(MenuItem* item) override;
};

class MenuSeparator final : public MenuNode
{
private:
    std::unique_ptr<NSMenuItem, NSMenuItemDeleter> _menuItem;
public:
    MenuSeparator();
    void Add(MenuNode* child) override;
    void AddTo(NSMenu* menu) override;
};

class MenuRenderer final
{
public:
    void Destroy(Menu* menu);
};

extern MenuRenderer menuRenderer;