
#pragma once

#include "Layer.h"
#include <cxxabi.h>
class DebugOverlay : public Layer {
public:
	DebugOverlay(Graphics &g)
		: Layer(g, "DebugOverlay") {
		interactive = false;
		visible		= false;
	}
	void draw() override {
		sendToFront();
		ScopedAlphaBlend b(g, true);

		g.setColor(1);
		g.drawText(mousedOverName, 20, 20);
		if (!mousedOverName.empty()) {
			g.noFill();
			g.setColor(1, 0, 1);
			g.drawRect(mousedOverRect);
			g.fill();
			g.setColor(1, 0, 1, 0.25);
			ScopedAlphaBlend b(g, true);
			g.drawRect(mousedOverRect);
		}
	}
	std::string mousedOverName;
	Rectf mousedOverRect;
	std::string demangle(const char *name) {
		int status = -4; // some arbitrary value to eliminate the compiler warning

		// enable c++11 by passing the flag -std=c++11 to g++
		std::unique_ptr<char, void (*)(void *)> res {abi::__cxa_demangle(name, NULL, NULL, &status), std::free};

		return (status == 0) ? res.get() : name;
	}
	Layer *insideWhich(Layer *l, float x, float y) {
		if (!l->visible) return nullptr;
		if (l == this) return nullptr;
		for (int i = l->getNumChildren() - 1; i >= 0; i--) {
			auto *c = l->getChild(i);
			auto *r = insideWhich(c, x - l->x, y - l->y);
			if (r != nullptr) return r;
		}
		if (l->inside(x, y)) return l;
		return nullptr;
	}
	void touchOver(float x, float y) override {
		auto *l = insideWhich(getParent(), x, y);

		if (!l) return;
		mousedOverRect = l->getAbsoluteRect();
		mousedOverName = demangle(typeid(*l).name());
		if (!l->name.empty()) {
			mousedOverName += " - " + l->name;
		}
	}
	void doLayout() override { size(g.width, g.height); }
};