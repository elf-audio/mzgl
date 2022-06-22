
#include "ScrollingList.h"
#include "Drawer.h"

void ScrollingListStringView::draw() {
	g.drawTextVerticallyCentred(item->name, glm::vec2(x+20 * g.pixelScale, y + height / 2));
}

void ScrollingListStringView::draw(Drawer &d) {

	if(hasBG) {
		if(selected) {
			d.setColor(selectedColor);
		} else {
			d.setColor(unselectedColor);
		}

		Rectf r = *this;
		r.y = (int) r.y+2;
		r.height = (int) r.height -1;
		d.drawRect(r);
	}
	
	if(hasDivider) {
		d.setColor(dividerColor);
		d.drawRect(x, (int)(y + height), width, 2);
	}
}

