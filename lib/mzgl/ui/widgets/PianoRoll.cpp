
#include "PianoRoll.h"
#include "Graphics.h"
#include "util.h"
#include "maths.h"
PianoRoll::PianoRoll(Graphics &g) : Layer(g, "piano roll") {
    bank = 0;
    mousedNote = NULL;
    playhead = 0;
    quantizePPQN = 4096/4;
    extending = false;
	interactive = true;
	
}



void PianoRoll::drawGrid(Rectf &r, int majorX, int minorX, int ys) {
    g.noFill();
    
    for(int i = 0; i < majorX; i++) {
        
        float xx = r.x + r.width * (i/(float)majorX);
        g.setColor(0.5);
        g.drawLine(xx, r.y, xx, r.y + r.height);
        g.setColor(0.27);
        for(int j = 1; j < minorX; j++) {
            float xxx = xx + (r.width/(float)majorX) * (j/(float)minorX);
            g.drawLine(xxx, r.y, xxx, r.y + r.height);
        }
    }
    g.setColor(0.4);
    for(int i = 1; i < ys; i++) {
        float yy = r.y + r.height * (i/(float)ys);
        g.drawLine(r.x, yy, r.x + r.width, yy);
    }
    float playheadX = this->width * playhead / (float) pattern->getPatternLengthInSamples();
	g.setColor(1, 0, 0);
    g.drawLine(playheadX, this->y, playheadX, this->bottom());
	
}



void PianoRoll::draw() {

	printf("%f", offset);
    float beatWidth = this->width/(float)(pattern->numBars * pattern->beatsPerBar);
    
    float noteHeight = this->height / (float) BANK_SIZE;
	
	glEnable(GL_SCISSOR_TEST);
	glScissor(x*2, (g.height-(y+height)*2), width*2, height*2);
	g.pushMatrix();
	g.translate(this->x + offset, 0, 0);
	g.scale(zoom, 1, 1);
	Rectf r = *this;
	r.x = 0;
	drawGrid(r, pattern->numBars * pattern->beatsPerBar, 4, BANK_SIZE);
    pattern->lock();
    auto it = pattern->begin();
    for(; it != pattern->end(); it++) {
        TimeOffset t = (*it).first;
        Note &n = (*it).second;
        int pitch = n.pitch;
        pitch -= START_NOTE + bank * BANK_SIZE;
        if(pitch < 0 || pitch >= BANK_SIZE) continue;
        
        float xOffset = /*this->x + */beatFraction(t) * beatWidth;
        float duration = beatFraction(n.length) * beatWidth;
        drawNote(xOffset, (this->bottom()) - (1 + pitch) * noteHeight, duration, noteHeight);
    }
    pattern->unlock();
	g.popMatrix();
	
	glDisable(GL_SCISSOR_TEST);
	
	g.noFill();
	g.setColor(1);
	
	g.drawRect(*this);
	
	g.fill();
}

void PianoRoll::drawNoteRect(float x, float y, float w, float h) {
	x++;
	y++;
	w -= 2;
	h -= 2;
	
	g.fill();
	g.setColor(200/255.f, 30/255.f, 0);
	g.drawRect(x, y, w, h);
	g.noFill();
	g.setColor(0);
	g.drawRect(x, y, w, h);
	g.setColor(1);
	
}

void PianoRoll::drawNote(float x, float y, float w, float h) {
	
	
	if((x - this->x) + w > width) { // wrap note if it goes over the edge
		float ww = width - (x - this->x);
		drawNoteRect(x, y, ww, h);
		ww = w - ww;
		drawNoteRect(this->x, y, ww, h);
	} else {
		drawNoteRect(x, y, w, h);
	}
	
	
	
	
}



bool PianoRoll::touchDown(float x, float y, int button) {
    int grabEndDistance = 10;
    if(!this->inside(x, y)) return false;
	

	touches[button] = Touch(x, y, touchXToPatternCoords(x));
	if(touches.size()>1) {
		// zooming!!!
		zooming = true;
		
	} else {
		// convert x to pianoroll coords
		x = touchXToPatternCoords(x);

		
		mouseDownCoord = glm::vec2(x, y);
		// check for note under mouse
		TimeOffset t = timeForCoord(x);
		
		int n = pitchForCoord(y);
		pattern->lock();
		const PatternNote &selectedNote = pattern->find(n, t);
		if(selectedNote != pattern->end()) {
			TimeOffset endOfNote = (*selectedNote).first + (*selectedNote).second.length;
			float endCoord = coordForTime(endOfNote);
			
			mousedNote = (PatternNote *)&selectedNote;
			mousedNoteStart = (*selectedNote).first;
			if(x>endCoord-grabEndDistance) { // extending
				extending = true;
				printf("Extending\n");
			} else { // dragging
				extending = false;
				printf("Dragging\n");
			}
			
		} else {
			Note note(n, 110, pattern->ppqn/4);
			// try to insert note
			mousedNote = (PatternNote*)&pattern->insertNote(note, quantize(t));
			extending = true;
		}
		pattern->unlock();
		
	}
	return true;
}


void PianoRoll::touchMoved(float x, float y, int button) {
	
	
	
	
	touches[button].update(x, y);
	
	if(zooming) {
		if(touches.size()<2) {
			return;
		}
		if(mousedNote!=nullptr) {
			pattern->erase(*mousedNote);
			mousedNote = nullptr;
		}
		Touch a = touches.begin()->second;
		Touch b = (++touches.begin())->second;
		if(a.pos.x>b.pos.x) std::swap(a, b);
	
		
		// now need to set zoom given its offset
		
		
		// easy to work out zoom, it's just the screen distance / pattern distance
		float pattDist = b.patternX - a.patternX;
		float screenDist = b.pos.x - a.pos.x;
		
		zoom = screenDist / pattDist;
		
		float xx = (a.pos.x + b.pos.x)/2.f;
		float xxx = (a.patternX + b.patternX) / 2.f;
		offset = xx - this->x - zoom * (xxx - this->x);
		
		
		
		if(offset>0) offset = 0;
		if(zoom * width + offset < width) {
			zoom = (width - offset) / width;
		}
	} else {
		// convert x to pianoroll coords
		x = touchXToPatternCoords(x);
		
		
		pattern->lock();
		// check to see whether we should extend the note
		// - the mouse should have wiggled at least a little
		// bit from its initial starting point to start extending
		// the note
		if(mousedNote!=NULL && abs(x - mouseDownCoord.x) > 5) {
			if(extending) {
				
				// must ensure that you can't have negative length
				int duration = timeForCoord((x + this->x) - coordForTime((*mousedNote)->first));
				if(duration<0) {
					duration += pattern->getPatternLength();
				}
				(*mousedNote)->second.length = fmaxf(MIN_NOTE_LENGTH, duration+2);
				
			} else {
				int mouseDelta = x - mouseDownCoord.x;
				
				// we have to erase the note and reinsert it because
				// the time value is the key in the multimap
				Note n = (*mousedNote)->second;
				pattern->erase((const PatternNote &) *mousedNote);
				TimeOffset newNoteTime = timeForCoord(coordForTime(mousedNoteStart) + mouseDelta);
				mousedNote = (PatternNote*)&pattern->insertNote(n, newNoteTime);
			}
		}
		pattern->unlock();
	}
}

void PianoRoll::touchUp(float x, float y, int button) {
	
	if(touches.find(button) != touches.end()) {
		touches.erase(button);
		if(touches.size()==0 && zooming) {
			zooming = false;
			return;
		}
	}
	
	// convert x to pianoroll coords
	x = touchXToPatternCoords(x);
	
    pattern->lock();
    if(mousedNote!=NULL) {
        if(!extending && abs(x - mouseDownCoord.x) < 5) {
            pattern->erase((const PatternNote &)*mousedNote);
        }
        mousedNote = NULL;
    }
    pattern->unlock();
	
}

float PianoRoll::touchXToPatternCoords(float x) {
	float left = this->x + offset;
	float right = this->x + offset + zoom * width;
	return mapf(x, left, right, this->x, this->x + this->width);
}

void PianoRoll::mouseScrolled(float x, float y, float scrollX, float scrollY ) {
	
	
	
	float mousePatternX = touchXToPatternCoords(x);
	zoom = clampf((float) zoom * pow(2.f, -scrollY * 0.1), 1.f, 100.f);
	
	offset = x - this->x - zoom * (-scrollX + mousePatternX - this->x);
	if(offset>0) offset = 0;
	if(zoom * width + offset < width) {
		zoom = (width - offset) / width;
	}
}


