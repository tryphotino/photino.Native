#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <MacTypes.h>
#include <memory>
#include <objc/NSObject.h>
#include <stdexcept>
#include "Photino.Mac.Menu.h"

// -------------------------------------------------------------------------------------------------
// Variables
// -------------------------------------------------------------------------------------------------

MenuRenderer menuRenderer{};

// -------------------------------------------------------------------------------------------------
// Utilities
// -------------------------------------------------------------------------------------------------

std::unique_ptr<MenuItemHandler, MenuItemHandlerDeleter> CreateMenuItemHandler(MenuItem* menuItem)
{
    return std::unique_ptr<MenuItemHandler, MenuItemHandlerDeleter>(
        [[MenuItemHandler alloc] initWithCallback:[=]()
        {
            menuItem->NotifyOnClicked(menuItem);
        }]);
}

std::unique_ptr<NSMenu, NSMenuDeleter> CreateNSMenu()
{
    return std::unique_ptr<NSMenu, NSMenuDeleter>([[NSMenu alloc] init]);
}

std::unique_ptr<NSMenuItem, NSMenuItemDeleter> CreateNSMenuItem(const MenuItemOptions& options)
{
    auto item = [[NSMenuItem alloc] init];
    item.title = [[NSString alloc] initWithCString:options.Label encoding:NSUTF8StringEncoding];
    return std::unique_ptr<NSMenuItem, NSMenuItemDeleter>(item);
}

std::unique_ptr<NSMenuItem, NSMenuItemDeleter> CreateSeparator()
{
    return std::unique_ptr<NSMenuItem, NSMenuItemDeleter>([NSMenuItem separatorItem]);
}

// -------------------------------------------------------------------------------------------------
// Deleters
// -------------------------------------------------------------------------------------------------

void NSMenuDeleter::operator()(NSMenu* menu)
{
    if (menu != nullptr)
    {
        [menu release];
    }
}

void NSMenuItemDeleter::operator()(NSMenuItem* menuItem)
{
    if (menuItem != nullptr)
    {
        [menuItem release];
    }
}

void MenuItemHandlerDeleter::operator()(MenuItemHandler* handler)
{
    if (handler != nullptr)
    {
        [handler release];
    }
}

// -------------------------------------------------------------------------------------------------
// Menu
// -------------------------------------------------------------------------------------------------

Menu::Menu()
    : MenuNode()
    , _menu(CreateNSMenu())
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

void Menu::AddTo(NSMenu* menu)
{
    throw std::logic_error("Menu is a top-level MenuNode.");
}

void Menu::Hide() const
{
    // TODO: Implement this.
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

void Menu::Show(const Photino* window, const int x, const int y) const
{
    NSPoint position
    {
        CGFloat(x),
        CGFloat(y)
    };

    [_menu.get() popUpMenuPositioningItem:nil atLocation:position inView:window->_webview];
}

// -------------------------------------------------------------------------------------------------
// MenuItem
// -------------------------------------------------------------------------------------------------

MenuItem::MenuItem(MenuItemOptions options)
    : MenuNode()
    , _menuItem(CreateNSMenuItem(options))
    , _menuItemHandler(CreateMenuItemHandler(this))
{
    SEL action = @selector(menuItemAction:);
    [_menuItem.get() setAction:action];
    [_menuItem.get() setTarget:_menuItemHandler.get()];
}

void MenuItem::Add(MenuNode* child)
{
    MenuNode::Add(child);

    if (_menu == nullptr)
    {
        _menu = CreateNSMenu();
        [_menuItem.get() setAction:nil];
        [_menuItem.get() setTarget:nil];
        [_menuItem.get() setSubmenu:_menu.get()];
    }

    child->AddTo(_menu.get());
}

void MenuItem::AddTo(NSMenu* menu)
{
    [menu addItem:_menuItem.get()];
}

void MenuItem::NotifyOnClicked(MenuItem* item)
{
    if (parent != nullptr)
    {
        parent->NotifyOnClicked(item);
    }
}

// -------------------------------------------------------------------------------------------------
// MenuItemHandler
// -------------------------------------------------------------------------------------------------

@implementation MenuItemHandler {
    std::function<void()> _callback;
}

- (instancetype)initWithCallback:(std::function<void()>)callback
{
    self = [super init];

    if (self != nil)
    {
        _callback = callback;
    }

    return self;
}

- (void)menuItemAction:(id)sender
{
    _callback();
}
@end

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
    children.push_back(node);
}

void MenuNode::NotifyOnClicked(MenuItem *item)
{
}

// -------------------------------------------------------------------------------------------------
// MenuSeparator
// -------------------------------------------------------------------------------------------------

MenuSeparator::MenuSeparator()
    : MenuNode()
    , _menuItem(CreateSeparator())
{
}

void MenuSeparator::Add(MenuNode* child)
{
    throw std::logic_error("Cannot add a MenuNode to a MenuSeparator.");
}

void MenuSeparator::AddTo(NSMenu* menu)
{
    [menu addItem:_menuItem.get()];
}

// -------------------------------------------------------------------------------------------------
// MenuRenderer
// -------------------------------------------------------------------------------------------------

void MenuRenderer::Destroy(Menu* menu)
{
    delete menu;
}

