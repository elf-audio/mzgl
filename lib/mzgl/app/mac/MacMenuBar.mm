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


map<string,NSMenu*> submenus;

@interface LambdaMenuItem : NSMenuItem {
	function<void()> actionLambda;
}
	
- (id)initWithTitle:(NSString *)title keyEquivalent:(NSString *)keyEquivalent actionLambda: (function<void()>) func;
@end

@interface LambdaMenuItem ()
- (void)menuAction:(NSMenuItem *)item;
@end

@implementation LambdaMenuItem
	
- (id)initWithTitle:(NSString *)title keyEquivalent:(NSString *)keyEquivalent actionLambda:(function<void()>) func {
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
	NSApp.mainMenu = [[NSMenu alloc] initWithTitle: @"MainMenu"];
}

MacMenuBar &MacMenuBar::instance() {
	static MacMenuBar inst;
	return inst;
}

shared_ptr<MacMenu> MacMenuBar::getMenu(string name) {
	if(menus.find(name)==menus.end()) {
		auto menu = make_shared<MacMenu>(name);
		
		menus[name] = menu;
		NSString *s = [NSString stringWithUTF8String:name.c_str()];
		
		  
		NSMenuItem *title1 = [NSApp.mainMenu addItemWithTitle:s action:nil keyEquivalent:@""];
		NSMenu *menu1 = [[NSMenu alloc] initWithTitle:s];
		[NSApp.mainMenu setSubmenu:menu1 forItem:title1];
			
		submenus[name] = menu1;
	
	}
	return menus[name];
}

////////////////////////////////////////////////////////////


void MacMenu::addItem(std::string title, std::string shortcut, std::function<void()> action) {
#if 1
	id titleNS = [NSString stringWithUTF8String: title.c_str()];
	id scNS = [NSString stringWithUTF8String:shortcut.c_str()];
	id item = [[LambdaMenuItem alloc] initWithTitle: titleNS
											 keyEquivalent:scNS actionLambda:action];
	
	[submenus[name] addItem: item];
#endif
}

void MacMenu::addSeparator() {
#if 1
	[submenus[name] addItem: [NSMenuItem separatorItem]];
#endif
}

