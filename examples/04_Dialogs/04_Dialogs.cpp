
#include "App.h"
#include <cmath>
#include "mzAssert.h"
#include "Dialogs.h"

/**
 * Very very simple button ui element class
 */
class Button : public Layer {
public:
	Button(Graphics &g, const std::string &name)
		: Layer(g, name) {
		interactive = true;
	}

	void draw() override {
		g.fill();

		if (down) {
			g.setColor(0);
			g.drawRoundedRect(*this, 5);
		} else if (over) {
			g.setColor(0.4);
			g.drawRoundedRect(*this, 5);
		}

		g.setColor(1);
		g.noFill();
		g.drawRoundedRect(*this, 5);

		g.drawTextCentred(name, centre());
	}

	std::function<void()> released = []() {};

	bool over = false;
	bool down = false;
	void touchOver(float x, float y) override { over = inside(x, y); }
	bool touchDown(float x, float y, int id) override {
		down = true;
		return true;
	}

	void touchMoved(float x, float y, int id) override { down = inside(x, y); }
	void touchUp(float x, float y, int id) override {
		if (inside(x, y)) {
			released();
		}
		down = false;
	}
};

class MyApp : public App {
public:
	Dialogs dialogs;
	MyApp(Graphics &g)
		: App(g)
		, dialogs(*this) {
		auto alertButton		   = new Button(g, "SHOW ALERT");
		auto twoOptionCancelButton = new Button(g, "SHOW TWO OPTION CANCEL");
		auto chooseImageButton	   = new Button(g, "CHOOSE IMAGE");
		auto shareButton		   = new Button(g, "SHARE");
		auto loadButton			   = new Button(g, "LOAD FILE");
		auto launchUrlButton	   = new Button(g, "LAUNCH URL");

		////////////////////////////////////////////////////////////////////////////////////
		// actions

		alertButton->released = [this]() { dialogs.alert("TITLE", "MESSAGE"); };

		twoOptionCancelButton->released = [this]() {
			dialogs.twoOptionCancelDialog(
				"TITLE",
				"MSG",
				"BUTTON ONE",
				[this]() { dialogs.alert("", "button one pressed"); },
				"BUTTON TWO",
				[this]() { dialogs.alert("", "button two pressed"); },

				// cancel
				[this]() { dialogs.alert("", "cancel pressed"); });
		};

		chooseImageButton->released = [this]() {
			dialogs.chooseImage([this](bool success, std::string path) {
				dialogs.alert("", "LOAD IMAGE - success: " + std::to_string(success) + " path: " + path);
			});
		};

		shareButton->released = [this]() {
			dialogs.share("SHARE THIS FILE", "/path/to/real/file", [this](bool success) {
				dialogs.alert("", "SHARE FILE - success: " + std::to_string(success));
			});
		};

		loadButton->released = [this]() {
			dialogs.loadFile("CHOOSE A FILE", [this](std::string path, bool success) {
				dialogs.alert("", "LOAD FILE - success: " + std::to_string(success) + " path: " + path);
			});
		};

		launchUrlButton->released = [this]() {
			dialogs.launchUrlInWebView("https://www.koalasampler.com", []() {});
		};

		////////////////////////////////////////////////////////////////////////////////////
		// do the layout
		const float buttonWidth	 = g.width * 0.5;
		const float buttonHeight = g.height * 0.1;
		const float padding		 = 20;

		alertButton->setFromCentre({g.width / 2, buttonHeight}, buttonWidth, buttonHeight);

		root->addChildren(
			{alertButton, twoOptionCancelButton, chooseImageButton, shareButton, loadButton, launchUrlButton});
		for (int i = 1; i < root->getNumChildren(); i++) {
			root->getChild(i)->size(alertButton->size());
			root->getChild(i)->positionUnder(root->getChild(i - 1), padding);
		}
	}

	void draw() override {
		// grey background
		g.clear(0.2);
	}
};

std::shared_ptr<App> instantiateApp(Graphics &g) {
	g.width	 = 800;
	g.height = 800;
	return std::make_shared<MyApp>(g);
}
