#include "OldScrollingList.h"
#include "log.h"

using namespace std;


/////////////////////////////////////////////////////////////////////////

void OldScrollingList::doLayout() {
	updateItems();
	vbo = nullptr;
}


OldScrollingList::OldScrollingList(Graphics &g, float itemHeight) : Scroller(g) {
	this->itemHeight = itemHeight;// * g.pixelScale;
	updateItems();
}

shared_ptr<ScrollingListItem> OldScrollingList::getSelectedItem(){
	if(selectedIndex!=-1){
		return items[selectedIndex];
	}
	return nullptr;
}

void OldScrollingList::setItems(const vector<shared_ptr<ScrollingListItem>> &items) {

	this->items = items;
	updateItems();
	unselect();
}

void OldScrollingList::updateItems() {
	
	Scroller::clear();
	
	for(auto &item : items) {
		auto *a = getNewListItem(item);
		a->width = this->width;
		a->height = itemHeight;
		if(content->getNumChildren()>0) {
			a->y = content->getLastChild()->bottom();
		}
		addContent(a);
	}
}




void OldScrollingList::_draw() {
	maskOn();
	
	Drawer d;
	
	// draw the bg in the coord space of content
	// because of the translate later
	d.setColor(0);
	Rectf r = *this;
	r.x = -content->x;// content->x;
	r.y = -content->y;//content->y;
	d.drawRect(r);
	
	
//	draw();
	// now draw the scroller stuff
	
	
	int from = 0;
	int to = content->getNumChildren();
	for(int i = 0; i < content->getNumChildren(); i++) {
		auto *item = (ScrollingListItemView*)content->getChild(i);
		
		
		// don't draw offscreen
		if(item->y + item->height + content->y<0) {
			from = i+1;
			continue;
		}
		if(item->y + content->y > height) {
			to = i;
			break;
		}

		// check coords here and decide whether to draw.
		item->draw(d);
	}
	g.setColor(1);
	
	{
		if(vbo==nullptr) vbo = Vbo::create();
		ScopedTranslate scp(g, x + content->x, y + content->y);
		d.commit(vbo);
		vbo->draw(g);
	
		for(int i = from; i < to; i++) {
			auto *item = (ScrollingListItemView*)content->getChild(i);
			item->draw();
		}
	}
	if(drawingScrollbar) {
		drawScrollbar();
	}
	maskOff();
	
}


bool OldScrollingList::touchDown(float x, float y, int id) {
	Scroller::touchDown(x, y, id);
	selecting = true;
	startTouch = glm::vec2(x, y);
	selectedIndex = -1;
	auto testTouch = startTouch - glm::vec2(this->x + content->x, this->y + content->y);//content->x, content->y);
	
	for(int i = 0; i < content->getNumChildren(); i++) {
		auto *t = (ScrollingListItemView*)content->getChild(i);
		if(t->inside(testTouch)) {
			t->selected = true;
			selectedIndex = i;
		} else {
			t->selected = false;
		}
	}
	
	return true;
}


void OldScrollingList::touchMoved(float x, float y, int id) {
	Scroller::touchMoved(x, y, id);
	glm::vec2 t(x, y);
	if(distance(startTouch, t)>10) {
		selecting = false;
		if(selectedIndex!=-1) {
			auto *t = (ScrollingListItemView*)content->getChild(selectedIndex);
			t->selected = false;
			selectedIndex = -1;
		}
	}
}

void OldScrollingList::touchUp(float x, float y, int id) {
	Scroller::touchUp(x, y, id);
	if(selecting) {
		if(itemSelected) {
			itemSelected(selectedIndex);
		}
	}
}

void OldScrollingList::unselect() {
	selecting = false;
	selectedIndex = -1;
}



void OldScrollingList::setItemHeight(float itemHeight) {
	this->itemHeight = itemHeight;
	updateItems();
}
