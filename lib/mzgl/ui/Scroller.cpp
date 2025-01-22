
#include "Scroller.h"

Scroller::Scroller(Graphics &g)
	: Layer(g) {
	interactive = true;
	content		= new Layer(g, "content");
	addChild(content);
	clipToBounds = true;
}
void Scroller::contentUpdated() {
	if (!contentHeightExplicitlySet) {
		Layer *lastVisibleChild = nullptr;
		for (int i = 0; i < content->getNumChildren(); i++) {
			Layer *c = content->getChild(i);
			if (c->visible) {
				lastVisibleChild = c;
			}
		}
		if (lastVisibleChild != nullptr) {
			content->height = lastVisibleChild->bottom();
		}
	}
}
void Scroller::addContent(Layer *layer) {
	content->addChild(layer);
	if (!contentHeightExplicitlySet) {
		content->height = std::max(content->height, layer->bottom());
	}
}

void Scroller::clear() {
	content->clear();
	content->height = 0;
}

void Scroller::updateDeprecated() {
	if (!scrolling) {
		float lerpSpeed = 0.86;
		if (content->y > 0) {
			content->y *= lerpSpeed;
		} else if (content->height < height) { //} && content->y < 0) {
			content->y *= lerpSpeed;
			//} else if(content->height < height && content->y > 0) {

		} else if (content->bottom() < height) {
			content->y = content->y * lerpSpeed + (height - content->height) * (1.0 - lerpSpeed);
		}

		contentVelocity *= 0.9;
		content->y += contentVelocity.y;
		if (abs(contentVelocity.y) < 0.01) {
			scrollbarActivityAmt -= 0.1;
			if (scrollbarActivityAmt < 0) scrollbarActivityAmt = 0;
		}
	} else {
		scrollbarActivityAmt += 0.1;
		if (scrollbarActivityAmt > 1) scrollbarActivityAmt = 1;
	}
}

void Scroller::draw() {
	if (color.a == 0) return;
	g.setColor(color);
	g.drawRect(*this);
}

void Scroller::_draw() {
	if (!visible) return;
	Layer::_draw();
	if (drawingScrollbar) {
		drawScrollbar();
	}
}
void Scroller::drawScrollbar() {
	if (content->height > height) {
		g.fill();
		ScopedAlphaBlend scpA(g, true);
		if (scrollbarDims) {
			g.setColor(1, 1, 1, 0.1 + 0.15 * scrollbarActivityAmt);
		} else {
			g.setColor(1, 1, 1, 0.25);
		}

		float a = content->y / (height - content->height);
		//mapf(content->y, 0, height - content->height, 0, 1);
		float squash = 1;
		if (a > 1) {
			squash = 2 - a;
			a	   = 1;
		} else if (a < 0) {
			squash = 1 + a;
			a	   = 0;
		}

		if (squash < 1) {
			squash = 1 + tanh(squash - 1) * 0.5f;
		}

		float w		  = 10;
		float padding = w;

		float h	 = (height - padding * 2) * (height / content->height) * squash;
		float yy = y + padding + (height - (padding * 2) - h) * a;
		g.drawRoundedRect(Rectf(x + width - padding * 2, yy, w, h), w / 2);
	}
}

void Scroller::touchOver(float x, float y) {
}

void Scroller::touchUp(float x, float y, int id) {
	scrolling = false;
}

void Scroller::touchMoved(float x, float y, int id) {
	if (content->height <= height) return;
	if (scrolling) {
		glm::vec2 pos(x, y);
		glm::vec2 delta = pos - lastTouch;

		contentVelocity = delta;

		// make it hard to drag left and right
		if (content->y > 0) {
			contentVelocity *= (1 - content->y / height) * 0.5;
		} else if (content->bottom() < height) {
			contentVelocity *= (1 - (height - content->bottom()) / height) * 0.5;
		}
		content->y += contentVelocity.y;

		lastTouch = pos;
	}
}

bool Scroller::touchDown(float x, float y, int id) {
	if (inside(x, y)) {
		lastTouch = glm::vec2(x, y);
		scrolling = true;
		return true;
	} else {
		return false;
	}
}

void Scroller::setContentHeight(float contentHeight) {
	this->content->height	   = contentHeight;
	contentHeightExplicitlySet = true;
}
void Scroller::mouseScrolled(float x, float y, float scrollX, float scrollY) {
	if (content->height <= height) {
		return;
	}

	contentVelocity = vec2(scrollX * 8.f, scrollY * 8.f);
}
