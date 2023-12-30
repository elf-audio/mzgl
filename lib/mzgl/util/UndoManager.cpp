//
//  UndoManager.cpp
//  koala
//
//  Created by Marek Bereza on 10/11/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#include "UndoManager.h"
#include "log.h"
#include "mzAssert.h"
#include <algorithm>

class LambdaUndoable : public Undoable {
public:
	LambdaUndoable(std::function<void()> &&redoIt, std::function<void()> &&undoIt)
		: redoIt(std::move(redoIt))
		, undoIt(std::move(undoIt)) {}

	std::function<void()> redoIt;
	std::function<void()> undoIt;
	void redo() override { redoIt(); }
	void undo() override { undoIt(); }
};

UndoManager::UndoManager() {
	clear();
}

void UndoManager::clear() {
	undoStack.clear();
	undoPos = undoStack.end();
}

std::size_t UndoManager::size() const {
	return undoStack.size();
}

void UndoManager::commit(std::function<void()> &&redo, std::function<void()> &&undo) {
	UndoableRef item = std::make_shared<LambdaUndoable>(std::move(redo), std::move(undo));
	commit(item);
}

bool UndoManager::canUndo() const {
	return !undoStack.empty() && undoPos != undoStack.begin();
}

bool UndoManager::canRedo() const {
	return undoPos != undoStack.end();
}

bool UndoManager::undo() {
	if (!canUndo()) return false;
	undoPos--;
	(*undoPos)->undo();
	//	Log::d() << "UNDO: Stack pos: " << std::distance(undoStack.begin(), undoPos) << " size: " << undoStack.size();
	return true;
}

bool UndoManager::redo() {
	if (!canRedo()) return false;
	(*undoPos)->redo();
	undoPos++;
	//	Log::d() << "REDO: Stack pos: " << std::distance(undoStack.begin(), undoPos) << " size: " << undoStack.size();
	return true;
}

class GroupUndoable : public Undoable {
public:
	void redo() override {
		for (auto &item: items)
			item->redo();
	}

	void undo() override {
		std::for_each(items.rbegin(), items.rend(), [](UndoableRef i) { i->undo(); });
	}

	void addStep(UndoableRef item) { items.push_back(item); }

private:
	std::deque<UndoableRef> items;
};

void UndoManager::beginGroup() {
	mzAssert(undoGroup == nullptr);
	undoGroup = std::make_shared<GroupUndoable>();
}
void UndoManager::endGroup() {
	auto group = undoGroup;
	undoGroup  = nullptr;
	commit(group);
}

/**
 * To perform an undoable action - wrap it in an undoable
 * and commit it here - you don't actually perform the action
 * - instead commit() calls redo() on your Undoable.
 */
void UndoManager::commit(UndoableRef item) {
	if (undoGroup != nullptr) {
		auto ug = std::dynamic_pointer_cast<GroupUndoable>(undoGroup);
		ug->addStep(item);
		return;
	}
	if (canRedo()) {
		undoStack.erase(undoPos, undoStack.end());
	}
	undoStack.push_back(item);
	while (undoStack.size() > MAX_UNDO_LEVELS) {
		undoStack.pop_front();
	}
	//	Log::d() << "COMMIT: Stack size: " << undoStack.size();

	item->redo();
	undoPos = undoStack.end();
}
