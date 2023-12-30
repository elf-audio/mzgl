//
//  UndoManager.h
//  koala
//
//  Created by Marek Bereza on 02/11/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once

#include <functional>
#include <memory>
#include <deque>

/**
 * this is a thing that can be undone or redone,
 * the basic unit of the undo manager
 */
class Undoable {
public:
	// the forward action to actually do the thing
	virtual void redo() = 0;

	// how to reverse the forward action.
	virtual void undo() = 0;
};

using UndoableRef = std::shared_ptr<Undoable>;

class UndoManager {
public:
	static constexpr int MAX_UNDO_LEVELS = 40;

	UndoManager();

	/**
	 * To perform an undoable action - wrap it in an undoable
	 * and commit it here - you don't actually perform the action
	 * - instead commit() calls redo() on your Undoable.
	 */
	void commit(UndoableRef item);

	/**
	 * Same thing as above but with lambdas for convenience
	 */
	void commit(std::function<void()> &&redo, std::function<void()> &&undo);

	std::size_t size() const;

	/**
	 * You can group multiple commits by sandwiching between
	 * beginGroup() and endGroup() - so all the commits in between
	 * act as a single undo-redo step.
	 */
	void beginGroup();
	void endGroup();

	bool undo();
	bool redo();

	bool canUndo() const;
	bool canRedo() const;

	void clear();

private:
	UndoableRef undoGroup = nullptr;
	std::deque<UndoableRef>::iterator undoPos;
	std::deque<UndoableRef> undoStack;
};

///////////////////////////////////////////////////////////////////////////////
/// SOME HELPERS
template <typename T>
class AssignmentUndoable : public Undoable {
public:
	T &ref;
	T newVal;
	T oldVal;
	AssignmentUndoable(T &oldVal, T newVal)
		: ref(oldVal)
		, oldVal(oldVal)
		, newVal(newVal) {}

	void redo() override { ref = newVal; }
	void undo() override { ref = oldVal; }
};

template <typename T>
std::shared_ptr<AssignmentUndoable<T>> undoableAssignment(T &oldVal, T newVal) {
	return std::make_shared<AssignmentUndoable<T>>(oldVal, newVal);
}

/*
 Example of undoableAssigment
 
 UndoManager u;
 
 float f = 0.32f;
 // I want to change f to 0.5 in an undoable way.
 
 u.commit(undoableAssigment(f, 0.5));
 
 // f == 0.5 now
 
 u.undo();
 
 // f == 0.32 now
 
 
 */
