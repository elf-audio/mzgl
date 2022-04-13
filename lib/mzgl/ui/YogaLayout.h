//
//  YogaLayout.h
//  mzgl
//
//  Created by Marek Bereza on 26/06/2020.
//  Copyright © 2020 Marek Bereza. All rights reserved.
//

#pragma once
#include "Yoga.h"

#include <memory>
#include <vector>
#include "Layer.h"
#include "util.h"

class LayoutNode;
typedef std::shared_ptr<LayoutNode> LayoutNodeRef;


struct LayoutAttribute {

	std::string name;
	std::vector<std::string> values;
	
	
	LayoutAttribute(std::string name, std::string val) : name(name) {
		values = split(val, " ");
	}
	LayoutAttribute(std::string name, const char *val) : name(name) {
		values = split(val, " ");
	}
	template<typename T>
	LayoutAttribute(std::string name, T value) : name(name) {
		values.push_back(std::to_string(value));
	}
	template<typename T, typename U>
	LayoutAttribute(std::string name, T v, U h) : name(name) {
		values.push_back(std::to_string(v));
		values.push_back(std::to_string(h));
	}
	template<typename T, typename U, typename V, typename W>
	LayoutAttribute(std::string name, T t, U r, V b, W l) : name(name) {
		values.push_back(std::to_string(t));
		values.push_back(std::to_string(r));
		values.push_back(std::to_string(b));
		values.push_back(std::to_string(l));
	}
	
	bool isPercent(int which = 0) const {
		return values[which].find("%")!=-1;
	}
	
	bool isAuto(int which = 0) const {
		return values[which]=="auto";
	}
	const std::string &value(int which = 0) const {
		return values[which];
	}
	float getValue(int which = 0) const {
		return stof(values[which]);
	}
	
	size_t numVals() const {
		return values.size();
	}
};



class LayoutBase {
public:

	void addChild(LayoutNodeRef child);
	void addChildren(const std::vector<LayoutNodeRef> &children) {
		for(auto c : children) addChild(c);
	}
	void update(float xOffset = 0.f, float yOffset = 0.f);
	
	void print() {
		YGNodePrint(node, (YGPrintOptions)(YGPrintOptionsLayout|YGPrintOptionsStyle |YGPrintOptionsChildren));
	}
	YGNodeRef node;
	
protected:
	LayoutBase(Layer *layer) : layer(layer) {}
	std::vector<LayoutNodeRef> children;
	Layer *layer = nullptr;
};


class LayoutNode : public LayoutBase {
public:
	static LayoutNodeRef create(Layer *layer = nullptr) {
		return LayoutNodeRef(new LayoutNode(layer));
	}

protected:
	
	LayoutNode(Layer *layer) : LayoutBase(layer) {
		node = YGNodeNew();
	}
};


class LayoutNodeCreator {
public:
	Layer *layer;
	std::vector<LayoutAttribute> attrs;
	std::vector<LayoutNodeCreator> children;
	
	LayoutNodeCreator(Layer *l, const std::vector<LayoutAttribute> &attrs, const std::vector<LayoutNodeCreator> & children) :
	layer(l), attrs(attrs), children(children) {
		
	}
	LayoutNodeCreator(Layer *l = nullptr, const std::vector<LayoutAttribute> &attrs = {}) :
	layer(l), attrs(attrs){
		
	}
	// this recursively creates nodes somehow
	LayoutNodeRef createNode() const {
		auto l = LayoutNode::create(layer);
		setAttributes(l, attrs);
		for(auto &c : children) {
			l->addChild(c.createNode());
		}
		
		return l;
	}
	
	
	static void setAttributes(LayoutNodeRef l, const std::vector<LayoutAttribute> &attrs) {
		setAttributes(l.get(), attrs);
	}
	static void setAttributes(LayoutBase *l, const std::vector<LayoutAttribute> &attrs);
	
};


class LayoutRoot : public LayoutBase {
public:
	LayoutRoot(Layer *root) : LayoutBase(root) {
		config = YGConfigNew();
		node = YGNodeNewWithConfig(config);
		YGNodeStyleSetWidth(node, root->width);
		YGNodeStyleSetHeight(node, root->height);
	}
	
	LayoutRoot(Layer *root, const std::vector<LayoutAttribute> &attrs, const std::vector<LayoutNodeCreator> & children) : LayoutRoot(root) {
		LayoutNodeCreator::setAttributes(this, attrs);
		for(auto &c : children) {
			addChild(c.createNode());
		}
		
	}
	
	virtual ~LayoutRoot() {
		YGNodeFreeRecursive(node);
		YGConfigFree(config);
	}
	void doLayout() {
		YGNodeCalculateLayout(node, YGUndefined, YGUndefined, YGDirectionLTR);
		for(auto c : children) {
			c->update(0, 0);
		}
	}
	
private:
	YGConfigRef config;
};
