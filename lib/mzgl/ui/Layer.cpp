//
//  Layer.cpp
//
//
//

#include "Layer.h"
#include "Graphics.h"
#include "util.h"
#include "stringUtil.h"
#include "log.h"
#include "mzAssert.h"

void setLayerSize(std::vector<Layer *> layers, float width, float height) {
	setLayerSize(layers, vec2(width, height));
}
void setLayerSize(std::vector<Layer *> layers, float sz) {
	setLayerSize(layers, vec2(sz, sz));
}
void setLayerSize(std::vector<Layer *> layers, vec2 sz) {
	for (auto *l: layers) {
		l->size(sz);
	}
}

Layer::Layer(Graphics &g, const std::string &name)
	: g(g)
	, name(name) {
}

Layer::Layer(Graphics &g, const std::vector<Layer *> &children)
	: Layer(g) {
	addChildren(children);
}
Layer::Layer(Graphics &g, const std::string &name, const std::vector<Layer *> &children)
	: Layer(g, name) {
	addChildren(children);
}

Layer::~Layer() {
	clear();
}

std::string Layer::toString() const {
	return "name: " + name + " (xy: " + to_string(x, 0) + "," + to_string(y, 0) + " " + to_string(width, 0)
		   + "  x " + to_string(height, 0) + ")";
}

void Layer::maskOn() {
	g.maskOn(getAbsoluteRect());
}

void Layer::maskOff() {
	g.maskOff();
}

void Layer::pushMask() {
	scopedMask.startMask(g, getAbsoluteRect());
}

void Layer::popMask() {
	scopedMask.stopMask();
}

// this draws regardless of mask
void Layer::__draw() {
	if (x != 0 || y != 0) {
		g.pushMatrix();
		draw();
		g.translate(x, y);
		for (auto *c: children)
			c->_draw();
		g.popMatrix();
	} else {
		draw();
		for (auto *c: children)
			c->_draw();
	}
}

void Layer::_draw() {
	if (!visible) return;

	if (clipToBounds) {
		pushMask();
		__draw();
		popMask();
	} else {
		__draw();
	}
}

bool Layer::isVisible() const {
	if (!visible) return false;
	if (parent != nullptr) {
		return parent->isVisible();
	}
	return visible;
}
void Layer::layoutSelfAndChildren() {
	doLayout();
	for (auto *c: children) {
		c->layoutSelfAndChildren();
	}
}

bool Layer::getRectRelativeTo(const Layer *l, Rectf &r) const {
	if (parent == nullptr) return false;
	if (parent == l) {
		r = *this;
		return true;
	}
	Layer *curr = parent;
	Rectf out	= *this;

	while (1) {
		if (curr == nullptr) {
			return false;
		}

		out.x += curr->x;
		out.y += curr->y;
		curr = curr->parent;

		if (curr == l) {
			r = out;
			return true;
		}
	}
	return false;
}

Layer *Layer::addChild(Layer *layer) {
	mzAssert(layer->getParent() == nullptr, "Layer already has a parent");

	layer->parent = this;

	mzAssert(layer != this, "Can't add a layer to itself");
	for (auto *c: children) {
		mzAssert(c != layer, "Can't add a layer to the same parent twice");
	}

	children.push_back(layer);
	return layer;
}

bool Layer::removeFromParent() {
	if (getParent() != nullptr) {
		bool res = getParent()->removeChild(this);
		parent	 = nullptr;
		return res;
	}
	return false;
}

bool Layer::removeChild(Layer *layer) {
	for (int i = 0; i < children.size(); i++) {
		if (children[i] == layer) {
			children[i]->parent = nullptr;
			children.erase(children.begin() + i);
			return true;
		}
	}
	return false;
}

void Layer::addChildren(std::vector<Layer *> layers) {
	for (auto *l: layers) {
		addChild(l);
	}
}

void Layer::_mouseScrolled(float x, float y, float scrollX, float scrollY) {
	if (!visible) return;

	float xx = x;
	float yy = y;
	transformMouse(xx, yy);
	for (auto it = children.rbegin(); it != children.rend(); it++) {
		(*it)->_mouseScrolled(xx, yy, scrollX, scrollY);
	}

	if (interactive && inside(x, y)) {
		mouseScrolled(x, y, scrollX, scrollY);
	}
}

void Layer::_mouseZoomed(float x, float y, float zoom) {
	if (!visible) return;

	float xx = x;
	float yy = y;
	transformMouse(xx, yy);
	for (auto it = children.rbegin(); it != children.rend(); it++) {
		(*it)->_mouseZoomed(xx, yy, zoom);
	}

	if (interactive && inside(x, y)) {
		mouseZoomed(x, y, zoom);
	}
}

void Layer::_touchOver(float x, float y) {
	if (!visible) return;

	float xx = x;
	float yy = y;
	transformMouse(xx, yy);

	for (auto it = children.rbegin(); it != children.rend(); it++) {
		(*it)->_touchOver(xx, yy);
	}

	if (interactive) {
		touchOver(x, y);
	}
}

void Layer::_touchUp(float x, float y, int id) {
	if (g.focusedLayers.find(id) == g.focusedLayers.end()) return;

	float xx = x;
	float yy = y;

	g.focusedLayers[id]->absoluteToLocalCoords(xx, yy);
	g.focusedLayers[id]->touchUp(xx, yy, id);
	g.focusedLayers.erase(id);
}

void Layer::_touchMoved(float x, float y, int id) {
	if (g.focusedLayers.find(id) == g.focusedLayers.end()) return;

	float xx = x;
	float yy = y;

	g.focusedLayers[id]->absoluteToLocalCoords(xx, yy);
	g.focusedLayers[id]->touchMoved(xx, yy, id);
}

void Layer::transformMouse(float &xx, float &yy) {
	xx -= this->x;
	yy -= this->y;
}

bool Layer::_touchDown(float x, float y, int id) {
	if (!visible) return false;
	float xx = x;
	float yy = y;
	transformMouse(xx, yy);

	// if we're clipping to bounds, reject any touches
	// that aren't inside the bounds
	if (clipToBounds) {
		if (!inside(x, y)) return false;
	}

	for (auto it = children.rbegin(); it != children.rend(); it++) {
		if ((*it)->_touchDown(xx, yy, id)) {
			return true;
		}
	}

	if (interactive && inside(x, y)) {
		g.focusedLayers[id] = this;

		return touchDown(x, y, id);
	}
	return false;
}

void Layer::_updateDeprecated() {
	for (auto *c: children) {
		c->_updateDeprecated();
	}
	updateDeprecated();
}

void Layer::sendToBack(Layer *child) {
	if (child == nullptr) {
		Layer *parent = this->getParent();
		if (parent != nullptr) {
			parent->sendToBack(this);
		}
	} else {
		if (children.size() == 0) return;
		if (children[0] == child) return;
		for (int i = 0; i < children.size(); i++) {
			if (children[i] == child) {
				children.erase(children.begin() + i);
				children.insert(children.begin(), child);
				return;
			}
		}
		Log::e() << "Couldn't find child in sendLayerToBack " << child->name;
	}
}

void Layer::sendToFront(Layer *child) {
	if (child == nullptr) {
		Layer *p = this->getParent();
		if (p != nullptr) {
			p->sendToFront(this);
		}
	} else {
		if (children.size() == 0) return;
		if (children.back() == child) return;
		for (int i = 0; i < children.size(); i++) {
			if (children[i] == child) {
				children.erase(children.begin() + i);
				children.push_back(child);
				return;
			}
		}
		Log::e() << "Couldn't find child in sendLayerToFront " << child->name;
	}
}

Layer *Layer::getParent() const {
	return parent;
}

Layer *Layer::getRoot() {
	if (parent == nullptr) {
		return this;
	} else {
		return parent->getRoot();
	}
}

int Layer::getNumChildren() const {
	return (int) children.size();
}

Layer *Layer::getChild(int index) {
	if (index < 0 || index >= children.size()) {
		Log::e() << "Couldn't find child of " << name << " at index " << index;
		return nullptr;
	}
	return children[index];
}

Layer *Layer::getFirstChild() {
	return children[0];
}
Layer *Layer::getLastChild() {
	return children.back();
}

Rectf Layer::getAbsoluteRect() const {
	return getAbsoluteRect(*this);
}

glm::vec2 Layer::getAbsolutePosition() const {
	return getAbsolutePosition(position());
}

void Layer::setAbsolutePosition(glm::vec2 p) {
	auto whereAmINow = getAbsolutePosition(tl());
	auto delta		 = p - whereAmINow;
	x += delta.x;
	y += delta.y;
}
glm::vec2 Layer::getAbsolutePosition(glm::vec2 pos) const {
	localToAbsoluteCoords(pos.x, pos.y);
	return pos;
}

glm::vec2 Layer::getLocalPosition(glm::vec2 pos) const {
	absoluteToLocalCoords(pos.x, pos.y);
	return pos;
}

Rectf Layer::getAbsoluteRect(const Rectf &rr) const {
	Rectf r;
	glm::vec2 tl = getAbsolutePosition(rr.tl());
	r.x			 = tl.x;
	r.y			 = tl.y;
	r.width		 = rr.width;
	r.height	 = rr.height;
	return r;
}

void Layer::absoluteToLocalCoords(float &xx, float &yy) const {
	const Layer *layer = this;
	while ((layer = layer->getParent()) != nullptr) {
		xx -= layer->x;
		yy -= layer->y;
	}
}
void Layer::localToAbsoluteCoords(float &xx, float &yy) const {
	const Layer *layer = this;
	while ((layer = layer->getParent()) != nullptr) {
		xx += layer->x;
		yy += layer->y;
	}
}

bool Layer::hasFocus() const {
	for (auto l: g.focusedLayers) {
		if (l.second == this) {
			return true;
		}
	}
	return false;
}

void Layer::removeFocus() {
	for (auto l = g.focusedLayers.begin(); l != g.focusedLayers.end(); l++) {
		if (l->second == this) {
			g.focusedLayers.erase(l);
			return;
		}
	}
}

/**
 * This used to take only the otherLayer as the parameter, but that meant
 * that if a layer had 2 fingers focused on it, there would be no way to
 * distinguish between the 2 fingers - so now you need to pass in a touchId
 */
void Layer::transferFocus(Layer *otherLayer, int touchId) {
	if (g.focusedLayers.find(touchId) != g.focusedLayers.end() && g.focusedLayers[touchId] == this) {
		g.focusedLayers[touchId] = otherLayer;
	} else {
		Log::e() << "Couldn't find the other layer to focus on";
	}
}

void Layer::transferFocus(Layer *fromLayer, Layer *toLayer) {
	for (auto &l: g.focusedLayers) {
		if (l.second == fromLayer) {
			l.second = toLayer;
			return;
		}
	}
}

void Layer::clear() {
	if (!g.focusedLayers.empty()) {
		for (auto *ch: children) {
			auto it = std::find_if(
				g.focusedLayers.begin(), g.focusedLayers.end(), [ch](auto &&focus) { return focus.second == ch; });
			if (it != g.focusedLayers.end()) {
				mzAssert(false, "Can't delete a layer whilst there is an interaction going on with it!");
				g.focusedLayers.erase(it);
			}
		}
	}

	for (auto *c: children) {
		delete c;
	}

	children.clear();
}

bool Layer::isInFront() const {
	if (parent) {
		return parent->children.back() == this;
	}
	return false;
}
void Layer::positionAbove(Layer *l, float padding) {
	x = l->x;
	setBottom(l->y - padding);
}

void Layer::positionAboveCentred(Layer *l, float padding) {
	setCentreX(l->centreX());
	setBottom(l->y - padding);
}
void Layer::positionUnder(Layer *l, float padding) {
	x = l->x;
	y = l->bottom() + padding;
}

void Layer::positionUnderCentred(Layer *l, float padding) {
	setCentreX(l->centreX());
	y = l->bottom() + padding;
}
void Layer::positionLeftOf(Layer *l, float padding) {
	setRight(l->x - padding);
	y = l->y;
}
void Layer::positionRightOf(Layer *l, float padding) {
	x = l->right() + padding;
	y = l->y;
}

void Layer::layoutChildrenAsGrid(int cols, int rows, float padding) {
	float w = (width - (cols - 1) * padding) / (float) cols;
	float h = (height - (rows - 1) * padding) / (float) rows;

	float wSpace = w + padding;
	float hSpace = h + padding;
	int pos		 = 0;
	for (auto *l: children) {
		if (l->visible) {
			l->width  = w;
			l->height = h;
			l->x	  = (pos % cols) * wSpace;
			l->y	  = (pos / cols) * hSpace;
			pos++;
		}
	}
}

void Layer::stackChildrenVertically(float padding) {
	int firstVisibleChild = -1;

	for (int i = 0; i < getNumChildren(); i++) {
		if (getChild(i)->visible) {
			firstVisibleChild = i;
			break;
		}
	}

	if (firstVisibleChild == -1) return;

	for (int i = 1; i < getNumChildren(); i++) {
		if (getChild(i)->visible) {
			getChild(i)->y = getChild(i - 1)->bottom() + padding;
		}
	}
}

void Layer::alignChildrenToPixels() {
	for (auto *ch: children) {
		ch->alignToPixels();
	}
}

Layer *Layer::getChild(const std::string &_name) {
	for (auto *ch: children) {
		if (ch->name == _name) return ch;
	}
	return nullptr;
}

bool Layer::_keyDown(int key) {
	if (!visible) return false;

	for (auto it = children.rbegin(); it != children.rend(); it++) {
		if ((*it)->_keyDown(key)) {
			return true;
		}
	}

	if (visible && keyDown(key)) {
		g.keyboardFocusedLayer = this;
		return true;
	}

	return false;
}
bool Layer::_keyUp(int key) {
	if (g.keyboardFocusedLayer) {
		g.keyboardFocusedLayer->keyUp(key);
		g.keyboardFocusedLayer = nullptr;
		return true;
	}
	return false;
}

Rectf Layer::thisAsRect() const {
	return Rectf {static_cast<const Rectf &>(*this)};
}
