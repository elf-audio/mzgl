//
//  NVGUI.h
//  midiloop
//
//  Created by Marek Bereza on 19/05/2020.
//  Copyright © 2020 elf-audio. All rights reserved.
//

#pragma once
#include "NVG.h"
#include <mzgl/animation/Tween.h>
#include <mzgl/util/maths.h>


namespace NVGUI {

	class NVGLayer : public Layer {
	public:
		NVG &nvg;
		NVGLayer(Graphics &g, NVG &nvg)
			: Layer(g)
			, nvg(nvg) {}
	};

	class Button : public NVGLayer {
	public:
		bool over = false;
		bool down = false;

		Button(Graphics &g, NVG &nvg)
			: NVGLayer(g, nvg) {
			x			= 100;
			y			= 400;
			width		= 150;
			height		= 50;
			interactive = true;
		}

		void draw() override {
			nvg.stroke({0.5, 0.5, 0.5, 1.f});
			nvg.strokeWidth(2);
			if (down) {
				nvg.fill({0.4, 0.4, 0.4, 1});
			} else if (over) {
				nvg.fill({0.6, 0.6, 0.6, 1});
			} else {
				//			nvg.fill({0.8, 0.8, 0.8, 1});
				nvg.fillGradientLinear({0.9, 0.9, 1.0, 1}, {1, 1, 0.7, 1}, {0, y}, {0, bottom()});
			}

			nvg.drawRoundedRect(*this, height / 4);
			nvg.fill({0, 0, 0, 1});

			nvg.drawTextCentred("Hola!", centre());
		}
		virtual void touchOver(float x, float y) override { over = inside(x, y); }
		virtual bool touchDown(float x, float y, int id) override {
			down = true;
			return true;
		}
		virtual void touchMoved(float x, float y, int id) override { down = inside(x, y); }
		virtual void touchUp(float x, float y, int id) override { down = false; }
	};

	class Slider : public NVGLayer {
	public:
		float min	 = 0;
		float max	 = 1;
		float *value = NULL;

		Slider(Graphics &g, NVG &nvg, float *value, float min = 0, float max = 1)
			: NVGLayer(g, nvg)
			, min(min)
			, max(max)
			, value(value) {
			width		= 200;
			height		= 30;
			interactive = true;
		}

		void draw() override {
			float radius		= height / 2;
			float railThickness = 12;
			float xx			= mapf(*value, min, max, x + radius, x + width - radius);

			nvg.noStroke();
			nvg.fill(hexColor(0x474747));
			//		// rail bg
			nvg.drawRoundedRect(
				Rectf(x + radius, y + radius - railThickness / 2, width - radius * 2, railThickness),
				railThickness / 2);
			//
			//		// highlight
			nvg.fill(hexColor(0x900000));
			nvg.drawRoundedRect(Rectf(x + radius,
									  y + radius - railThickness / 2,
									  mapf(*value, min, max, 0, width - radius * 2),
									  railThickness),
								railThickness / 2);

			nvg.fill(hexColor(0xb2b2b2));
			nvg.drawCircle({xx, y + radius}, radius);
		}

		bool touchDown(float x, float y, int id) override {
			float radius = height / 2;
			*value		 = mapf(x, this->x + radius, this->x + width - radius, min, max, true);
			return true;
		}

		void touchMoved(float x, float y, int id) override {
			float radius = height / 2;
			*value		 = mapf(x, this->x + radius, this->x + width - radius, min, max, true);
		}
	};

	class Toggle : public NVGLayer {
	public:
		Toggle(Graphics &g, NVG &nvg)
			: NVGLayer(g, nvg) {
			interactive = true;
			width		= 110;
			height		= 60;
		}

		RoundedRect outline;
		RoundedRect bg;
		RoundedRect handle;

		/*color.setHex(0x474747);
		 rail->color.setHex(0x232323);
		 handle->color.setHex(0xb2b2b2);*/
		void draw() override {
			nvg.noStroke();

			glm::vec4 bgC		= hexColor(0x474747);
			glm::vec4 highlight = hexColor(0x900000);

			nvg.fill(bgC * (1.f - amt) + highlight * amt);
			nvg.drawRoundedRect(*this, height / 2);
			Rectf r = *this;
			r.width = mapf(decidey, 0, 1, r.height, r.height + (r.width - r.height) * 0.3);
			r.x		= mapf(amt, 0, 1, x, x + width - height) - (r.width - r.height) * amt;
			int p	= 6;
			r.x += p;
			r.y += p;
			r.width -= p * 2;
			r.height -= p * 2;
			nvg.fill(hexColor(0xb2b2b2));
			nvg.drawRoundedRect(r, r.height / 2);

			nvg.noFill();
			nvg.stroke(hexColor(0x232323));
			nvg.drawRoundedRect(*this, height / 2);
		}

		bool down  = false;
		bool value = false;

		float amt	  = 0;
		float decidey = 0;

		bool touchDown(float x, float y, int id) override {
			down = true;
			EASE_OUT(decidey, 1, 0.25);
			return true;
		}

		void touchUp(float x, float y, int id) override {
			if (inside(x, y)) {
				value = !value;
				EASE_OUT(amt, value ? 1 : 0, 0.4);
				EASE_OUT(decidey, 0, 0.25);
			}
			down = false;
		}
	};

	/**
	 howto - add as a sibling under the modal,
	 add click() handler, and call start() and stop() to control it.
	 **/
	class Underlay : public NVGLayer {
	public:
		Underlay(Graphics &g, NVG &nvg)
			: NVGLayer(g, nvg) {
			visible		= false;
			amt			= 0;
			interactive = false;
			shouldShow	= false;
		}

		function<void()> clicked;
		bool shouldShow = false;

		virtual void start() {
			visible		= true;
			amt			= 0;
			interactive = true;
			shouldShow	= true;
		}

		void stop() {
			interactive = false;
			shouldShow	= false;
		}
		function<void()> underlayDone = []() {};
		float amt					  = 0;
		void update() override {
			if (!shouldShow) {
				if (amt > 0) {
					amt -= 0.1;
					if (amt < 0.01) {
						amt		= 0;
						visible = false;
						underlayDone();
					}
				}
			} else {
				amt += 0.1;
				if (amt > 1) amt = 1;
			}
		}


		virtual void draw() override {
			if (amt < 0.001) return;
			nvg.noStroke();
			nvg.fill({0, 0, 0, smootherstep(amt) * 0.5});
			width  = g.width;
			height = g.height;
			nvg.drawRect(*this);
		}

		virtual bool touchDown(float x, float y, int id) override {
			if (clicked) {
				clicked();
			} else {
				printf("ERROR: no clicked defined for this underlay\n");
			}
			return true;
		}
	};

	class Label : public NVGLayer {
	public:
		string name;
		Label(Graphics &g, NVG &nvg, string text)
			: NVGLayer(g, nvg)
			, name(text) {}
		void draw() override {
			nvg.fill(hexColor(0xCCCCCC));
			nvg.noStroke();
			nvg.drawText(name, {x, y + height / 2}, -1, 0);
		}
	};

	class DropDown : public NVGLayer {
	public:
		Underlay *underlay;

		vector<string> options;
		function<void(int)> itemSelected = [](int) {};
		int selectedIndex				 = 0;
		bool out						 = false;
		float outAmt					 = 0;

		float originalHeight;

		DropDown(Graphics &g, NVG &nvg, vector<string> options)
			: NVGLayer(g, nvg)
			, options(options) {
			interactive			   = true;
			underlay			   = new Underlay(g, nvg);
			underlay->clicked	   = [this]() { dismiss(); };
			underlay->underlayDone = [this]() {
				if (underlay->getParent() != nullptr) {
					underlay->removeFromParent();
				}
			};
			width		   = 150;
			height		   = 50;
			originalHeight = height;
		}

		void update() override {
			if (out) outAmt += 0.1;
			else outAmt -= 0.1;

			outAmt = std::clamp(outAmt, 0.f, 1.f);

			if (outAmt == 0) {
				originalHeight = height;
			}
			height = max(originalHeight, originalHeight * options.size() * outAmt);

			if (y + height > g.height) {
				y = g.height - height;
			}
		}

		int hoveredIndex = 0;
		void draw() override {
			const float cornerRadius = originalHeight / 4.f;
			const float textIndent	 = 9;

			auto bgColor = hexColor(0xcccccc);
			nvg.noStroke();
			nvg.fill(bgColor);
			nvg.drawRoundedRect(*this, cornerRadius);
			auto normalTextColor = hexColor(0x333333);
			if (out) {
				for (int i = 0; i < options.size(); ++i) {
					auto textColor = hexColor(0x333333);
					if (i == hoveredIndex || i == selectedIndex) { // highlight

						if (selectedIndex == hoveredIndex) {
							nvg.fill(hexColor(0x900000));
							textColor = hexColor(0xFFFFFF);
						} else if (i == hoveredIndex) {
							nvg.fill(hexColor(0xaaaaaa));
							textColor = normalTextColor;
						} else {
							nvg.fill(hexColor(0x900000));
							textColor = hexColor(0xFFFFFF);
						}
						if (i == 0) {
							nvg.drawRoundedRect({x, y + originalHeight * i, width, originalHeight},
												cornerRadius,
												cornerRadius,
												0,
												0);
						} else if (i == options.size() - 1) {
							nvg.drawRoundedRect({x, y + originalHeight * i, width, originalHeight},
												0,
												0,
												cornerRadius,
												cornerRadius);
						} else {
							nvg.drawRect({x, y + originalHeight * i + 1, width, originalHeight - 2});
						}
					}
					nvg.fill(textColor);
					nvg.drawText(options[i],
								 {x + textIndent, y + (originalHeight * i * outAmt + originalHeight / 2)},
								 -1,
								 0);
				}
			} else {
				//				nvg.fill(bgColor);
				//				nvg.drawRoundedRect({x, y, width, height}, originalHeight/4);
				nvg.fill(normalTextColor);
				nvg.drawText(options[selectedIndex], {x + textIndent, y + height / 2}, -1, 0);

				// draw triangle
				vec2 c = {x + width - 25, y + originalHeight / 2 + 5};
				nvg.drawTriangle(c + vec2(-8, -10), c + vec2(8, -10), c);
			}
		}

		bool brokenOut = false;
		glm::vec2 startPos;
		bool startedSelecting = false;
		Layer *originalParent = nullptr;
		glm::vec2 originalPosition;

		void touchOver(float x, float y) override {
			if (inside(x, y)) {
				hoveredIndex = std::clamp((y - this->y) / originalHeight, 0.f, options.size() - 1.f);
			} else {
				hoveredIndex = -1;
			}
		}

		bool touchDown(float x, float y, int id) override {
			if (!out) {
				out		  = true;
				brokenOut = false;
				if (underlay->getParent() == nullptr) {
					getRoot()->addChild(underlay);
				}

				startPos		 = getParent()->getAbsolutePosition() + vec2(x, y);
				originalPosition = position();
				auto pos		 = getAbsolutePosition();
				originalParent	 = getParent();

				removeFromParent();
				underlay->addChild(this);
				position(pos);

				underlay->start();

				sendToFront();
				startedSelecting = false;
			} else {
				if (brokenOut) {
					startedSelecting = true;
					int newIndex	 = std::clamp((y - this->y) / originalHeight, 0, options.size() - 1);
					if (selectedIndex != newIndex) {
						selectedIndex = newIndex;
						itemSelected(selectedIndex);
					}
				}
			}
			return true;
		}
		void touchMoved(float x, float y, int id) override {
			if (out) {
				vec2 ap = getParent()->getAbsolutePosition();
				if (glm::distance({ap.x + x, ap.y + y}, startPos) > 20) {
					brokenOut = true;
				}
				if (brokenOut) {
					startedSelecting = true;

					int newIndex = std::clamp((y - this->y) / originalHeight, 0, options.size() - 1);
					if (selectedIndex != newIndex) {
						selectedIndex = newIndex;
						itemSelected(selectedIndex);
					}
				}
			}
		}

		void touchUp(float x, float y, int id) override {
			if (startedSelecting) dismiss();
			brokenOut = true;
		}

		void dismiss() {
			underlay->stop();
			position(originalPosition);
			removeFromParent();
			originalParent->addChild(this);

			out = false;
		}

	private:
	};
}; // namespace NVGUI
