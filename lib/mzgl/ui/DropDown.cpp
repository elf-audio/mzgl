//
//  DropDown.cpp
//  TapeSampler
//
//  Created by Marek Bereza on 22/11/2017.
//
//

#include "DropDown.h"


void DropDown::draw() {
	
	if(options.size()==0) {
		g.setColor(1);
		g.drawRect(x, y, width, height);
		return;
	}
	if(collapsed) {
		g.setColor(1);
		g.drawRect(x, y, width, height);
		g.setColor(0);
		g.drawText(options[selectedIndex], x + 5, y + height - 5);
	} else {
		g.setColor(1);
		g.drawRect(x, y, width, height);
		g.setColor(0);
		
		// divider line
		g.drawLine(x, y + originalHeight, x + width, y + originalHeight);
		
		g.drawText(options[selectedIndex], x + 5, y + originalHeight - 5);
		for(int i = 0; i < options.size(); ++i) {
			if(i==hoveredIndex) {// highlight
				g.setColor(200/255.f);
				g.drawRect(x, y + originalHeight * (i + 1) + 1, width, originalHeight - 2);
			}
			g.setColor(0);
			g.drawText(options[i], x + 5, y + originalHeight * (2 + i) - 5);
		}
	}
}


void DropDown::toggle() {
	collapsed = !collapsed;
	if(!collapsed) {
		originalHeight = height;
		height = originalHeight * (options.size() + 1);
	} else {
		height = originalHeight;
	}
}

bool DropDown::touchDown(float x, float y, int id) {
	if(options.size()==0) return false;
	touchStart = glm::vec2(x, y);
	if(collapsed) {
		toggle();
		expandingClick = true;
	}
	touchHasMoved = false;
	return true;
}
void DropDown::touchMoved(float x, float y, int id) {
	if(options.size()==0) return;
	glm::vec2 t = glm::vec2(x, y);
	if(distance(t, touchStart)>3) {
		touchHasMoved = true;
	}
	
	
	if(!collapsed) {
		hoveredIndex = floor(((y - this->y) / originalHeight) - 1);
		if(!inside(x, y)) {
			hoveredIndex = -1;
		}
	}
}

void DropDown::touchOver(float x, float y) {
	if(options.size()==0) return;
	if(!collapsed) {
		hoveredIndex = floor(((y - this->y) / originalHeight) - 1);
		if(!inside(x, y)) {
			hoveredIndex = -1;
		}
	}
}

void DropDown::touchUp(float x, float y, int id) {
	if(options.size()==0) return;
	if(!collapsed && ((!expandingClick && !touchHasMoved) || (expandingClick && touchHasMoved))) {
		if(x>=this->x && x < this->x + this->width) {
			int ix = floor(((y - this->y) / originalHeight) - 1);
			if(ix<0 || ix >= options.size()) {
				// range check
			} else {
				if(selectedIndex!=ix) {
					selectedIndex = ix;
					if(onChange) onChange(selectedIndex);
						}
				
			}
			toggle();
		} else if(!inside(x, y)) {
			toggle(); // disable if touch up outside
		}
	}
	hoveredIndex = -1;
	expandingClick = false;
	
}
