#include <algorithm>
#include <memory>
#include <stdexcept>
#include "Photino.Linux.Menu.h"
#include "gdk/gdk.h"
#include "gtk/gtk.h"

// -------------------------------------------------------------------------------------------------
// Variables
// -------------------------------------------------------------------------------------------------

MenuRenderer menuRenderer{};

// -------------------------------------------------------------------------------------------------
// Utilities
// -------------------------------------------------------------------------------------------------

GtkMenu* CreateMenu()
{
    return GTK_MENU(gtk_menu_new());
}

GtkMenuItem* CreateMenuItem(const MenuItemOptions& options)
{
    auto result = GTK_MENU_ITEM(gtk_menu_item_new());
    gtk_menu_item_set_label(result, options.Label);
    gtk_widget_show_all(GTK_WIDGET(result));
    return result;
}

GtkSeparatorMenuItem* CreateSeparator()
{
    return reinterpret_cast<GtkSeparatorMenuItem*>(gtk_separator_menu_item_new());
}

void DeleteMenu(GtkMenu* menu)
{
    if (menu != nullptr)
    {
        gtk_widget_destroy(GTK_WIDGET(menu));
    }
}

void DeleteMenuItem(GtkMenuItem* menuItem)
{
    if (menuItem != nullptr)
    {
        gtk_widget_destroy(GTK_WIDGET(menuItem));
    }
}

void DeleteSeparator(GtkSeparatorMenuItem* separator)
{
    if (separator != nullptr)
    {
        gtk_widget_destroy(GTK_WIDGET(separator));
    }
}

// -------------------------------------------------------------------------------------------------
// Menu
// -------------------------------------------------------------------------------------------------

Menu::Menu()
    : MenuNode()
    , _menu(CreateMenu(), &DeleteMenu)
{
}

void Menu::Add(MenuNode* child)
{
    MenuNode::Add(child);
    child->AddTo(_menu.get());
}

void Menu::AddOnClicked(const OnClickedCallback onClicked)
{
    _onClicked.push_back(onClicked);
}

void Menu::AddTo(GtkMenu* menu)
{
    throw std::logic_error("Menu is a top-level container.");
}

void Menu::Hide() const
{
    gtk_menu_popdown(_menu.get());
}

void Menu::NotifyOnClicked(MenuItem* item)
{
    auto copied = _onClicked;

    for (auto& onClicked : copied)
    {
        onClicked(item);
    }
}

void Menu::RemoveOnClicked(const OnClickedCallback onClicked)
{
    auto index = std::find(_onClicked.begin(), _onClicked.end(), onClicked);

    if (index != _onClicked.end())
    {
        _onClicked.erase(index);
    }
}

void Menu::Show(const Photino* window, const int x, const int y)
{
    // NOTE: We create a synthetic event here since, although this function is likely called as a
    // response to a user action, that action is unavailable to us.

    int screenX;
    int screenY;

    gtk_widget_translate_coordinates(
        window->_window,
        gtk_widget_get_toplevel(window->_window),
        x,
        y,
        &screenX,
        &screenY);

    GdkEventButton buttonEvent =
    {
        GDK_BUTTON_PRESS,
        gtk_widget_get_window(window->_window),
        true,
        static_cast<uint>(g_get_monotonic_time() / 1000),
        static_cast<double>(x),
        static_cast<double>(y),
        nullptr,
        0,
        3,
        nullptr,
        static_cast<double>(screenX),
        static_cast<double>(screenY)
    };

    GdkEvent event;
    event.button = buttonEvent;
    event.type = GDK_BUTTON_PRESS;

    GdkRectangle rect
    {
        0,
        0,
        1,
        1
    };

    gtk_widget_translate_coordinates(
        window->_webview,
        window->_window,
        x,
        y,
        &rect.x,
        &rect.y);

    gtk_menu_popup_at_rect(
        _menu.get(),
        buttonEvent.window,
        &rect,
        GDK_GRAVITY_SOUTH_EAST,
        GDK_GRAVITY_NORTH_WEST,
        &event);
}

// -------------------------------------------------------------------------------------------------
// MenuNode
// -------------------------------------------------------------------------------------------------

MenuNode::MenuNode()
    : parent(nullptr)
{
}

void MenuNode::Add(MenuNode* child)
{
    child->parent = this;
    children.push_back(child);
}

void MenuNode::NotifyOnClicked(MenuItem* item)
{
}

// -------------------------------------------------------------------------------------------------
// MenuItem
// -------------------------------------------------------------------------------------------------

MenuItem::MenuItem(MenuItemOptions options)
    : MenuNode()
    , _menu(nullptr, &DeleteMenu)
    , _menuItem(CreateMenuItem(options), &DeleteMenuItem)
    , _label(options.Label)
{
    g_signal_connect(
        _menuItem.get(),
        "activate",
        G_CALLBACK(&MenuItem::OnActivated),
        this);
}

void MenuItem::Add(MenuNode* child)
{
    MenuNode::Add(child);

    if (_menu == nullptr)
    {
        g_signal_handlers_disconnect_by_func(
            _menuItem.get(),
            reinterpret_cast<void*>(&MenuItem::OnActivated),
            this);

        _menu = std::unique_ptr<GtkMenu, void(*)(GtkMenu*)>(CreateMenu(), &DeleteMenu);
        gtk_menu_item_set_submenu(_menuItem.get(), GTK_WIDGET(_menu.get()));
    }

    child->AddTo(_menu.get());
}

void MenuItem::AddTo(GtkMenu* menu)
{
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(_menuItem.get()));
}

void MenuItem::OnActivated(GtkMenuItem* gtkItem, MenuItem* photinoItem)
{
    MenuNode* parent = photinoItem;

    while (parent->parent != nullptr)
    {
        parent = parent->parent;
    }

    parent->NotifyOnClicked(photinoItem);
}

// -------------------------------------------------------------------------------------------------
// MenuRenderer
// -------------------------------------------------------------------------------------------------

void MenuRenderer::Destroy(Menu* menu)
{
    delete menu;
}

// -------------------------------------------------------------------------------------------------
// MenuSeparator
// -------------------------------------------------------------------------------------------------

MenuSeparator::MenuSeparator()
    : MenuNode()
    , _separator(CreateSeparator(), &DeleteSeparator)
{
}

void MenuSeparator::Add(MenuNode* child)
{
    throw std::logic_error("Cannot add an item to a separator.");
}

void MenuSeparator::AddTo(GtkMenu* menu)
{
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(_separator.get()));
}