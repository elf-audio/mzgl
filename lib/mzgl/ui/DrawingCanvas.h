//
//  DrawingCanvas.h
//
//
//

#pragma once

class DrawingCanvas: public Layer {
public:
	
	
	DrawingCanvas(): Layer("") {
		interactive = true;
		penColor.set(0,0,0);
	}
	
	glm::vec4 penColor;
	
	vector<vector<glm::vec2> > strokes;
	
	void clear() {
		strokes.clear();
	}

	bool drawing = false;

	
	virtual void draw() {
		Layer::draw();
		ofNoFill();
		ofSetColor(penColor);
		for(int i = 0; i < strokes.size(); i++) {
			ofBeginShape();
			for(int j = 0; j < strokes[i].size(); j++) {
				ofVertex(strokes[i][j].x, strokes[i][j].y);
			}
			ofEndShape();
		}
		
		ofFill();

	}
	
	virtual bool touchDown(float x, float y, int id) {
		if(inside(x, y)) {
			
			drawing = true;
			strokes.push_back(vector<glm::vec2>());
			strokes.back().push_back(glm::vec2(x,y));
			return true;
		}
		return false;
	}
	
	virtual void touchMoved(float x, float y, int id) {
		if(drawing) {
			strokes.back().push_back(glm::vec2(x,y));
		}
	}
	virtual void touchUp(float x, float y, int id) {
		drawing = false;
	}

};

