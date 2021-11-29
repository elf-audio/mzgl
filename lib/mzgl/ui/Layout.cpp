//
//  Layout.cpp
//  TapeSampler
//
//  Created by Marek Bereza on 16/11/2017.
//
//

#include "Layout.h"
#include <ostream>

using namespace std;

const void Layout::print(ostream &o, int depth) const {
	for(int i = 0; i < depth; i++) o << "\t";
	
	if(layer!=NULL) {
		o << "Layer('" << layer->name << "') " << endl;
	} else {
		string dir = "[h]";
		if(direction==VERTICAL) {
			dir = "[v]";
		}
		string dims = "";
		o << "Layout " << dir << " " << *this << endl;
		if(children.size()>0) {
			for(auto &c : children) {
				c.print(o, depth+1);
			}
		}
	}
}




void Layout::layout(const Rectf &r, float padding) {
	padding /= 2;
	auto rr = r;
	rr.x += padding / 2;
	rr.y += padding / 2;
	rr.width -= padding;
	rr.height -= padding;
	
	layoutInternal(rr, padding);
	
}



void Layout::layoutInternal(const Rectf &r, float padding) {
	set(r);
	
	if(layer!=NULL) {
		if(modeHorizontal==Mode::FIT_TO_PARENT) {
			Rectf rr = r;
			
			rr.x += padding;
			rr.y += padding;
			rr.width -= padding * 2;
			rr.height -= padding * 2;
			layer->set(rr);
		} else if(modeHorizontal==Mode::CENTRED) {
			layer->setFromCentre(r.centre(), layer->width, layer->height);
		}
		assert(children.size()==0);
	} else {
		layoutChildren(padding);
	}
}


void Layout::layoutChildren(float padding) {
	// my size is correct, how do I divide it up between my children
	float remainingSize = width;
	
	if(isVertical()) {
		remainingSize = height;
	}
	
	vector<Layout*> dimensionless;
	for(auto &l : children) {
		if(l.dimension==-1) {
			dimensionless.push_back(&l);
		} else {
			if(isVertical()) {
				l.width = width;
				if(l.dimType==DimensionType::percent) {
					l.height = l.dimension * 0.01f * height;
				} else {
					l.height = l.dimension;
				}
				remainingSize -= l.height;
			} else {
				l.height = height;
				if(l.dimType==DimensionType::percent) {
					l.width = l.dimension * 0.01f * width;
				} else {
					l.width = l.dimension;
				}
				remainingSize -= l.width;
			}
		}
	}
	
	// split the remaining space between the dimensionless ones
	float remainingDimension = remainingSize / (float) dimensionless.size();
	for(auto l : dimensionless) {
		if(isVertical()) {
			l->width = width;
			l->height = remainingDimension;
		} else {
			l->height = height;
			l->width = remainingDimension;
		}
	}
	
	// now reposition, we've only set the dimensions of each
	// and call layout on our children
	
	for(int i = 0; i < children.size(); i++) {
		if(i==0) {
			children[i].x = x;
			children[i].y = y;
		} else {
			if(isVertical()) {
				children[i].x = children[i-1].x;
				children[i].y = children[i-1].bottom();
			} else {
				children[i].x = children[i-1].right();
				children[i].y = children[i-1].y;
			}
		}
		// lay it out with its own rect
		children[i].layoutInternal(children[i], padding);
	}
}


std::ostream& operator<<(std::ostream& os, const Layout& layout) {
	layout.print(os);
	return os;
}
vector<Layer*> Layout::getAllLayers() {
	vector<Layer*> layers;
	getAllLayersRecursive(layers);
	return layers;
}

void Layout::getAllLayersRecursive(vector<Layer*> &layers) {
	if(layer!=nullptr) {
		
		layers.push_back(layer);
	}
	for(auto &c : children) {
		c.getAllLayersRecursive(layers);
	}
}
