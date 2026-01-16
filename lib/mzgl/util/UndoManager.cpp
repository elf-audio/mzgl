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
	LambdaUndoable(std::function<void()> &&redoIt, std::function<void()> &&undoIt, size_t memSize = 0)
		: redoIt(std::move(redoIt))
		, undoIt(std::move(undoIt))
		, memorySize(memSize) {}

	std::function<void()> redoIt;
	std::function<void()> undoIt;
	size_t memorySize;

	void redo() override { redoIt(); }
	void undo() override { undoIt(); }
	size_t getMemorySize() const override { return memorySize; }
};

UndoManager::UndoManager(size_t memLimit)
	: memoryLimit(memLimit)
	, currentMemoryUsage(0) {
	clear();
}

void UndoManager::clear() {
	undoStack.clear();
	currentMemoryUsage = 0;
	undoPos			   = undoStack.end();
	notify([](UndoManagerListener *l) { l->undoRedoStateChanged(); });
}

std::size_t UndoManager::size() const {
	return undoStack.size();
}

void UndoManager::commit(std::function<void()> &&redo, std::function<void()> &&undo, size_t memorySize) {
	UndoableRef item = std::make_shared<LambdaUndoable>(std::move(redo), std::move(undo), memorySize);
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
	notify([](UndoManagerListener *l) { l->undoRedoStateChanged(); });
	return true;
}

bool UndoManager::redo() {
	if (!canRedo()) return false;
	(*undoPos)->redo();
	undoPos++;
	//	Log::d() << "REDO: Stack pos: " << std::distance(undoStack.begin(), undoPos) << " size: " << undoStack.size();
	notify([](UndoManagerListener *l) { l->undoRedoStateChanged(); });
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

	void addStep(UndoableRef item) {
		items.push_back(item);
		totalMemorySize += item->getMemorySize();
	}

	[[nodiscard]] bool empty() const { return items.empty(); }

	size_t getMemorySize() const override { return totalMemorySize; }

private:
	std::deque<UndoableRef> items;
	size_t totalMemorySize = 0;
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

void UndoManager::beginGesture() {
	if (gestureGroup != nullptr) {
		return;
	}
	gestureGroup = std::make_shared<GroupUndoable>();
}


void UndoManager::endGesture() {
	if (gestureGroup == nullptr) {
		return;
	}
	auto group = gestureGroup;
	gestureGroup = nullptr;
	if (!dynamic_pointer_cast<GroupUndoable>(group)->empty()) {
		commit(group);
	} else {
		// Notify even if gesture was empty, in case state needs updating
		notify([](UndoManagerListener *l) { l->undoRedoStateChanged(); });
	}
}

/**
 * To perform an undoable action - wrap it in an undoable
 * and commit it here - you don't actually perform the action
 * - instead commit() calls redo() on your Undoable.
 */
void UndoManager::commit(UndoableRef item) {
	if (gestureGroup != nullptr) {
		item->redo();
		dynamic_pointer_cast<GroupUndoable>(gestureGroup)->addStep(item);
		return;
	}

	if (undoGroup != nullptr) {
		auto ug = std::dynamic_pointer_cast<GroupUndoable>(undoGroup);
		ug->addStep(item);
		return;
	}

	// Branching: if we can redo, we're in the middle of history
	// Remove all entries after current position and subtract their memory
	if (canRedo()) {
		auto it = undoPos;
		while (it != undoStack.end()) {
			subtractMemoryUsage((*it)->getMemorySize());
			++it;
		}
		undoStack.erase(undoPos, undoStack.end());
	}

	// Add new item
	undoStack.push_back(item);
	addMemoryUsage(item->getMemorySize());

	// Evict old entries if needed
	evictOldEntriesIfNeeded();

	// Execute the action
	item->redo();

	// Update position
	undoPos = undoStack.end();
	notify([](UndoManagerListener *l) { l->undoRedoStateChanged(); });
}

// Memory tracking helper methods

void UndoManager::addMemoryUsage(size_t bytes) {
	currentMemoryUsage += bytes;
}

void UndoManager::subtractMemoryUsage(size_t bytes) {
	if (currentMemoryUsage >= bytes) {
		currentMemoryUsage -= bytes;
	} else {
		// Safety: shouldn't happen, but prevent underflow
		currentMemoryUsage = 0;
	}
}

void UndoManager::evictOldEntriesIfNeeded() {
	// Evict from front while either limit is exceeded
	while (!undoStack.empty()
		   && (undoStack.size() > MAX_UNDO_LEVELS || currentMemoryUsage > memoryLimit)) {
		// Calculate position of undoPos in the stack
		size_t posIndex = std::distance(undoStack.begin(), undoPos);

		// Can't remove if it would invalidate undoPos
		// (i.e., we're at the beginning and haven't undone anything)
		if (posIndex == 0) {
			// We're at the oldest entry and can't undo further
			// This is an edge case where current state > limit
			break;
		}

		// Remove oldest entry
		size_t removedSize = undoStack.front()->getMemorySize();
		undoStack.pop_front();
		subtractMemoryUsage(removedSize);

		// Adjust undoPos iterator (it shifts back by one)
		undoPos = undoStack.begin() + (posIndex - 1);
	}
}

// Public memory tracking API

size_t UndoManager::getMemoryUsage() const {
	return currentMemoryUsage;
}

size_t UndoManager::getMemoryLimit() const {
	return memoryLimit;
}

void UndoManager::setMemoryLimit(size_t limit) {
	memoryLimit = limit;
	evictOldEntriesIfNeeded();
	notify([](UndoManagerListener *l) { l->undoRedoStateChanged(); });
}
