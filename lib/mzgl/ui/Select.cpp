//
//  Select.cpp
//

#include "Select.h"
#include "Font.h"

Select::Select(Graphics &g, const std::string &label, const std::vector<std::string> &options)
	: Layer(g, "select")
	, label(label)
	, options(options) {
	interactive = true;
}

void Select::setOptions(const std::vector<std::string> &opts) {
	options = opts;
	if (selectedIndex >= static_cast<int>(options.size())) {
		selectedIndex = options.empty() ? 0 : static_cast<int>(options.size()) - 1;
	}
}

void Select::setSelectedIndex(int index) {
	if (index >= 0 && index < static_cast<int>(options.size())) {
		selectedIndex = index;
	}
}

std::string Select::getSelectedOption() const {
	if (selectedIndex >= 0 && selectedIndex < static_cast<int>(options.size())) {
		return options[selectedIndex];
	}
	return "";
}

void Select::expand() {
	if (expanded || options.empty()) return;
	expanded		= true;
	collapsedHeight = height;
	height			= collapsedHeight * (static_cast<float>(options.size()) + 1);
	sendToFront();
}

void Select::collapse() {
	if (!expanded) return;
	expanded = false;
	height	 = collapsedHeight;
}

void Select::updateHoveredIndex(float x, float y) {
	if (!expanded) {
		hoveredIndex = -1;
		return;
	}

	if (!inside(x, y)) {
		hoveredIndex = -1;
		return;
	}

	int idx = static_cast<int>(std::floor((y - this->y) / collapsedHeight)) - 1;
	if (idx >= 0 && idx < static_cast<int>(options.size())) {
		hoveredIndex = idx;
	} else {
		hoveredIndex = -1;
	}
}

void Select::draw() {
	if (options.empty()) {
		g.setColor(0.3f);
		g.fill();
		g.drawRect(*this);
		g.setColor(0.6f);
		g.noFill();
		g.drawRect(*this);
		return;
	}

	// Background
	g.setColor(0.2f);
	g.fill();
	g.drawRect(*this);

	// Border
	g.setColor(0.5f);
	g.noFill();
	g.drawRect(*this);

	float rowH     = expanded ? collapsedHeight : height;
	float textY    = y + rowH * 0.7f;
	float textPad  = 8.f;
	float arrowPad = 28.f;
	int maxTextW   = static_cast<int>(width - textPad - arrowPad);
	Font &font     = g.getFont();

	if (!expanded) {
		// Draw label and selected value
		g.setColor(1.f);
		std::string displayText = label.empty() ? options[selectedIndex] : label + ": " + options[selectedIndex];
		g.drawText(font.ellipsize(displayText, maxTextW), x + textPad, textY);

		// Draw dropdown arrow
		float arrowX = x + width - 20;
		float arrowY = y + rowH * 0.5f;
		g.setColor(0.7f);
		g.fill();
		g.drawTriangle({arrowX, arrowY - 4}, {arrowX + 8, arrowY - 4}, {arrowX + 4, arrowY + 4});
	} else {
		// Draw selected header
		g.setColor(0.3f);
		g.fill();
		g.drawRect(x, y, width, collapsedHeight);

		g.setColor(1.f);
		std::string displayText = label.empty() ? options[selectedIndex] : label + ": " + options[selectedIndex];
		g.drawText(font.ellipsize(displayText, maxTextW), x + textPad, textY);

		// Divider
		g.setColor(0.4f);
		g.drawLine(x, y + collapsedHeight, x + width, y + collapsedHeight);

		// Draw options (full width available, no arrow)
		int optMaxW = static_cast<int>(width - textPad * 2);
		for (int i = 0; i < static_cast<int>(options.size()); i++) {
			float optY = y + collapsedHeight * (i + 1);

			// Highlight hovered option
			if (i == hoveredIndex) {
				g.setColor(0.35f);
				g.fill();
				g.drawRect(x + 1, optY + 1, width - 2, collapsedHeight - 2);
			}

			// Option text
			g.setColor(i == selectedIndex ? 0.5f : 1.f);
			g.drawText(font.ellipsize(options[i], optMaxW), x + textPad, optY + collapsedHeight * 0.7f);
		}
	}

	g.fill();
}

bool Select::touchDown(float x, float y, int id) {
	if (options.empty()) return false;

	touchStart		  = glm::vec2(x, y);
	touchHasMoved	  = false;
	wasExpandedOnTouch = expanded;

	if (!expanded) {
		expand();
	}

	return true;
}

void Select::touchMoved(float x, float y, int id) {
	if (options.empty()) return;

	if (glm::distance(glm::vec2(x, y), touchStart) > 3) {
		touchHasMoved = true;
	}

	updateHoveredIndex(x, y);
}

void Select::touchOver(float x, float y) {
	updateHoveredIndex(x, y);
}

void Select::touchUp(float x, float y, int id) {
	if (options.empty()) return;

	if (expanded) {
		// Only select if we moved (drag selection) or if it was already expanded
		if (touchHasMoved || wasExpandedOnTouch) {
			if (hoveredIndex >= 0 && hoveredIndex < static_cast<int>(options.size())) {
				if (selectedIndex != hoveredIndex) {
					selectedIndex = hoveredIndex;
					if (onChange) onChange(selectedIndex);
				}
			}
			collapse();
		} else if (!inside(x, y)) {
			// Clicked outside while expanded
			collapse();
		}
		// If clicked without moving and wasn't expanded before, leave it expanded
	}

	hoveredIndex = -1;
}
