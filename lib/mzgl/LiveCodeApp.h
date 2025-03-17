//
//  LiveCodeApp.h
//  MZGL
//
//  Created by Marek Bereza on 23/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include "App.h"
#include "RecompilingAppDylib.h"
#include "TextConsole.h"
#include "MacMenuBar.h"

#include "NSEventDispatcher.h"
#include "filesystem.h"

class LiveCodeApp : public App {
public:
	RecompilingAppDylib compiler;

	App *d = nullptr;

	TextConsole errorConsole;

	std::shared_ptr<_AudioSystem> audioSystem;

	void addFileToWatch(std::string path) { compiler.addFileToWatch(path); }

	LiveCodeApp(Graphics &g, std::string headerFile, std::shared_ptr<_AudioSystem> audioSystem = nullptr)
		: App(g)
		, compiler(g) {
		this->audioSystem = audioSystem;
		compiler.setup(headerFile);

		// watch any other header files for changes, but not any .cpp files
		// because that's too complicated for now

		fs::path srcDir(SRCROOT);
		for (const auto &entry: fs::directory_iterator(srcDir)) {
			auto p = entry.path();
			if (p.extension() == ".h" && p.filename() != headerFile) {
				compiler.addFileToWatch(p.string());
			}
		}

		compiler.willCloseDylib = [&]() {
			this->exit();
			root->clear();
			NSEventDispatcher::instance().clearListeners();
		};
		compiler.recompiled = [&](void *appPtr) {
			//root.clear();
			// this is a memory leak, right here.
			// don't know why I can't fix it.
			//			if(d != nullptr) {
			//				delete d;
			//				d = nullptr;
			//			}
			d = (App *) appPtr;

			if (d != nullptr) {
				root->addChild(d->root);
				d->setup();
			}
		};

		errorConsole.show();

		compiler.successCallback = [&]() {
			errorConsole.setText("Build Succeeded");
			errorConsole.setBgColor(glm::vec3(0, 0, 0));
		};

		compiler.failureCallback = [&](std::string errorMsg) {
			errorConsole.setText(errorMsg);
			errorConsole.setBgColor(glm::vec3(0.25, 0, 0));
		};
		//MacMenuBar::instance().getMenu("LiveCoding")->addItem("Reload", "r", [this]() {compiler.recompile();});
	}

	void updateDeprecated() override {
		compiler.update();
		if (d != nullptr) {
			d->update();
		}
	}

	void audioIn(float *ins, int length, int numChannels) override {
		compiler.lock();
		if (d) d->audioIn(ins, length, numChannels);
		compiler.unlock();
	}

	void audioOut(float *outs, int length, int numChannels) override {
		compiler.lock();
		if (d) d->audioOut(outs, length, numChannels);
		compiler.unlock();
	}

	// FORWARD ALL THE OTHER EVENTS

	// KEYS
	void keyDown(int key) override {
		if (d) d->keyDown(key);
	}
	void keyUp(int key) override {
		if (d) d->keyUp(key);
	}

	// TOUCHES
	void touchOver(float x, float y) override {
		if (d) d->touchOver(x, y);
	}
	void touchDown(float x, float y, int id) override {
		if (d) d->touchDown(x, y, id);
	}
	void touchMoved(float x, float y, int id) override {
		if (d) d->touchMoved(x, y, id);
	}
	void touchUp(float x, float y, int id) override {
		if (d) d->touchUp(x, y, id);
	}

	void mouseScrolled(float x, float y, float dX, float dY) override {
		if (d) d->mouseScrolled(x, y, dX, dY);
	}

	// EVENTS
	void resized() override {
		if (d) d->resized();
	}

	// LIFE-CYCLE
	void setup() override {
		if (d) d->setup();
	}
	void draw() override {
		if (d) d->draw();
	}
	void drawAfterUI() override {
		if (d) d->drawAfterUI();
	}

	void exit() override {
		if (d) d->exit();
	}
	// FILE DROP
	//	virtual void fileDragBegin(float x, float y) override {if(d) d->fileDragBegin(x,y);}
	virtual void fileDragUpdate(float x, float y, int touchId, int numFiles) override {
		if (d) d->fileDragUpdate(x, y, touchId, numFiles);
	}

	// return true to accept, false to reject
	virtual void filesDropped(const std::vector<ScopedUrlRef> &paths, int touchId) override {
		if (d) d->filesDropped(paths, touchId);
	}

	// return true if you can open, false if you can't
	virtual bool canOpenFiles(const std::vector<std::string> &paths) override {
		if (d) return d->canOpenFiles(paths);
		else return false;
	}
};
