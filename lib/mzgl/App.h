//
//  App.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

#include <string>

#include "Texture.h"
#include "Font.h"
#include "AudioSystem.h"
#include "Graphics.h"
#include "util.h"
#include "maths.h"
#include "Vbo.h"
#include "AudioSystem.h"
#include "RootLayer.h"
#include "Dialogs.h"
#include "mainThread.h"
#include "ScopedUrl.h"

class App : public AudioIO {
public:
	RootLayer *root = nullptr;
	Graphics &g;
	// maybe only implemented on mac
	virtual bool isHeadless() const { return false; }
	virtual bool isWebView() const { return false; }
	App(Graphics &g)
		: g(g)
		, dialogs(*this) {
		root = new RootLayer(g);
	}
	virtual ~App() {}

	virtual std::string getName() { return "hello"; }
	// KEYS
	virtual void keyDown(int key) {}
	virtual void keyUp(int key) {}

	virtual void iosViewWillPause(bool pausing) {}

	// TOUCHES
	virtual void touchOver(float x, float y) {}
	virtual void touchDown(float x, float y, int id) {}
	virtual void touchMoved(float x, float y, int id) {}
	virtual void touchUp(float x, float y, int id) {}

	// EVENTS
	virtual void resized() {}
	virtual void memoryWarning() {}

	// mouse scroll and zoom, mac only
	virtual void mouseScrolled(float x, float y, float dX, float dY) {}
	virtual void mouseZoomed(float x, float y, float zoom) {}

	// LIFE-CYCLE
	virtual void setup() {}

	virtual void update() {}
	virtual void draw() {}
	virtual void drawAfterUI() {}
	virtual void exit() {}

	//	virtual void audioIn(float *ins, int length, int numChannels) {}
	//	virtual void audioOut(float *outs, int length, int numChannels) {}

	virtual void didEnterBackground() {}
	virtual void willEnterForeground() {}

	// this is for if you have a custom url scheme,
	// return true if you handle the url, false if you
	// don't need it. Implemented for AudioShare initially.
	// Keep a reference to the ScopedUrlRef for as long as you
	// need to access the resource, because it may be deleted
	// once url destructs.
	virtual bool openUrl(ScopedUrlRef url) { return false; }

	// FILE DROP
	//	virtual void fileDragBegin(float x, float y, int touchId, int numFiles){}
	virtual void fileDragUpdate(float x, float y, int touchId, int numFiles) {}

	// return true to accept, false to reject
	virtual void filesDropped(const std::vector<ScopedUrlRef> &paths, int touchId) {}

	virtual void fileDragCancelled(int touchId) {}
	virtual void fileDragExited(float x, float y, int id) {}

	virtual void androidDrawLoading() {}

	virtual void androidOnPause() {}
	virtual void androidOnResume() {}
	// return true if you can open, false if you can't
	virtual bool canOpenFiles(const std::vector<std::string> &paths) { return false; }

	Dialogs dialogs;

	// only available on iOS
	void *viewController	 = nullptr;
	void *windowHandle		 = nullptr;
	void *nativeWindowHandle = nullptr;
	void *viewHandle		 = nullptr;

	MainThreadRunner main;

	void updateInternal() {
		++g.frameNum;
		main.pollMainThreadQueue();
	}
};

std::shared_ptr<App> instantiateApp(Graphics &g);
