//
//  MZOpenGLView.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

//#include <string>
#include <mzgl/App.h>

class TextConsole {
public:
	void show();
	void setText(std::string text);
	void *theTextView_ = nullptr;

	void setBgColor(glm::vec3 c);
};
