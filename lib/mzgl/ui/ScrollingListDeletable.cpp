#include "ScrollingListDeletable.h"
#include "maths.h"
#include "animation.h"
#include "Drawer.h"
#include "Dialogs.h"
#include <algorithm>

void ScrollingListDeletableView::draw() {
	if (collapsing) {
		return;
	}
	g.setColor(settings.buttonTextColor);
	float notDeletePos =
		x + width
		+ std::max(-settings.deleteButtonWidth / 2.f, horizontalScroll + settings.deleteButtonWidth / 2.f);
	float deletePos = x + width + std::max(horizontalScroll, -width) + settings.deleteButtonWidth / 2.f;

	float pos = notDeletePos;
	if (deleting) {
		deleteDecidey = std::clamp(deleteDecidey + (swipedFarEnoughToDelete ? 0.05f : -0.05f), 0.f, 1.f);
		pos			  = mapf(easeInOutCubic(deleteDecidey), 0, 1, notDeletePos, deletePos);
	}

	if (horizontalScroll < 0) {
		if (settings.font == nullptr) {
			g.drawTextCentred("DELETE", vec2(pos, y + height / 2));
		} else {
			settings.font->drawCentred(g, "DELETE", vec2(pos, y + height / 2));
		}
	}

	if (settings.hasActionButton) {
		auto p = vec2(horizontalScroll - settings.actionButtonWidth / 2, y + height / 2);

		if (settings.font == nullptr) {
			g.drawTextCentred(settings.actionName, p);
		} else {
			settings.font->drawCentred(g, settings.actionName, p);
		}
	}
}

void ScrollingListDeletableView::draw(Drawer &d) {
	//	if(down && length(totalMovement)<10) {
	//		selected = true;
	//	}

	if (settings.momentary && down && length(totalMovement) < 10 && downCount >= 3) {
		float amt = mapf(downCount, 3, 12, 0.25, 1, true);
		d.setColor(unselectedColor * (1.f - amt) + selectedColor * amt);
	} else {
		d.setColor((item->selected && !settings.momentary) ? selectedColor : unselectedColor);
	}

	if (down) {
		downCount++;
	}
	Rectf r	 = *this;
	r.y		 = (int) r.y + 2;
	r.height = (int) r.height - 1;
	d.drawRect(r);

	// this resets the button if we've lost focus (e.g. dragging has started again)
	if (horizontalScrollTarget != 0 && !hasFocus() && !g.focusedLayers.empty()) {
		horizontalScrollTarget = 0;
	}

	if (!settings.hasActionButton) {
		horizontalScrollTarget = std::min(0.f, horizontalScrollTarget);
	}
	// draw delete
	horizontalScroll = horizontalScroll * 0.8 + horizontalScrollTarget * 0.2;

	// delete bg
	if (horizontalScroll < 0) {
		d.fill();

		d.setColor(settings.deleteColor);

		d.drawRect(right(), y + 2, horizontalScroll, height - 1);
		// action bg
	} else if (settings.hasActionButton && horizontalScroll > 0) {
		d.fill();
		d.setColor(settings.actionColor);
		d.drawRect(x, y + 2, horizontalScroll, height - 1);
	}

	d.setColor(dividerColor);
	d.drawRect(x, (int) (y + height), width, 2);
}

bool ScrollingListDeletableView::touchDown(float x, float y, int id) {
	downCount = 0;
	start	  = {x, y};
	down	  = true;
	prevTouch = start;
	//	totalYMovement = 0.f;
	horizontallyScrolling = false;
	deleting			  = false;
	deleteDecidey		  = 0.f;
	totalMovement		  = {0.f, 0.f};
	initialScrollTarget	  = horizontalScrollTarget;
	if (settings.selectionBehaviour == Settings::SelectionBehaviour::OnMouseDown) {
		selectedSelf();
	}
	return true;
}

void ScrollingListDeletableView::touchMoved(float x, float y, int id) {
	vec2 currTouch = vec2(x, y);
	vec2 delta	   = currTouch - start;

	if (canDelete) {
		if (horizontallyScrolling) {
			// if the scroll is more than halfway left, do a full delete swipe
			if (deleting) {
				horizontalScrollTarget = delta.x;

				if (x < decidePoint - width * 0.02 && !swipedFarEnoughToDelete) {
					swipedFarEnoughToDelete = true;
					haptics.lightTap();
				} else if (x > decidePoint + width * 0.02 && swipedFarEnoughToDelete) {
					swipedFarEnoughToDelete = false;
					haptics.lightTap();
				}
			} else if (delta.x < -width * 0.5) {
				haptics.lightTap();
				horizontalScrollTarget	= -width;
				deleting				= true;
				swipedFarEnoughToDelete = true;
				start.x					= width + x;
				decidePoint				= x + width * 0.05;
				//		} else if(horizontalScrollTarget>deleteButtonWidth) {
				//			horizontalScrollTarget = deleteButtonWidth;
			} else {
				horizontalScrollTarget = delta.x + initialScrollTarget;
				if (settings.hasActionButton) {
					if (horizontalScrollTarget > settings.actionButtonWidth) {
						horizontalScrollTarget = settings.actionButtonWidth;
					}
				}
			}
		} else {
			if (abs(delta.x) > 20 && abs(delta.y) < 20) {
				horizontallyScrolling = true;

			} else if (abs(delta.y) > 20) {
				down = false;
				localToAbsoluteCoords(x, y);

				Layer *scrollingList = getParent()->getParent();
				transferFocus(scrollingList, id);
				scrollingList->absoluteToLocalCoords(x, y);
				scrollingList->touchDown(x, y, id);
			}
		}
	}
	totalMovement += (currTouch - prevTouch);
	prevTouch = currTouch;
}

void ScrollingListDeletableView::touchUp(float x, float y, int id) {
	if (canDelete && horizontallyScrolling) {
		if (deleting) {
			auto deleteOrNot = [this](bool shouldDelete) {
				if (shouldDelete) {
					horizontalScrollTarget = -width;
					collapsing			   = true;
					deleteSelf();
				} else {
					horizontalScrollTarget = 0.f;
				}
			};
			if (swipedFarEnoughToDelete && settings.checkIfShouldDelete) {
				settings.checkIfShouldDelete(deleteOrNot);
			} else {
				deleteOrNot(swipedFarEnoughToDelete);
			}

		} else {
			if (settings.hasActionButton && horizontalScrollTarget > settings.actionButtonWidth / 2) {
				horizontalScrollTarget = settings.actionButtonWidth;
			} else if (horizontalScrollTarget < -settings.deleteButtonWidth / 2) {
				horizontalScrollTarget = -settings.deleteButtonWidth;
			} else {
				horizontalScrollTarget = 0.f;
			}
		}
	} else {
		if (length(totalMovement) < 10) {
			if (horizontalScrollTarget == -settings.deleteButtonWidth) {
				if (x > width - settings.deleteButtonWidth) {
					//					printf("delete\n");
					settings.dialogs.confirm(
						"are you sure?",
						settings.warningDeleteMessage,
						[this]() {
							collapsing = true;
							deleteSelf();
						},
						[this]() { horizontalScrollTarget = 0.f; });

				} else {
					// do nothing
				}

			} else if (settings.hasActionButton && horizontalScrollTarget == settings.actionButtonWidth) {
				if (x < settings.actionButtonWidth) {
					action();
					horizontalScrollTarget = 0;
				}
			} else {
				if (settings.selectionBehaviour == Settings::SelectionBehaviour::OnMouseUp) {
					selectedSelf();
				}
			}
		}
	}
	down = false;
}
