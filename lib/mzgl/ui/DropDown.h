/**     ___           ___           ___                         ___           ___     
 *     /__/\         /  /\         /  /\         _____         /  /\         /__/|    
 *    |  |::\       /  /::\       /  /::|       /  /::\       /  /::\       |  |:|    
 *    |  |:|:\     /  /:/\:\     /  /:/:|      /  /:/\:\     /  /:/\:\      |  |:|    
 *  __|__|:|\:\   /  /:/~/::\   /  /:/|:|__   /  /:/~/::\   /  /:/  \:\   __|__|:|    
 * /__/::::| \:\ /__/:/ /:/\:\ /__/:/ |:| /\ /__/:/ /:/\:| /__/:/ \__\:\ /__/::::\____
 * \  \:\~~\__\/ \  \:\/:/__\/ \__\/  |:|/:/ \  \:\/:/~/:/ \  \:\ /  /:/    ~\~~\::::/
 *  \  \:\        \  \::/          |  |:/:/   \  \::/ /:/   \  \:\  /:/      |~~|:|~~ 
 *   \  \:\        \  \:\          |  |::/     \  \:\/:/     \  \:\/:/       |  |:|   
 *    \  \:\        \  \:\         |  |:/       \  \::/       \  \::/        |  |:|   
 *     \__\/         \__\/         |__|/         \__\/         \__\/         |__|/   
 *
 *  Description: 
 *
 *	TapeSampler	
 *		 
 *  DropDown.h, created by Marek Bereza on 21/11/2017.
 *  
 */
#pragma once
#include "Layer.h"
#include <functional>
class DropDown: public Layer {
public:
    std::function<void(int)> onChange;
    int selectedIndex = 0;
    std::vector<std::string> options;
    
    
    DropDown(Graphics &g, std::vector<std::string> options) : Layer(g, "dropdown"), options(options) {
        interactive = true;
    }
	void draw() override;
	
	void toggle();
	bool touchDown(float x, float y, int id) override;
	
	void touchMoved(float x, float y, int id) override;
	
	void touchOver(float x, float y) override;
	
	void touchUp(float x, float y, int id) override;
	
private:
    
    bool collapsed = true;
    float originalHeight = 0;
    bool expandingClick = false;
    bool touchHasMoved = false;
    int hoveredIndex = -1;
    glm::vec2 touchStart;
};
