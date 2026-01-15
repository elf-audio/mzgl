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
#include "Listenable.h"

/**
 * this is a thing that can be undone or redone,
 * the basic unit of the undo manager
 */
class Undoable {
public:
	virtual ~Undoable() {}
	// the forward action to actually do the thing
	virtual void redo() = 0;

	// how to reverse the forward action.
	virtual void undo() = 0;
};

using UndoableRef = std::shared_ptr<Undoable>;

class UndoManagerListener {
public:
	virtual ~UndoManagerListener() = default;
	virtual void undoRedoStateChanged() = 0;
};

class UndoManager : public Listenable<UndoManagerListener> {
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

	void beginGesture();
	void endGesture();

	bool undo();
	bool redo();

	bool canUndo() const;
	bool canRedo() const;

	void clear();

private:
	UndoableRef undoGroup	 = nullptr;
	UndoableRef gestureGroup = nullptr;
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
	std::function<void()> onChange = nullptr;
	AssignmentUndoable(T &_oldVal, T _newVal, std::function<void()> _onChange = nullptr)
		: ref(_oldVal)
		, oldVal(_oldVal)
		, newVal(_newVal)
		, onChange(_onChange) {}

	void redo() override {
		ref = newVal;
		if (onChange) onChange();
	}
	void undo() override {
		ref = oldVal;
		if (onChange) onChange();
	}
};

template <typename T>
std::shared_ptr<AssignmentUndoable<T>>
	undoableAssignment(T &oldVal, T newVal, std::function<void()> onChange = nullptr) {
	return std::make_shared<AssignmentUndoable<T>>(oldVal, newVal, onChange);
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
