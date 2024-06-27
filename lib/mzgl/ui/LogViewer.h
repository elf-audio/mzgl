//
//  LogViewer.h
//  mzgl
//
//  Created by Marek Bereza on 21/09/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once

#include <mzgl/ui/Scroller.h>
#include <mzgl/util/log.h>

class LogViewer : public Layer {
public:
	class CloseButton : public Layer {
	public:
		CloseButton(Graphics &g)
			: Layer(g) {
			interactive = true;
			width = height = 100;
		}
		std::function<void()> pressed;
		bool touchDown(float x, float y, int id) override {
			if (inside(x, y)) {
				if (pressed) pressed();
				return true;
			}
			return false;
		}
		void draw() override {
			g.setColor(1);
			g.drawCross(centre(), width, width * 0.2);
		}
	};
	class ClearButton : public Layer {
	public:
		ClearButton(Graphics &g)
			: Layer(g) {
			interactive = true;
			width = height = 100;
		}
		std::function<void()> pressed;
		bool touchDown(float x, float y, int id) override {
			if (inside(x, y)) {
				if (pressed) pressed();
				return true;
			}
			return false;
		}
		void draw() override {
			g.setColor(1);
			g.drawRect(*this);
			//			g.drawCross(centre(), width, width * 0.2);
		}
	};

	class LogWindow : public Layer {
	public:
		LogCapturer &logCapturer;
		LogWindow(Graphics &g, LogCapturer &logCapturer)
			: Layer(g)
			, logCapturer(logCapturer) {
			height = 2000;
			width  = 100;
			//			Log::Logger::addListener(this);
		}

		virtual ~LogWindow() {
			//			Log::Logger::removeListener(this);
		}
		std::function<void()> contentUpdated;
		//		std::vector<std::string> lines;
		float lineHeight = 20;

		void draw() override {
			lineHeight	  = g.getFont().getHeight("Ig");
			int newHeight = logCapturer.lines.size() * lineHeight;
			if (height != newHeight) {
				height = newHeight;
				contentUpdated();
			}

			g.setColor(1);
			//			for(int i = 0; i < logCapturer.lines.size(); i++) {
			int i = 0;
			for (auto &l: logCapturer.lines) {
				g.drawText(l, {20, (i + 1) * lineHeight});
				i++;
			}
		}
	};

	CloseButton *closeButton;
	ClearButton *clearButton;

	Scroller *scroller;
	LogWindow *logWindow;

	LogViewer(Graphics &g, LogCapturer &logCapturer)
		: Layer(g) {
		logWindow = new LogWindow(g, logCapturer);
		scroller  = new Scroller(g);
		scroller->addContent(logWindow);
		scroller->drawingScrollbar = true;
		interactive				   = true;

		addChild(scroller);
		logWindow->contentUpdated = [this]() { scroller->contentUpdated(); };

		closeButton			 = new CloseButton(g);
		closeButton->pressed = [this]() { visible = false; };

		clearButton			 = new ClearButton(g);
		clearButton->pressed = [this]() { logWindow->logCapturer.lines.clear(); };

		addChild(closeButton);
		addChild(clearButton);
	}
	void update() override {
		if (scroller->width != this->width - 40) {
			scroller->width	 = this->width;
			scroller->height = this->height;
			scroller->x		 = 0;
			scroller->y		 = 0;
			scroller->inset(20);
			scroller->contentUpdated();
			closeButton->setTopRight(scroller->tr());
			clearButton->setTopRight(closeButton->tl() + vec2(-20.f, 0.f));
		}
	}
	void draw() override {
		ScopedAlphaBlend ab(g, true);
		g.setColor(0, 0, 0, 0.5);
		g.drawRect(*this);
		g.setColor(1);
		g.noFill();
		Rectf r = *scroller;
		r.x += x;
		r.y += y;
		g.drawRect(r);
		g.fill();
	}
};
