#include "ScrollingList.h"
#include "log.h"

using namespace std;

void ScrollingList::doLayout() {
	updateItems();
	
	vbo = nullptr;
}


ScrollingList::ScrollingList(Graphics &g, float itemHeight) : Scroller(g) {
	this->itemHeight = itemHeight;// * g.pixelScale;
	updateItems();
}

shared_ptr<ScrollingListItem> ScrollingList::getSelectedItem() {
	if(selectedIndex!=-1){
		return items[selectedIndex];
	}
	return nullptr;
}

void ScrollingList::setItems(const vector<shared_ptr<ScrollingListItem>> &items) {

	this->items = items;
	updateItems();
	unselect();
}

void ScrollingList::touchHeld() {
	// work out which view the touch is on and send the touch.
	
	auto testTouch = startTouch - glm::vec2(this->x + content->x, this->y + content->y);//content->x, content->y);
	
	for(int i = 0; i < content->getNumChildren(); i++) {
		auto *t = (ScrollingListItemView*)content->getChild(i);
		if(t->inside(testTouch)) {
			t->touchHeld(this, testTouch.x, testTouch.y, touchingId);
			return;
		}
	}
}
void ScrollingList::update() {
	Scroller::update();
	
	
	if(touchingId!=-1 && touchRect.getMaxDimension()<10 && !touchHeldCalled && g.currFrameTime - touchDownTime>0.5) {
		touchHeld();
		touchHeldCalled = true;
	}
	
	
	if(collapsingCells.size()>0) {
		
		// collapse the collapsing cells
		for(int ci = 0; ci < collapsingCells.size(); ci++) {
			
			auto *c = collapsingCells[ci];
			
			c->height *= 0.9;
			if(c->height<0.5) {
				
				// delete this collapsing cell
				
				// reset any selections
				selectedIndex = -1;
				
				// now remove it from items
				for(auto i = items.begin(); i != items.end(); i++) {
					if(*i == c->item) {
						
						
						Log::d() << "Found item to delete";
						
						// do the callback if necessary
						if(itemDeleted) {
							itemDeleted(*i);
						} else {
							Log::e() << "Item deleted but there's no callback";
						}
						// we're gonna erase this item
						items.erase(i);
						break;
					}
				}
				content->removeChild(c);
				collapsingCells.erase(collapsingCells.begin() + ci);
				ci--;
				delete c;
			}
		}
		
		// readjust positions
		for(int i = 1; i < content->getNumChildren(); i++) {
			content->getChild(i)->y = content->getChild(i-1)->bottom();
		}
		
		// I think you need this to make sure
		// the content is the right height after reomval
		if(!contentHeightExplicitlySet) {
			if(content->getNumChildren()>0) {
				content->height = content->getLastChild()->bottom();
			}
		}
	}
}

void ScrollingList::collapseAndDeleteCell(ScrollingListItemView *collapsingCell) {
	collapsingCells.push_back(collapsingCell);
}

void ScrollingList::updateItems() {
	
	Scroller::clear();
	
	
	for(auto &item : items) {
		auto *a = getNewListItem(item);
		a->width = this->width;
		if(a->hasCustomHeight()) {
			a->height = a->getCustomHeight();
		} else {
			a->height = itemHeight;
		}
		
		a->layoutSelfAndChildren();

		a->selectedSelf = [this, a]() {
			
			for(int i = 0; i < content->getNumChildren(); i++) {
				auto *t = (ScrollingListItemView*)content->getChild(i);
				if(t==a) {
					t->selected = true;
					selectedIndex = i;
				} else {
					t->selected = false;
				}
			}
			if(itemSelected) {
				itemSelected(selectedIndex);
			}
		};
		
		a->deleteSelf = [this, a]() {
			collapseAndDeleteCell(a);
		};
		
		if(content->getNumChildren()>0) {
			a->y = content->getLastChild()->bottom();
		}
		addContent(a);
		
	}
}




void ScrollingList::_draw() {
	maskOn();
	
	Drawer d;
	
	// draw the bg in the coord space of content
	// because of the translate later
	d.setColor(bgColor);
	Rectf r = *this;
	//r.x = -content->x;// content->x;
	r.y = -content->y;//content->y;
	d.drawRect(r);
	
	
//	draw();
	// now draw the scroller stuff
	
	
	int from = 0;
	int to = content->getNumChildren();
	for(int i = 0; i < content->getNumChildren(); i++) {
		auto *item = content->getChild(i);
		
		// don't draw offscreen
		if(item->y + item->height + content->y<0) {
			from = i+1;
			continue;
		}
		if(item->y + content->y > height) {
			to = i;
			break;
		}
	}
	
	
	for(int i = from; i < to; i++) {
		auto *item = (ScrollingListItemView*)content->getChild(i);
		item->draw(d);
	}
	
	g.setColor(1);
	
	{
		if(vbo==nullptr) vbo = Vbo::create();
		ScopedTranslate scp(g, x + content->x, y + content->y);
		d.commit(vbo);
		vbo->draw(g);
	
		for(int i = from; i < to; i++) {
			content->getChild(i)->_draw();
		}
	}
	
	
	if(drawingScrollbar) {
		drawScrollbar();
	}
	maskOff();
}



void ScrollingList::setItemHeight(float itemHeight) {
	this->itemHeight = itemHeight;
	updateItems();
}



///////////////////////////////////////////////////////
/// INTERACTIVITY
///

bool ScrollingList::touchDown(float x, float y, int id) {
	if(!canSelect) return true;
	// its a new touch, we only want one touch here
	if(touchingId!=-1) return true;
	touchingId = id;
	touchHeldCalled = false;
	Scroller::touchDown(x, y, id);
	selecting = true;
	touchDownTime = g.currFrameTime;
	startTouch = glm::vec2(x, y);
	touchRect.setFromCentre(startTouch, 0, 0);
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


void ScrollingList::touchMoved(float x, float y, int id) {
	if(!canSelect) return;
	if(id!=touchingId) return;
	Scroller::touchMoved(x, y, id);
	
	touchRect.growToInclude({x, y});
	if(touchRect.getMaxDimension()>10) {
		selecting = false;
		if(selectedIndex!=-1) {
			auto *t = (ScrollingListItemView*)content->getChild(selectedIndex);
			t->selected = false;
			selectedIndex = -1;
			if(itemSelected) itemSelected(-1);
		}
	}
}

void ScrollingList::cancelTouches() {
	touchingId = -1;
}

void ScrollingList::touchUp(float x, float y, int id) {
	if(touchingId!=id) return;
	touchingId = -1;
	if(!canSelect) return;
	Scroller::touchUp(x, y, id);
	if(selecting) {
		if(itemSelected) {
			itemSelected(selectedIndex);
		}
	}
}

void ScrollingList::unselect() {
	bool mustCallback = selectedIndex!=-1;
	if(selectedIndex!=-1) {
		auto *t = (ScrollingListItemView*)content->getChild(selectedIndex);
		t->selected = false;
	}
	selecting = false;
	selectedIndex = -1;
	if(mustCallback) if(itemSelected) itemSelected(-1);
}





