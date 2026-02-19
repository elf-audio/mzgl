//
//  Layer.h
//
//
//

#pragma once

#include "Graphics.h"
#include <string>
#include "Texture.h"
#include <map>
#include "Font.h"
#include <functional>

class Layer : public Rectf {
public:
	std::string name;

	bool interactive = false;
	bool visible	 = true;

	Layer(Graphics &g, const std::string &name = "");
	Layer(Graphics &g, const std::vector<Layer *> &children);
	Layer(Graphics &g, const std::string &name, const std::vector<Layer *> &children);

	virtual ~Layer();

	virtual void draw() {}

	Layer *addChild(Layer *layer);
	bool removeChild(Layer *layer);
	void addChildren(std::vector<Layer *> layers);
	bool removeFromParent();

	virtual void clear();

	[[nodiscard]] std::string toString() const;

	bool clipToBounds			= false;
	bool receivesTouchesOutside = false;
	// reorder layers - if the parameter is null, it's referring to this
	// layer.
	void sendToFront(Layer *child = nullptr);
	void sendToBack(Layer *child = nullptr);

	bool isInFront() const;
	// override for when ui needs to be rebuilt
	virtual void doLayout() {}

	// override for touch events
	virtual void touchOver(float x, float y) {}
	virtual bool touchDown(float x, float y, int id) { return false; }
	virtual void touchMoved(float x, float y, int id) {}
	virtual void touchUp(float x, float y, int id) {}

	virtual bool mouseScrolled(float x, float y, float scrollX, float scrollY) { return false; }
	virtual bool mouseZoomed(float x, float y, float zoom) { return false; }

	virtual bool keyDown(int key) { return false; }
	virtual void keyUp(int key) {}

	// override to have something to do before the draw
	virtual void updateDeprecated() {}

	[[nodiscard]] int getNumChildren() const;
	Layer *getChild(int index);
	Layer *getFirstChild();
	Layer *getLastChild();

	// get a child by its name, returns null if it
	// doesn't find one
	Layer *getChild(const std::string &name);

	bool isVisible() const;
	// called by api, do not use
	virtual void _draw();
	void __draw();
	void layoutSelfAndChildren();
	void _touchOver(float x, float y);
	void _touchUp(float x, float y, int id);
	void _touchMoved(float x, float y, int id);
	bool _touchDown(float x, float y, int id);
	bool _mouseScrolled(float x, float y, float scrollX, float scrollY);
	bool _mouseZoomed(float x, float y, float zoom);
	bool _keyDown(int key);
	bool _keyUp(int key);
	void _updateDeprecated();

	Layer *getParent() const;
	Layer *getRoot();

	glm::vec2 getAbsolutePosition() const;
	Rectf getAbsoluteRect() const;

	glm::vec2 getAbsolutePosition(glm::vec2 p) const;
	Rectf getAbsoluteRect(const Rectf &r) const;
	void setAbsolutePosition(glm::vec2 p);
	glm::vec2 getLocalPosition(glm::vec2 pp) const;

	// rather than absolute coords, this tries to give you the rect
	// relative to an ancestor. If it can't find the ancestor, it will return false
	bool getRectRelativeTo(const Layer *l, Rectf &r) const;

	// same functionality as getLocalPosition()
	void absoluteToLocalCoords(float &xx, float &yy) const;

	// same functionality as getAbsolutePosition()
	void localToAbsoluteCoords(float &xx, float &yy) const;

	void setBottomCenter(float x, float y) {
		set(x - this->width / 2, y - this->height, this->width, this->height);
	}

	// attempt to pass focus from this layer to another

	void transferFocus(Layer *otherLayer, int touchId);
	void transferFocus(Layer *fromLayer, Layer *toLayer);
	[[nodiscard]] bool hasFocus() const;
	void removeFocus();

	void positionAbove(Layer *l, float padding = 0);
	void positionAboveCentred(Layer *l, float padding = 0);
	void positionUnder(Layer *l, float padding = 0);
	void positionUnderCentred(Layer *l, float padding = 0);
	void positionLeftOf(Layer *l, float padding = 0);
	void positionRightOf(Layer *l, float padding = 0);
	void layoutChildrenAsGrid(int cols, int rows, float padding = 0);
	void stackChildrenVertically(float padding = 0);
	void alignChildrenToPixels();

	// used for hacky things, don't use
	Graphics &getGraphics() { return g; }

	auto begin() { return std::begin(children); }
	auto begin() const { return std::begin(children); }
	auto end() { return std::end(children); }
	auto end() const { return std::end(children); }

	template <class T>
	std::vector<T *> collectChildrenOfType(bool recursive = false) {
		std::vector<T *> childrenOfType;
		for (auto child: children) {
			if (auto typedChild = dynamic_cast<T *>(child)) {
				childrenOfType.push_back(typedChild);
			}
			if (recursive) {
				auto subChildren = child->collectChildrenOfType<T>(recursive);
				childrenOfType.insert(std::end(childrenOfType), std::begin(subChildren), std::end(subChildren));
			}
		}
		return childrenOfType;
	}

	template <class T>
	void performOnAllChildrenOfType(const std::function<void(T *)> &perLayerFunction, bool recursive = false) {
		for (auto control: collectChildrenOfType<T>(recursive)) {
			perLayerFunction(control);
		}
	}

	[[nodiscard]] Rectf thisAsRect() const;

protected:
	void transformMouse(float &x, float &y);

	void maskOn();
	void maskOff();

	Graphics &g;

private:
	Layer *parent = nullptr;
	std::vector<Layer *> children;

#ifdef DEBUG
	int iteratingDepth = 0;
	void assertNotIterating(const char *operation);

	struct ScopedIterationGuard {
		Layer &layer;
		ScopedIterationGuard(Layer &l)
			: layer(l) {
			layer.iteratingDepth++;
		}
		~ScopedIterationGuard() { layer.iteratingDepth--; }
	};
#endif

	void pushMask();
	void popMask();
	// this gets abused because its not
	// really scoped usage, but it helps
	// to not reuse some code.
	ScopedMask scopedMask;

	//static void transformFocusedMouse(float &x, float &y);
	//static glm::vec2 focusedMouseTransform;
};

class ColouredRectLayer : public Layer {
public:
	ColouredRectLayer(Graphics &g)
		: Layer(g) {}
	vec4 color;
	void draw() override {
		if (color.a == 0) return;
		g.setColor(color);
		if (color.a < 1) {
			ScopedAlphaBlend scp(g, true);
			g.drawRect(*this);
		} else {
			g.drawRect(*this);
		}
	}
};

void setLayerSize(std::vector<Layer *> layers, float width, float height);
void setLayerSize(std::vector<Layer *> layers, float sz);
void setLayerSize(std::vector<Layer *> layers, vec2 sz);
void arrangeHorizontally(std::vector<Layer *> layers, float padding = 0);
void arrangeVertically(std::vector<Layer *> layers, float padding = 0);