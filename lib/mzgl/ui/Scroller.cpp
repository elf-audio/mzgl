
#include "Scroller.h"
#include <algorithm>
#include <cmath>

namespace {
// UIKit "Normal" deceleration: 0.998 per ms.
constexpr float kDecelerationPerMs = 0.998f;
// Rubber-band constant. Higher = stronger resistance / lower asymptote (max overshoot = dim/c).
// Apple's reported value is 0.55 (asymptote ~1.82 viewport). Bumped for tighter feel.
constexpr float kRubberBandC = 3.0f;
// Spring (critically damped: c = 2*sqrt(k)).
constexpr float kSpringK = 250.0f;
constexpr float kSpringC = 31.62f;
// Velocity sample window for fling.
constexpr double kVelocityWindow = 0.1;
// On release, last sample must be within this to fling.
constexpr double kFlingMaxSampleAge = 0.05;
// Per-frame dt clamp (avoid huge steps after stalls).
constexpr double kMaxDt = 1.0 / 30.0;

// Apple rubber-band: content offset given finger overshoot past edge.
// y = F * dim / (F * c + dim)  — at F=0 maps 1:1, asymptotes to dim/c.
float rubberBandOffset(float fingerOvershoot, float dim) {
	if (dim <= 0.f) return 0.f;
	return fingerOvershoot * dim / (fingerOvershoot * kRubberBandC + dim);
}
} // namespace

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
	double dt = std::clamp(static_cast<double>(g.frameDelta), 0.0, kMaxDt);

	if (scrolling) {
		scrollbarActivityAmt = std::min(1.0f, scrollbarActivityAmt + 0.1f);
		return;
	}

	// allowed range for content->y (top-anchored).
	float maxY = 0.f;
	float minY = (content->height > height) ? (height - content->height) : 0.f;

	bool overTop = content->y > maxY;
	bool overBot = content->y < minY;

	if (overTop || overBot) {
		float target = overTop ? maxY : minY;
		float disp	 = content->y - target;
		// critically damped spring
		contentVelocity.y += (-kSpringK * disp - kSpringC * contentVelocity.y) * static_cast<float>(dt);
		content->y += contentVelocity.y * static_cast<float>(dt);
		if (std::abs(disp) < 0.5f && std::abs(contentVelocity.y) < 1.0f) {
			content->y		  = target;
			contentVelocity.y = 0.f;
		}
	} else {
		contentVelocity.y *= std::pow(kDecelerationPerMs, static_cast<float>(dt * 1000.0));
		content->y += contentVelocity.y * static_cast<float>(dt);
		if (std::abs(contentVelocity.y) < 1.0f) {
			contentVelocity.y = 0.f;
		}
	}

	if (std::abs(contentVelocity.y) < 1.0f) {
		scrollbarActivityAmt -= 0.1f;
		if (scrollbarActivityAmt < 0.f) scrollbarActivityAmt = 0.f;
	}
}

void Scroller::draw() {
	if (color.a == 0) return;
	g.setColor(color);
	g.drawRect(*this);
}

void Scroller::drawSelfAndChildren() {
	if (!visible) return;
	Layer::drawSelfAndChildren();
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
	scrolling		  = false;
	contentVelocity.y = 0.f;

	double now = g.currFrameTime;
	// drop samples outside fling window
	while (!velocitySamples.empty() && now - velocitySamples.front().first > kVelocityWindow) {
		velocitySamples.pop_front();
	}
	if (velocitySamples.size() >= 2) {
		const auto &front = velocitySamples.front();
		const auto &back  = velocitySamples.back();
		double sampleDt	  = back.first - front.first;
		// last move must be recent — finger paused → no fling
		if (sampleDt > 0.001 && now - back.first < kFlingMaxSampleAge) {
			contentVelocity.y = static_cast<float>((back.second - front.second) / sampleDt);
		}
	}
	velocitySamples.clear();
}

void Scroller::touchMoved(float x, float y, int id) {
	if (!scrolling) return;

	float maxY = 0.f;
	float minY = (content->height > height) ? (height - content->height) : 0.f;

	// raw position if no rubber band (finger 1:1).
	float rawY = dragAnchorContentY + (y - dragAnchorTouchY);

	if (rawY > maxY) {
		content->y = maxY + rubberBandOffset(rawY - maxY, height);
	} else if (rawY < minY) {
		content->y = minY - rubberBandOffset(minY - rawY, height);
	} else {
		content->y = rawY;
	}

	double now = g.currFrameTime;
	velocitySamples.push_back({now, content->y});
	while (!velocitySamples.empty() && now - velocitySamples.front().first > kVelocityWindow) {
		velocitySamples.pop_front();
	}

	lastTouch = glm::vec2(x, y);
}

bool Scroller::touchDown(float x, float y, int id) {
	if (inside(x, y)) {
		lastTouch		  = glm::vec2(x, y);
		scrolling		  = true;
		contentVelocity.y = 0.f;
		// anchor for rubber-band drag mapping
		dragAnchorContentY = content->y;
		dragAnchorTouchY   = y;
		velocitySamples.clear();
		velocitySamples.push_back({g.currFrameTime, content->y});
		return true;
	} else {
		return false;
	}
}

void Scroller::setContentHeight(float contentHeight) {
	this->content->height	   = contentHeight;
	contentHeightExplicitlySet = true;
}
bool Scroller::mouseScrolled(float x, float y, float scrollX, float scrollY) {
	if (content->height <= height) {
		return true;
	}
	// Direct position update — trackpad/wheel deltas already encode finger motion + OS momentum.
	// Accumulating into velocity caused trailing decay even when fingers rested motionless.
	contentVelocity.y = 0.f;

	float maxY = 0.f;
	float minY = (height - content->height);

	float rawY = content->y + scrollY * 8.f;

	if (rawY > maxY) {
		content->y = maxY + rubberBandOffset(rawY - maxY, height);
	} else if (rawY < minY) {
		content->y = minY - rubberBandOffset(minY - rawY, height);
	} else {
		content->y = rawY;
	}
	return true;
}
