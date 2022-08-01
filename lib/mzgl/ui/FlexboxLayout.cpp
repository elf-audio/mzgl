 //
//  YogaLayout.cpp
//  mzgl
//
//  Created by Marek Bereza on 26/06/2020.
//  Copyright © 2020 Marek Bereza. All rights reserved.
//

#include "FlexboxLayout.h"
#include "log.h"

using namespace std;
void Flexbox::LayoutBase::addChild(LayoutNodeRef child) {
	children.push_back(child);
	YGNodeInsertChild(node, child->node, children.size()-1);
}

void Flexbox::LayoutBase::update(float xOffset, float yOffset) {

	if(isnan(xOffset)) xOffset = 0.0;
	if(isnan(yOffset)) yOffset = 0.0;

	if(layer!=nullptr) {
		layer->x = YGNodeLayoutGetLeft(node) + xOffset;
		layer->y = YGNodeLayoutGetTop(node) + yOffset;
		layer->width = YGNodeLayoutGetWidth(node);
		layer->height = YGNodeLayoutGetHeight(node);
	} else {
		xOffset += YGNodeLayoutGetLeft(node);
		yOffset += YGNodeLayoutGetTop(node);
	}
	
	for(auto c : children) {
		c->update(xOffset, yOffset);
	}
}
void Flexbox::LayoutNodeCreator::setAttributes(LayoutBase *l, const vector<LayoutAttribute> &attrs) {

	for(auto &a : attrs) {
		// todo
		if(a.name=="width") {
			if(a.isAuto()) YGNodeStyleSetWidthAuto(l->node);
			else if(a.isPercent()) YGNodeStyleSetWidthPercent(l->node, a.getValue());
			else YGNodeStyleSetWidth(l->node, a.getValue());
			
		} else if(a.name=="height") {
			if(a.isAuto()) YGNodeStyleSetHeightAuto(l->node);
			else if(a.isPercent()) YGNodeStyleSetHeightPercent(l->node, a.getValue());
			else YGNodeStyleSetHeight(l->node, a.getValue());
			
		} else if(a.name=="aspect-ratio") {
			YGNodeStyleSetAspectRatio(l->node, a.getValue());
			
		} else if(a.name=="direction" || a.name=="flex-direction") {
			if(a.value()=="vertical") YGNodeStyleSetFlexDirection(l->node, YGFlexDirectionColumn);
			else if(a.value()=="horizontal") YGNodeStyleSetFlexDirection(l->node, YGFlexDirectionRow);
			
		} else if(a.name=="align" || a.name=="align-items") {
			if(a.value()=="center") YGNodeStyleSetAlignItems(l->node, YGAlignCenter);
			else if(a.value()=="start") YGNodeStyleSetAlignItems(l->node, YGAlignFlexStart);
			else if(a.value()=="end") YGNodeStyleSetAlignItems(l->node, YGAlignFlexEnd);
			
		} else if(a.name=="justify") {
			if(a.value()=="start") YGNodeStyleSetJustifyContent(l->node, YGJustifyFlexStart);
			else if(a.value()=="end") YGNodeStyleSetJustifyContent(l->node, YGJustifyFlexEnd);
			else if(a.value()=="center") YGNodeStyleSetJustifyContent(l->node, YGJustifyCenter);
			else if(a.value()=="space-between") YGNodeStyleSetJustifyContent(l->node, YGJustifySpaceBetween);
			else if(a.value()=="space-around") YGNodeStyleSetJustifyContent(l->node, YGJustifySpaceAround);
			else if(a.value()=="space-evenly") YGNodeStyleSetJustifyContent(l->node, YGJustifySpaceEvenly);
			
		} else if(a.name=="min-width") {
			if(a.isPercent()) YGNodeStyleSetMinWidthPercent(l->node, a.getValue());
			else YGNodeStyleSetMinWidth(l->node, a.getValue());
		} else if(a.name=="min-height") {
			if(a.isPercent()) YGNodeStyleSetMinHeightPercent(l->node, a.getValue());
			else YGNodeStyleSetMinHeight(l->node, a.getValue());
				
		} else if(a.name=="max-width") {
			if(a.isPercent()) YGNodeStyleSetMaxWidthPercent(l->node, a.getValue());
			else YGNodeStyleSetMaxWidth(l->node, a.getValue());
		} else if(a.name=="max-height") {
			if(a.isPercent()) YGNodeStyleSetMaxHeightPercent(l->node, a.getValue());
			else YGNodeStyleSetMaxHeight(l->node, a.getValue());
		} else if(a.name=="grow" || a.name=="flex-grow") {
			YGNodeStyleSetFlexGrow(l->node, a.getValue());
		} else if(a.name=="shrink" || a.name=="flex-shrink") {
			YGNodeStyleSetFlexShrink(l->node, a.getValue());
		} else if(a.name=="margin") {
			if(a.numVals()==1) {
				if(a.isPercent()) YGNodeStyleSetMarginPercent(l->node, YGEdgeAll, a.getValue());
				else YGNodeStyleSetMargin(l->node, YGEdgeAll, a.getValue());
			} else if(a.numVals()==2) {
				// vertical and horizontal
				if(a.isPercent(0)) {
					YGNodeStyleSetMarginPercent(l->node, YGEdgeTop, a.getValue(0));
					YGNodeStyleSetMarginPercent(l->node, YGEdgeBottom, a.getValue(0));
				} else {
					YGNodeStyleSetMargin(l->node, YGEdgeTop, a.getValue(0));
					YGNodeStyleSetMargin(l->node, YGEdgeBottom, a.getValue(0));
				}
				
				if(a.isPercent(1)) {
					YGNodeStyleSetMarginPercent(l->node, YGEdgeLeft, a.getValue(1));
					YGNodeStyleSetMarginPercent(l->node, YGEdgeRight, a.getValue(1));
				} else {
					YGNodeStyleSetMargin(l->node, YGEdgeLeft, a.getValue(1));
					YGNodeStyleSetMargin(l->node, YGEdgeRight, a.getValue(1));
				}
			} else if(a.numVals()==4) {
				if(a.isPercent(0)) YGNodeStyleSetMarginPercent(l->node, YGEdgeTop, a.getValue(0));
				else               YGNodeStyleSetMargin       (l->node, YGEdgeTop, a.getValue(0));
				
				if(a.isPercent(1)) YGNodeStyleSetMarginPercent(l->node, YGEdgeRight, a.getValue(1));
				else               YGNodeStyleSetMargin       (l->node, YGEdgeRight, a.getValue(1));
				
				if(a.isPercent(2)) YGNodeStyleSetMarginPercent(l->node, YGEdgeBottom, a.getValue(2));
				else               YGNodeStyleSetMargin       (l->node, YGEdgeBottom, a.getValue(2));
				
				if(a.isPercent(3)) YGNodeStyleSetMarginPercent(l->node, YGEdgeLeft, a.getValue(3));
				else               YGNodeStyleSetMargin       (l->node, YGEdgeLeft, a.getValue(3));
			}
		} else if(a.name=="margin-top") {
			if(a.isPercent()) YGNodeStyleSetMarginPercent(l->node, YGEdgeTop, a.getValue());
			else               YGNodeStyleSetMargin      (l->node, YGEdgeTop, a.getValue());
		} else if(a.name=="margin-right") {
			if(a.isPercent()) YGNodeStyleSetMarginPercent(l->node, YGEdgeRight, a.getValue());
			else               YGNodeStyleSetMargin      (l->node, YGEdgeRight, a.getValue());
		} else if(a.name=="margin-bottom") {
			if(a.isPercent()) YGNodeStyleSetMarginPercent(l->node, YGEdgeBottom, a.getValue());
			else              YGNodeStyleSetMargin       (l->node, YGEdgeBottom, a.getValue());
		} else if(a.name=="margin-left") {
			if(a.isPercent()) YGNodeStyleSetMarginPercent(l->node, YGEdgeLeft, a.getValue());
			else               YGNodeStyleSetMargin      (l->node, YGEdgeLeft, a.getValue());
		
		} else if(a.name=="padding") {
			if(a.numVals()==1) {
				if(a.isPercent()) YGNodeStyleSetPaddingPercent(l->node, YGEdgeAll, a.getValue());
				else YGNodeStyleSetPadding(l->node, YGEdgeAll, a.getValue());
			} else if(a.numVals()==2) {
				// vertical and horizontal
				if(a.isPercent(0)) {
					YGNodeStyleSetPaddingPercent(l->node, YGEdgeTop, a.getValue(0));
					YGNodeStyleSetPaddingPercent(l->node, YGEdgeBottom, a.getValue(0));
				} else {
					YGNodeStyleSetPadding(l->node, YGEdgeTop, a.getValue(0));
					YGNodeStyleSetPadding(l->node, YGEdgeBottom, a.getValue(0));
				}
				
				if(a.isPercent(1)) {
					YGNodeStyleSetPaddingPercent(l->node, YGEdgeLeft, a.getValue(1));
					YGNodeStyleSetPaddingPercent(l->node, YGEdgeRight, a.getValue(1));
				} else {
					YGNodeStyleSetPadding(l->node, YGEdgeLeft, a.getValue(1));
					YGNodeStyleSetPadding(l->node, YGEdgeRight, a.getValue(1));
				}
			} else if(a.numVals()==4) {
				if(a.isPercent(0)) YGNodeStyleSetPaddingPercent(l->node, YGEdgeTop, a.getValue(0));
				else               YGNodeStyleSetPadding       (l->node, YGEdgeTop, a.getValue(0));
				
				if(a.isPercent(1)) YGNodeStyleSetPaddingPercent(l->node, YGEdgeRight, a.getValue(1));
				else               YGNodeStyleSetPadding       (l->node, YGEdgeRight, a.getValue(1));
				
				if(a.isPercent(2)) YGNodeStyleSetPaddingPercent(l->node, YGEdgeBottom, a.getValue(2));
				else               YGNodeStyleSetPadding       (l->node, YGEdgeBottom, a.getValue(2));
				
				if(a.isPercent(3)) YGNodeStyleSetPaddingPercent(l->node, YGEdgeLeft, a.getValue(3));
				else               YGNodeStyleSetPadding       (l->node, YGEdgeLeft, a.getValue(3));
			}
		} else if(a.name=="padding-top") {
			if(a.isPercent()) YGNodeStyleSetPaddingPercent(l->node, YGEdgeTop, a.getValue());
			else               YGNodeStyleSetPadding      (l->node, YGEdgeTop, a.getValue());
		} else if(a.name=="padding-right") {
			if(a.isPercent()) YGNodeStyleSetPaddingPercent(l->node, YGEdgeRight, a.getValue());
			else               YGNodeStyleSetPadding      (l->node, YGEdgeRight, a.getValue());
		} else if(a.name=="padding-bottom") {
			if(a.isPercent()) YGNodeStyleSetPaddingPercent(l->node, YGEdgeBottom, a.getValue());
			else              YGNodeStyleSetPadding       (l->node, YGEdgeBottom, a.getValue());
		} else if(a.name=="padding-left") {
			if(a.isPercent()) YGNodeStyleSetPaddingPercent(l->node, YGEdgeLeft, a.getValue());
			else               YGNodeStyleSetPadding      (l->node, YGEdgeLeft, a.getValue());
		} else {
			Log::e() << "ERROR! Layout - No such attribute name! ("<<a.name<<")";
		}
	}
}
