//
//  EventDispatcher.h
//  MZGL
//
//  Created by Marek Bereza on 24/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include "WebViewApp.h"
#include "ScopedUrl.h"
#include "util.h"
#include "GraphicsAPI.h"
#include <chrono>
#include <cstdint>
#include <cstdio>

class EventDispatcher {
public:
	EventDispatcher(std::shared_ptr<App> app) { this->app = app; }

	virtual ~EventDispatcher() { printf("Bye bye event dispatcher\n"); }

	void androidDrawLoading() { app->androidDrawLoading(); }

	// KEYS
	void keyDown(int key) {
		if (app->root->_keyDown(key)) {
			return;
		}
		app->keyDown(key);
	}

	void keyUp(int key) {
		if (app->root->_keyUp(key)) {
			return;
		}
		app->keyUp(key);
	}

	void iosViewWillPause(bool pausing) { app->iosViewWillPause(pausing); }

	virtual bool canOpenFiles(const std::vector<std::string> &paths) { return app->canOpenFiles(paths); }

	//	virtual void fileDragBegin(float x, float y, int touchId, int numItems) {
	//		app->fileDragBegin(x, y, touchId, numItems);
	//	}

	virtual void fileDragUpdate(float x, float y, int touchId, int numItems) {
		app->fileDragUpdate(x, y, touchId, numItems);
	}

	void filesDropped(const std::vector<ScopedUrlRef> &paths, int touchId) { app->filesDropped(paths, touchId); }
	void fileDragExited(float x, float y, int id) { app->fileDragExited(x, y, id); }

	void fileDragCancelled(int touchId) { return app->fileDragCancelled(touchId); }

	bool openUrl(ScopedUrlRef url) { return app->openUrl(url); }

	// TOUCHES
	void touchOver(float x, float y) {
		mouse = glm::vec2(x, y);
		app->touchOver(x, y);
		app->root->_touchOver(x, y);
	}

	void touchDown(float x, float y, int id) {
		mouse			= glm::vec2(x, y);
		downTouches[id] = true;
		app->touchDown(x, y, id);
		app->root->_touchDown(x, y, id);
	}

	void touchMoved(float x, float y, int id) {
		mouse			= glm::vec2(x, y);
		downTouches[id] = true;
		app->touchMoved(x, y, id);
		app->root->_touchMoved(x, y, id);
	}

	void touchUp(float x, float y, int id) {
		mouse			= glm::vec2(x, y);
		downTouches[id] = false;
		app->touchUp(x, y, id);
		app->root->_touchUp(x, y, id);
	}

	void mouseScrolled(float x, float y, float dx, float dy) {
		app->mouseScrolled(x, y, dx, dy);
		app->root->_mouseScrolled(x, y, dx, dy);
	}

	void mouseZoomed(float x, float y, float zoom) {
		app->mouseZoomed(x, y, zoom);
		app->root->_mouseZoomed(x, y, zoom);
	}

	void didEnterBackground() { app->didEnterBackground(); }

	void willEnterForeground() { app->willEnterForeground(); }

	void memoryWarning() { app->memoryWarning(); }

	void exit() { app->exit(); }

	// EVENTS
	void resized() { app->resized(); }

	// LIFE-CYCLE
	void setup() {
		app->setup();
		hasCalledSetup = true;
	}

	bool hasSetup() { return hasCalledSetup; }

	void updateDeprecated() {
		app->updateDeprecated();
		app->g.runRegisteredUpdaters();
	}

	void draw() {
		app->draw();
		app->root->drawSelfAndChildren();
		app->drawAfterUI();
	}

	void androidOnPause() { app->androidOnPause(); }

	void androidOnResume() { app->androidOnResume(); }

	std::shared_ptr<App> app;
	glm::vec2 mouse;
	std::map<int, bool> downTouches;
	bool hasCalledSetup = false;

	void runFrame() {
#ifdef DO_DRAW_STATS
		Vbo::resetDrawStats();
#endif
		app->g.setupView();
		app->updateInternal();

		updateFrame();
	}

	void updateFrame() {
		updateDeprecated();

		auto s = getSeconds();

		app->g.frameDelta	 = s - app->g.currFrameTime;
		app->g.currFrameTime = s;
		app->g.frameNum++;

		if (perfEnabled) {
			auto _t0   = std::chrono::steady_clock::now();
			draw();
			auto _t1   = std::chrono::steady_clock::now();
			double ms  = std::chrono::duration<double, std::milli>(_t1 - _t0).count();
			perfRecord(ms, s);
		} else {
			draw();
		}
	}

	// --- draw-call perf instrumentation (enabled with --perf-log) ---
	// Times just the draw() call (UI render command submission, excludes vsync/swap)
	// and prints a one-line summary once per second to stdout for offline parsing.
	bool perfEnabled		= hasCommandLineFlag("--perf-log");
	bool perfStarted		= false;
	double perfRunStart		= 0.0;
	double perfWindowStart	= 0.0;
	uint64_t perfFramesWindow = 0;
	double perfMsWindow		= 0.0;
	double perfMinWindow	= 1e30;
	double perfMaxWindow	= 0.0;

	void perfRecord(double drawMs, double now) {
		if (!perfStarted) {
			perfStarted		= true;
			perfRunStart	= now;
			perfWindowStart = now;
			printf("[PERF] backend=%s draw-call timing started\n",
				   app->g.getAPI().getBackendName().c_str());
			fflush(stdout);
		}
		perfFramesWindow++;
		perfMsWindow += drawMs;
		if (drawMs < perfMinWindow) perfMinWindow = drawMs;
		if (drawMs > perfMaxWindow) perfMaxWindow = drawMs;

		double winElapsed = now - perfWindowStart;
		if (winElapsed >= 1.0) {
			double avg = perfMsWindow / static_cast<double>(perfFramesWindow);
			double fps = static_cast<double>(perfFramesWindow) / winElapsed;
			// draw_load_pct = fraction of one CPU core spent in draw() (avg_ms * fps / 10)
			printf("[PERF] t=%6.2f backend=%s fps=%5.1f frames=%llu "
				   "draw_avg_ms=%.4f draw_min_ms=%.4f draw_max_ms=%.4f draw_load_pct=%.2f\n",
				   now - perfRunStart,
				   app->g.getAPI().getBackendName().c_str(),
				   fps,
				   static_cast<unsigned long long>(perfFramesWindow),
				   avg,
				   perfMinWindow,
				   perfMaxWindow,
				   avg * fps / 10.0);
			fflush(stdout);
			perfFramesWindow = 0;
			perfMsWindow	 = 0.0;
			perfMinWindow	 = 1e30;
			perfMaxWindow	 = 0.0;
			perfWindowStart	 = now;
		}
	}

	virtual void receivedJSMessage(const std::string &key, const std::string &value) {
		std::dynamic_pointer_cast<WebViewApp>(app)->receivedJSMessage(key, value);
	}
};
