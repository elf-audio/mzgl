//
//  MenuBar.hpp
//  Koala Sampler
//
//  Created by Marek Bereza on 16/09/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#pragma once
#include <memory>
#include <string>
#include <functional>
#include <map>

class MacMenu {
public:
	enum class KeyModifier { None, Shift, Alt };
	std::string name;
	MacMenu(std::string name)
		: name(name) {}
	void addItem(std::string title, std::string shortcut, std::function<void()> action);
	void addItem(std::string title, std::string shortcut, KeyModifier modifier, std::function<void()> action);
	void addSeparator();
	void clear();
};

class MacMenuBar {
public:
	static MacMenuBar &instance();
	std::shared_ptr<MacMenu> getMenu(std::string name);
	std::vector<std::shared_ptr<MacMenu>> getMenus();

private:
	MacMenuBar();
	std::map<std::string, std::shared_ptr<MacMenu>> menus;
};
