
#pragma once

#include "Layer.h"
#include "mzgl_platform.h"
#ifndef MZGL_WIN
#	include <cxxabi.h>
#endif
class DebugOverlay : public Layer {
public:
	DebugOverlay(Graphics &g)
		: Layer(g, "DebugOverlay") {
		interactive = false;
		visible		= false;
	}
	void draw() override {
		if (mousedOverName.empty()) return;
		sendToFront();
		ScopedAlphaBlend b(g, true);
		g.setColor(1);

		g.drawTextWithBG(mousedOverName, vec4(0, 0, 0, 1), 20, 20);

		g.noFill();
		g.setColor(1, 0, 1);
		g.drawRect(mousedOverRect);
		g.fill();
		g.setColor(1, 0, 1, 0.25);
		g.drawRect(mousedOverRect);
	}
	std::string mousedOverName;
	Rectf mousedOverRect;
	std::string demangle(const char *name) {
#ifndef MZGL_WIN
		int status = -4; // some arbitrary value to eliminate the compiler warning

		// enable c++11 by passing the flag -std=c++11 to g++
		std::unique_ptr<char, void (*)(void *)> res {abi::__cxa_demangle(name, nullptr, nullptr, &status),
													 std::free};

		return (status == 0) ? res.get() : name;
#else
		return name;
#endif
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