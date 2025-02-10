//
//  ChromaticKeyboard.h
//  mzgl
//
//  Created by Marek Bereza on 13/07/2020.
//  Copyright © 2020 Marek Bereza. All rights reserved.
//

#pragma once
#include "Layer.h"
#include "Drawer.h"
#include <set>

class ChromaticKeyboard : public Layer {
public:
	ChromaticKeyboard(Graphics &g)
		: Layer(g) {
		interactive = true;
	}
	int numWhiteNotes = 10;
	//	int numNotes			  = 10;
	float blackNoteHeight = 0.67;
	bool drawShadow		  = true;

	vec4 whiteKeyColor {1.f, 1.f, 1.f, 1.f};
	vec4 blackKeyColor {0.4f, 0.4f, 0.4f, 1.f};
	vec4 downKeyColor {1.f, 0.5f, 0.5f, 1.f};
	vec4 bgColor {0.f, 0.f, 0.f, 0.f};
	void draw() override {
		Drawer d;
		if (bgColor.a > 0) {
			d.setColor(bgColor);
			d.drawRect(*this);
		}
		std::set<int> downNotes;

		for (auto &t: touches) {
			downNotes.insert(t.second);
		}

		// white keys
		float radius = 8;
		//		int numWhiteNotes = octaves * 7;
		float ww = width / (float) numWhiteNotes;
		for (int i = 0; i < numWhiteNotes; i++) {
			Rectf r(x + ww * i, y, ww, height);
			r.inset(2);
			int noteNum = whiteNoteIndexToChromatic[i % 7] + (i / 7) * 12;

			if (downNotes.find(noteNum) != downNotes.end()) {
				d.setColor(downKeyColor);
			} else {
				d.setColor(whiteKeyColor);
			}
			d.drawRoundedRect(r, radius, false, false, true, true);
		}

		int octaves = static_cast<int>(std::ceil(numWhiteNotes / 7.f));

		for (int i = 0; i < octaves; i++) {
			// C#
			int octaveOffset = (i * 7 * ww);

			for (int j = 0; j < sharpOffsets.size(); j++) {
				int noteNum = i * 12 + sharpIndexToChromatic[j];
				if (downNotes.find(noteNum) != downNotes.end()) {
					d.setColor(downKeyColor);
				} else {
					d.setColor(blackKeyColor);
				}
				Rectf r(x + octaveOffset + ww * sharpOffsets[j], y + 2, ww * 0.5f, height * blackNoteHeight);
				if (r.x > right()) {
					break;
				}
				d.drawRoundedRect(r, radius, false, false, true, true);
			}
		}
		if (drawShadow) {
			Rectf gradient = *this;
			gradient.height *= 0.1f;
			vec4 a {0.f, 0.f, 0.f, 0.2f};
			vec4 b {0.f, 0.f, 0.f, 0.f};

			d.drawTriangle(gradient.tl(), gradient.tr(), gradient.bl(), a, a, b);
			d.drawTriangle(gradient.tr(), gradient.br(), gradient.bl(), a, b, b);
		}

		ScopedAlphaBlend s(g, drawShadow);
		VboRef vbo = Vbo::create();
		d.commit(vbo);
		g.setColor(1);
		vbo->draw(g);
	}

	bool touchDown(float x, float y, int id) override {
		touches[id] = touchToNote(x, y);
		noteOn(touches[id]);
		return true;
	}

	void touchMoved(float x, float y, int id) override {
		int noteOnNow = touchToNote(x, y);
		if (touches.find(id) != touches.end()) {
			// touch is already down on this keyboard
			if (touches[id] != noteOnNow) {
				noteOff(touches[id]);
				touches[id] = noteOnNow;
				noteOn(noteOnNow);
			}

		} else {
			// note dragged onto the keyboard
			touches[id] = noteOnNow;
			noteOn(noteOnNow);
		}
	}

	void touchUp(float x, float y, int id) override {
		if (touches.find(id) != touches.end()) {
			noteOff(touches[id]);
			touches.erase(id);
		}
	}

	std::function<void(int)> noteOff = [](int note) { Log::d() << "note off"; };
	std::function<void(int)> noteOn	 = [](int note) { Log::d() << "note on"; };

private:
	int touchToNote(float x, float y) {
		x -= this->x;
		y -= this->y;
		//		int numWhiteNotes = octaves * 7;
		float n	   = x / (width / (float) numWhiteNotes);
		int octave = n / 7;
		float note = fmod(n, 7);

		if (y < height * blackNoteHeight) {
			for (int i = 0; i < sharpOffsets.size(); i++) {
				if (note >= sharpOffsets[i] && note <= sharpOffsets[i] + 0.5f) {
					return sharpIndexToChromatic[i] + octave * 12;
				}
			}
		}
		return whiteNoteIndexToChromatic[((int) note) % 7] + octave * 12;
	}

	// touch id to keyboard note
	std::map<int, int> touches;
	std::vector<int> sharpIndexToChromatic	   = {1, 3, 6, 8, 10};
	std::vector<float> sharpOffsets			   = {0.7f, 1.8f, 3.7f, 4.75f, 5.8f};
	std::vector<int> whiteNoteIndexToChromatic = {0, 2, 4, 5, 7, 9, 11};
};
