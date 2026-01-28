//
//  Select.h
//
//  A dropdown select widget similar to HTML <select>
//

#pragma once
#include "Layer.h"
#include <functional>
#include <string>
#include <vector>

class Select : public Layer {
public:
	std::function<void(int)> onChange;

	Select(Graphics &g, const std::string &label, const std::vector<std::string> &options);
	Select(Graphics &g, const std::vector<std::string> &options)
		: Select(g, "", options) {}

	void setOptions(const std::vector<std::string> &opts);
	void setSelectedIndex(int index);
	int getSelectedIndex() const { return selectedIndex; }
	std::string getSelectedOption() const;

	void draw() override;
	bool touchDown(float x, float y, int id) override;
	void touchMoved(float x, float y, int id) override;
	void touchOver(float x, float y) override;
	void touchUp(float x, float y, int id) override;

private:
	void expand();
	void collapse();
	void updateHoveredIndex(float x, float y);

	std::string label;
	std::vector<std::string> options;
	int selectedIndex	 = 0;
	bool expanded		 = false;
	float collapsedHeight = 0;
	int hoveredIndex	 = -1;
	glm::vec2 touchStart;
	bool touchHasMoved	  = false;
	bool wasExpandedOnTouch = false;
};
