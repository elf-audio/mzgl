//
//  UndoManager.cpp
//  koala
//
//  Created by Marek Bereza on 10/11/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#include "UndoManager.h"

class LambdaUndoable : public Undoable {
public:
	
	LambdaUndoable(std::function<void()> &&redoIt, std::function<void()> &&undoIt) :
	redoIt(std::move(redoIt)), undoIt(std::move(undoIt)) {}
	
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
/**
 * To perform an undoable action - wrap it in an undoable
 * and commit it here - you don't actually perform the action
 * - instead commit() calls redo() on your Undoable.
 */
void UndoManager::commit(UndoableRef item) {
	if(canRedo()) {
		undoStack.erase(undoPos, undoStack.end());
	}
	undoStack.push_back(item);
	while(undoStack.size()>MAX_UNDO_LEVELS) {
		undoStack.pop_front();
	}
	item->redo();
	undoPos = undoStack.end();
}
	
std::size_t UndoManager::size() const { return undoStack.size(); }

void UndoManager::commit(std::function<void()> &&redo, std::function<void()> &&undo) {
	UndoableRef item = std::make_shared<LambdaUndoable>(std::move(redo), std::move(undo));
	commit(item);
}

bool UndoManager::canUndo() const {
	return !undoStack.empty() && undoPos!=undoStack.begin();
}

bool UndoManager::canRedo() const {
	return undoPos!=undoStack.end();
}

bool UndoManager::undo() {
	if(!canUndo()) return false;
	undoPos--;
	(*undoPos)->undo();
	return true;
}

bool UndoManager::redo() {
	if(!canRedo()) return false;
	(*undoPos)->redo();
	undoPos++;
	return true;
}