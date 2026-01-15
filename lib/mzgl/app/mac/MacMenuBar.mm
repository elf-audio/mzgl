//
//  MenuBar.cpp
//  Koala Sampler
//
//  Created by Marek Bereza on 16/09/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//
#include <AppKit/AppKit.h>
#include "MacMenuBar.h"

std::map<std::string, NSMenu *> submenus;

@interface LambdaMenuItem : NSMenuItem {
	std::function<void()> actionLambda;
}

- (id)initWithTitle:(NSString *)title keyEquivalent:(NSString *)keyEquivalent actionLambda:(std::function<void()>)func;
@end

@interface LambdaMenuItem ()
- (void)menuAction:(NSMenuItem *)item;
@end

@implementation LambdaMenuItem

- (id)initWithTitle:(NSString *)title keyEquivalent:(NSString *)keyEquivalent actionLambda:(std::function<void()>)func {
	self = [super initWithTitle:title action:@selector(menuAction:) keyEquivalent:keyEquivalent];
	if (self) {
		actionLambda = func;
		[self setTarget:self];
	}
	return self;
}

- (void)menuAction:(NSMenuItem *)item {
	if ((item == self) && (actionLambda)) {
		actionLambda();
	}
}
@end

MacMenuBar::MacMenuBar() {
	NSApp.mainMenu = [[NSMenu alloc] initWithTitle:@"MainMenu"];
}

MacMenuBar &MacMenuBar::instance() {
	static MacMenuBar inst;
	return inst;
}

std::shared_ptr<MacMenu> MacMenuBar::getMenu(std::string name) {
	if (menus.find(name) == menus.end()) {
		auto menu = std::make_shared<MacMenu>(name);

		menus[name] = menu;
		NSString *s = [NSString stringWithUTF8String:name.c_str()];

		NSMenuItem *title1 = [NSApp.mainMenu addItemWithTitle:s action:nil keyEquivalent:@""];
		NSMenu *menu1	   = [[NSMenu alloc] initWithTitle:s];
		// Disable automatic menu item validation so we can control enabled state manually
		menu1.autoenablesItems = NO;
		[NSApp.mainMenu setSubmenu:menu1 forItem:title1];

		submenus[name] = menu1;
	}
	return menus[name];
}

////////////////////////////////////////////////////////////

void MacMenu::clear() {
	[submenus[name] removeAllItems];
}

std::shared_ptr<MacMenuItem> MacMenu::addItem(std::string title,
											   std::string shortcut,
											   std::function<void()> action) {
	return addItem(title, shortcut, MacMenu::KeyModifier::None, action);
}

std::shared_ptr<MacMenuItem> MacMenu::addItem(std::string title,
											   std::string shortcut,
											   MacMenu::KeyModifier modifier,
											   std::function<void()> action) {
	id titleNS			 = [NSString stringWithUTF8String:title.c_str()];
	id scNS				 = [NSString stringWithUTF8String:shortcut.c_str()];
	LambdaMenuItem *item = [[LambdaMenuItem alloc] initWithTitle:titleNS keyEquivalent:scNS actionLambda:action];
	if (modifier == MacMenu::KeyModifier::Shift) {
		item.keyEquivalentModifierMask = NSEventModifierFlagShift;
	} else if (modifier == MacMenu::KeyModifier::Alt) {
		item.keyEquivalentModifierMask = NSEventModifierFlagOption;
	}
	[submenus[name] addItem:item];

	auto menuItem = std::make_shared<MacMenuItem>();
	menuItem->setNativeMenuItem((void *)CFBridgingRetain(item));
	return menuItem;
}

void MacMenu::addSeparator() {
	[submenus[name] addItem:[NSMenuItem separatorItem]];
}

std::vector<std::shared_ptr<MacMenu>> MacMenuBar::getMenus() {
	std::vector<std::shared_ptr<MacMenu>> v;
	for (auto &m: menus) {
		v.push_back(m.second);
	}
	return v;
}

////////////////////////////////////////////////////////////
// MacMenuItem implementation

void MacMenuItem::setEnabled(bool enabled) {
	if (nativeMenuItem) {
		NSMenuItem *item = (__bridge NSMenuItem *)nativeMenuItem;
		dispatch_async(dispatch_get_main_queue(), ^{
			item.enabled = enabled ? YES : NO;
		});
	}
}

bool MacMenuItem::isEnabled() const {
	if (nativeMenuItem) {
		NSMenuItem *item = (__bridge NSMenuItem *)nativeMenuItem;
		return item.enabled == YES;
	}
	return false;
}

void *MacMenuItem::getNativeMenuItem() {
	return nativeMenuItem;
}

void MacMenuItem::setNativeMenuItem(void *item) {
	nativeMenuItem = item;
}
