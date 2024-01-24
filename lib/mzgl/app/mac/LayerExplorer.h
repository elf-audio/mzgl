//
//  MZOpenGLView.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

//#include <string>
#include <mzgl/App.h>

class Layer;
class LayerExplorer {
public:
	void setup(Layer *root);
	void show();
	void hide();
	bool showing() { return isShowing; }
	void setText(std::string text);
	void setBgColor(glm::vec3 c);

private:
	Layer *rootLayer = nullptr;
	void *browser_	 = nullptr;
	bool isShowing	 = false;
};
