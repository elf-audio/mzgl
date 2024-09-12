#pragma once
#include "Layer.h"
#include "util.h"
class WindowLayer : public Layer {
public:
	WindowLayer(Graphics &g, const std::string &name = "")
		: Layer(g, name) {
		interactive = true;
	}

	void toggle() {
		if (width == 0) {
			set(0, 0, g.width, g.height);
			inset(20);
		}
		visible = !visible;
		if (!visible) setCursor(Cursor::ARROW);
		toggled(visible);
	}
	void draw() override {
		g.setColor(1, 0, 1);
		g.fill();
		g.drawRect(*this);
	}

	std::function<void(bool)> toggled = [](bool) {};
	std::function<void()> resized	  = []() {};

	void touchOver(float x, float y) override {
		auto edge = whichDragType(vec2(x, y));
		setCursorForDragType(edge);
	}

	bool touchDown(float x, float y, int id) override {
		startTouch	 = {x, y};
		currDragType = whichDragType(startTouch);
		setCursorForDragType(currDragType);
		startRect = *this;
		return true;
	}
	void touchMoved(float x, float y, int id) override {
		Rectf originalRect			 = *this;
		static constexpr int minSize = 40;
		switch (currDragType) {
			case DragType::TopLeft: moveTopLeft(startRect.tl() + vec2(x, y) - startTouch); break;
			case DragType::TopRight: moveTopRight(startRect.tr() + vec2(x, y) - startTouch); break;
			case DragType::BottomRight: moveBottomRight(startRect.br() + vec2(x, y) - startTouch); break;
			case DragType::BottomLeft: moveBottomLeft(startRect.bl() + vec2(x, y) - startTouch); break;

			case DragType::Top: moveTopEdge(startRect.y + y - startTouch.y); break;
			case DragType::Bottom: moveBottomEdge(startRect.bottom() + y - startTouch.y); break;
			case DragType::Left: moveLeftEdge(startRect.x + x - startTouch.x); break;
			case DragType::Right: moveRightEdge(startRect.right() + x - startTouch.x); break;
			case DragType::Inside: position(startRect.tl() + vec2(x, y) - startTouch); break;

			case DragType::None: break;
		}
		if (width < minSize || height < minSize) {
			set(originalRect);
		}
		if (*this != originalRect) {
			resized();
		}
	}
	void touchUp(float x, float y, int id) override { setCursor(Cursor::ARROW); }

private:
	vec2 startTouch;
	Rectf startRect;
	enum class DragType { Inside, TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left, None };

	DragType currDragType = DragType::None;
	bool isCorner(DragType edge) {
		return edge == DragType::TopLeft || edge == DragType::TopRight || edge == DragType::BottomRight
			   || edge == DragType::BottomLeft;
	}
	DragType whichDragType(vec2 p) {
		if (!inside(p)) return DragType::None;
		float minDist = 20;
		if (glm::distance(p, tl()) < minDist) return DragType::TopLeft;
		if (glm::distance(p, tr()) < minDist) return DragType::TopRight;
		if (glm::distance(p, bl()) < minDist) return DragType::BottomLeft;
		if (glm::distance(p, br()) < minDist) return DragType::BottomRight;
		if (p.y - this->y < minDist) return DragType::Top;
		if (right() - p.x < minDist) return DragType::Right;
		if (bottom() - p.y < minDist) return DragType::Bottom;
		if (p.x - this->x < minDist) return DragType::Left;
		return DragType::Inside;
	}

	void setCursorForDragType(DragType edge) {
		if (isCorner(edge)) {
			setCursor(Cursor::CROSSHAIR);
		} else if (edge == DragType::Top || edge == DragType::Bottom) {
			setCursor(Cursor::UP_DOWN_RESIZE);
		} else if (edge == DragType::Left || edge == DragType::Right) {
			setCursor(Cursor::LEFT_RIGHT_RESIZE);
		} else if (edge == DragType::None) {
			setCursor(Cursor::ARROW);
		} else {
			setCursor(Cursor::OPEN_HAND);
		}
	}
};