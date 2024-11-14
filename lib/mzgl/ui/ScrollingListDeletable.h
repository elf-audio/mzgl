//
//  ScrollingListDeletable.h
//  mzgl
//
//  Created by Marek Bereza on 16/06/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once

#include "ScrollingListItem.h"
#include "Haptics.h"

class Dialogs;
// swipe left to delete
class ScrollingListDeletableView : public ScrollingListItemView {
public:
	struct Settings {
		Settings(const Dialogs &dialogs)
			: dialogs(dialogs) {}
		vec4 deleteColor {1.f, 0.f, 0.f, 1.f}; //0xFE453A;
		vec4 actionColor	 = {0.f, 1.f, 0.f, 1.f}; //0x00FF00;
		vec4 buttonTextColor = {1.f, 1.f, 1.f, 1.f};
		// configure action here:
		bool hasActionButton			 = false;
		std::string actionName			 = "SHARE";
		int actionButtonWidth			 = 100;
		float deleteButtonWidth			 = 100;
		std::string warningDeleteMessage = "are you sure you want to delete this item?";
		Font *font						 = nullptr;
		const Dialogs &dialogs;
		bool momentary = true;

		enum class SelectionBehaviour {
			OnMouseDown,
			OnMouseUp,
		};

		SelectionBehaviour selectionBehaviour {SelectionBehaviour::OnMouseUp};
	};

	ScrollingListDeletableView(Graphics &g, Settings &settings, std::shared_ptr<ScrollingListItem> item)
		: ScrollingListItemView(g, item)
		, settings(settings) {}

	// you use this to offset your content
	float horizontalScroll = 0.f;
	bool canDelete		   = true;

	virtual void draw() override;
	virtual void draw(Drawer &d) override;

	std::function<void()> action = []() { printf("ACTION!\n"); };

	bool touchDown(float x, float y, int id) override;
	void touchMoved(float x, float y, int id) override;
	void touchUp(float x, float y, int id) override;

protected:
	Settings &settings;
	bool down = false;

private:
	int downCount = 0;
	vec2 start {0.f, 0.f};
	float horizontalScrollTarget = 0.f;
	bool horizontallyScrolling	 = false;
	float initialScrollTarget	 = 0.f;
	bool deleting				 = false;
	float decidePoint			 = 0;
	bool shouldDelete			 = false;
	float deleteDecidey			 = 0.f;
	vec2 totalMovement {0.f, 0.f};
	vec2 prevTouch {0.f, 0.f};

	Haptics haptics;
};

class ScrollingListDeletableStringView : public ScrollingListDeletableView {
public:
	ScrollingListDeletableStringView(Graphics &g, Settings &settings, std::shared_ptr<ScrollingListItem> item)
		: ScrollingListDeletableView(g, settings, item) {}

	void draw() override {
		ScrollingListDeletableView::draw();
		if (collapsing) return;
		g.setColor(1);
		g.drawTextVerticallyCentred(item->name,
									glm::vec2(horizontalScroll + x + 20 * g.pixelScale, y + height / 2));
	}
};
