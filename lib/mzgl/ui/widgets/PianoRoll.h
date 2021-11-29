//
//  PianoRoll.h
//  This is a GUI element that lets you draw notes like a piano roll
//
//  Created by Marek Bereza on 28/09/2015.
//
//
#pragma once

#include "Layer.h"

#include "NotePattern.h"
#define START_NOTE 0
#define BANK_SIZE 16
#define MIN_NOTE_LENGTH 160


class PianoRoll: public Layer {
public:
	
	
	PianoRoll(Graphics &g);
	
    // the note that the mouse is currently down on
    PatternNote *mousedNote;
    
    // this is the time of the moused note before it was moved (if it's being dragged)
    TimeOffset mousedNoteStart;
    
    // if mouse is down on a note, are we extending it?
    // i.e. - did the user mouse on the end of the note?
    // if false, you're dragging it
    bool extending;
	

	int quantizePPQN;
	int playhead;
	int bank;

    
    // this is the value of where the mouse
    // went down, so we can do hysteresis.
    // ideally there'd be one for each touch
	glm::vec2 mouseDownCoord;
	
	NotePattern *pattern;
	
    
	void setPattern(NotePattern *pattern) {
		this->pattern = pattern;
	}
	
    void drawGrid(Rectf &r, int majorX, int minorX, int ys);
    
	float beatFraction(TimeOffset t) {
		return t / (float) pattern->
		ppqn;
	}
	
    void draw() override;
    
    void drawNote(float x, float y, float w, float h);
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	/// NEED A CLEVER MAPPING BETWEEN COORDS ON SCREEN AND NOTES
	////////////////////////////////////////////////////////////////////////////////////////////////
	
	inline TimeOffset timeForCoord(float x) {
		float totalPpqn = pattern->beatsPerBar * pattern->numBars * pattern->ppqn;
		return (x - this->x) * totalPpqn/ this->width;
	}
	
    inline float coordForTime(TimeOffset t) {
		float totalPpqn = pattern->beatsPerBar * pattern->numBars * pattern->ppqn;
		return this->x + t*this->width/totalPpqn;
	}
    
	inline int pitchForCoord(float y) {
		int local = (BANK_SIZE - ((int)(BANK_SIZE * (y  - this->y)/this->height)) - 1);
		return START_NOTE + local + bank * BANK_SIZE;
	}
	
	inline TimeOffset quantize(TimeOffset t) {
		t /= quantizePPQN;
		t *= quantizePPQN;
		return t;
	}
    
	
    bool touchDown(float x, float y, int button) override;
    
    void touchMoved(float x, float y, int button) override;
    
    void touchUp(float x, float y, int button) override;
	class Touch {
	public:
		Touch(float x = 0, float y = 0, float patternX = 0) : pos(x, y), patternX(patternX) {}
		void update(float x, float y) {
			prev = pos;
			pos = glm::vec2(x, y);
		}
		float patternX;
		glm::vec2 pos;
		glm::vec2 prev;
	};
	
	::map<int, Touch> touches;
	void drawNoteRect(float x, float y, float w, float h);
	void mouseScrolled(float x, float y, float scrollX, float scrollY ) override;
	
	float offset = 0;
	float zoom = 1;
	float touchXToPatternCoords(float x);
	bool zooming = false;
};


