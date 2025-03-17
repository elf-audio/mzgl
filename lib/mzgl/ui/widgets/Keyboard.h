//
//  Keyboard.h
//  autosampler
//
//  Created by Marek Bereza on 16/03/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#pragma once

class Key : public Layer {
public:
	bool shouldBeOn = false;
	bool isOn		= false;
	bool isBlack	= false;
	Key(bool isBlack)
		: Layer()
		, isBlack(isBlack) {
		cornerRadius = 9;
	}

	void update() override {
		if (isOn) color = {1, 0.5, 0.5, 1};
		else {
			if (isBlack) {
				color = {0, 0, 0, 1};
			} else {
				color = {1, 1, 1, 1};
			}
		}
	}
};

class Keyboard : public Layer {
public:
	bool isBlack(int i) {
		i %= 12;
		return (i == 1 || i == 3 || i == 6 || i == 8 || i == 10);
	}

	int getNumNotes() { return numKeys; }

	Keyboard()
		: Layer() {
		interactive = true;
		height		= HEIGHT;
		width		= WIDTH / 2;
		float kh	= height / (float) numKeys;
		for (int i = 0; i < numKeys; i++) {
			auto *k	  = new Key(isBlack(i));
			k->width  = width;
			k->height = kh;
			k->inset(4);
			k->y = kh * i;
			addChild(k);
			keys.push_back(k);
		}
	}

	vector<Key *> keys;

	map<int, glm::vec2> touches;
	function<void(int)> notePressed;
	function<void(int)> noteReleased;
	void updateDeprecated() override {
		for (int k = 0; k < keys.size(); k++) {
			keys[k]->shouldBeOn = false;
		}
		for (int i = 0; i < touches.size(); i++) {
			for (int k = 0; k < keys.size(); k++) {
				if (keys[k]->inside(touches[i])) {
					keys[k]->shouldBeOn = true;
				}
			}
		}
		for (int k = 0; k < keys.size(); k++) {
			if (keys[k]->shouldBeOn && !keys[k]->isOn) {
				keys[k]->isOn = true;
				if (notePressed) notePressed(k);
			} else if (!keys[k]->shouldBeOn && keys[k]->isOn) {
				keys[k]->isOn = false;
				if (noteReleased) noteReleased(k);
			}
		}
	}

	bool touchDown(float x, float y, int id) {
		if (inside(x, y)) {
			touches[id] = {x, y};
			return true;
		}
		return false;
	}

	void touchMoved(float x, float y, int id) { touches[id] = {x, y}; }

	void touchUp(float x, float y, int id) {
		if (touches.find(id) != touches.end()) {
			touches.erase(id);
		}
	}

private:
	int numKeys = 24;
};
