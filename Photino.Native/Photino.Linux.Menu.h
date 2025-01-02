#pragma once
#include <memory>
#include <string>
#include <gtk/gtk.h>
#include "Photino.h"

class Menu;
class MenuChild;
class MenuItem;
class MenuItemOptions;
class MenuNode;
class MenuParent;
class MenuRenderer;
class MenuSeparator;

typedef void (*OnClickedCallback)(MenuItem* menuItem);

class MenuNode
{
public:
    std::vector<MenuNode*> children;
    MenuNode* parent;
    MenuNode();
    virtual ~MenuNode() = default;
    virtual void Add(MenuNode* node);
    virtual void AddTo(GtkMenu* menu) = 0;
    virtual void NotifyOnClicked(MenuItem* item);
};

class Menu final : public MenuNode
{
protected:
    std::unique_ptr<GtkMenu, void(*)(GtkMenu*)> _menu;
    std::vector<OnClickedCallback> _onClicked;
public:
    Menu();
    void Add(MenuNode* node) override;
    void AddOnClicked(const OnClickedCallback onClicked);
    void AddTo(GtkMenu* menu) override;
    void Hide() const;
    void NotifyOnClicked(MenuItem* item) override;
    void RemoveOnClicked(const OnClickedCallback onClicked);
    void Show(const Photino* window, const int x, const int y);
};

class MenuItemOptions final
{
public:
	char* Label;
};

class MenuItem final : public MenuNode
{
private:
    std::unique_ptr<GtkMenu, void(*)(GtkMenu*)> _menu;
    std::unique_ptr<GtkMenuItem, void(*)(GtkMenuItem*)> _menuItem;
    std::string _label;
    static void OnActivated(GtkMenuItem* gtkItem, MenuItem* photinoItem);
public:
    MenuItem(MenuItemOptions options);
    void Add(MenuNode* child) override;
    void AddTo(GtkMenu* menu) override;
};

class MenuRenderer final
{
private:
public:
    void Destroy(Menu* menu);
};

class MenuSeparator final : public MenuNode
{
private:
    std::unique_ptr<GtkSeparatorMenuItem, void(*)(GtkSeparatorMenuItem*)> _separator;
public:
    MenuSeparator();
    void Add(MenuNode* child) override;
    void AddTo(GtkMenu* menu) override;
};

extern MenuRenderer menuRenderer;