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
 *  Layout.h, created by Marek Bereza on 16/11/2017.
 *
 *  examples: 
 *
	// 1 fullscreen layer
	layout = {one};
 
	// 3 evenly spaced layers
	layout = {{one, two, three}};
 
	// vertical layout, one at 50%, next taking up extra space, other 100px
	layout = {"v", {{50.f, one}, two, {100, three}}};
 
 
	// same as previous but 2 horizontal layers taking up the middle section
	Layout layout2 = {{two, four}};
	layout = {"v", {{50.f, one}, layout2, {100, three}}};

 *     
 *  
 */
#pragma once

#include "Layer.h"

#define VERTICAL   "v"
#define HORIZONTAL "h"

class Layout : public Rectf {
public:
	enum class Mode { NONE, FIT_TO_PARENT, CENTRED, FIXED };
	Layout(std::vector<Layout> childs)
		: children(childs) {}

	Layout(std::string dir, std::vector<Layout> childs)
		: direction(dir)
		, children(childs) {
		assert(dir == "v" || dir == "h");
	}

	Layout(std::string dir, float dim, std::vector<Layout> childs)
		: direction(dir)
		, children(childs)
		, dimension(dim)
		, dimType(DimensionType::percent) {
		assert(dir == "v" || dir == "h");
	}

	Layout(std::string dir, int dim, std::vector<Layout> childs)
		: direction(dir)
		, children(childs)
		, dimension(dim)
		, dimType(DimensionType::pixels) {
		assert(dir == "v" || dir == "h");
	}

	Layout() {}

	// if you set only one mode, it applies to both horizontal and vertial
	Layout(Layer *lay, Mode modeHorizontal = Mode::FIT_TO_PARENT, Mode modeVertical = Mode::NONE)
		: layer(lay)
		, modeHorizontal(modeHorizontal)
		, modeVertical(modeVertical) {
		if (this->modeVertical == Mode::NONE) {
			this->modeVertical = this->modeHorizontal;
		}
	}

	Layout(float dim, Layer *lay)
		: layer(lay)
		, dimension(dim)
		, dimType(DimensionType::percent) {}

	Layout(int dim, Layer *lay)
		: layer(lay)
		, dimension(dim)
		, dimType(DimensionType::pixels) {}

	const void print(std::ostream &o, int depth = 0) const;

	void addChild(Layout layout) { children.push_back(layout); }
	void addChild(Layer *lay) { // bad name
		children.emplace_back(lay);
	}

	void layout(const Rectf &r, float padding = 0);
	std::vector<Layer *> getAllLayers();

private:
	void getAllLayersRecursive(std::vector<Layer *> &layers);

	void layoutInternal(const Rectf &r, float padding = 0);
	inline bool isVertical() { return direction == VERTICAL; }
	void layoutChildren(float padding);

	enum class DimensionType { pixels, percent };

	Mode modeHorizontal	  = Mode::FIT_TO_PARENT;
	Mode modeVertical	  = Mode::FIT_TO_PARENT;
	Layer *layer		  = nullptr;
	std::string direction = HORIZONTAL;
	DimensionType dimType = DimensionType::pixels;
	std::vector<Layout> children;
	float dimension = -1;
};

std::ostream &operator<<(std::ostream &os, const Layout &layout);
