//
//  MenuBar.cpp
//  Koala Sampler
//
//  Created by Marek Bereza on 16/09/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//
#include <AppKit/AppKit.h>
#include "MacMenuBar.h"

using namespace std;

map<string, NSMenu *> submenus;

@interface LambdaMenuItem : NSMenuItem {
	function<void()> actionLambda;
}

- (id)initWithTitle:(NSString *)title keyEquivalent:(NSString *)keyEquivalent actionLambda:(function<void()>)func;
@end

@interface LambdaMenuItem ()
- (void)menuAction:(NSMenuItem *)item;
@end

@implementation LambdaMenuItem

- (id)initWithTitle:(NSString *)title keyEquivalent:(NSString *)keyEquivalent actionLambda:(function<void()>)func {
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

shared_ptr<MacMenu> MacMenuBar::getMenu(string name) {
	if (menus.find(name) == menus.end()) {
		auto menu = make_shared<MacMenu>(name);

		menus[name] = menu;
		NSString *s = [NSString stringWithUTF8String:name.c_str()];

		NSMenuItem *title1 = [NSApp.mainMenu addItemWithTitle:s action:nil keyEquivalent:@""];
		NSMenu *menu1	   = [[NSMenu alloc] initWithTitle:s];
		[NSApp.mainMenu setSubmenu:menu1 forItem:title1];

		submenus[name] = menu1;
	}
	return menus[name];
}

////////////////////////////////////////////////////////////

void MacMenu::clear() {
	[submenus[name] removeAllItems];
}
void MacMenu::addItem(std::string title, std::string shortcut, std::function<void()> action) {
	addItem(title, shortcut, MacMenu::KeyModifier::None, action);
}

void MacMenu::addItem(std::string title,
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
