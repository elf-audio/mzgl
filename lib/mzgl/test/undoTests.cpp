//
//  undoTests.cpp
//  Unit Tests
//
//  Created by Marek Bereza on 24/11/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#include "tests.h"

#include "UndoManager.h"

TEST_CASE("basic-undo", "[undo]") {
	UndoManager u;

	int a = 1;
	u.commit(undoableAssignment(a, 2));
	REQUIRE(a == 2);

	u.commit(undoableAssignment(a, 3));

	REQUIRE(a == 3);
	REQUIRE(u.canUndo() == true);
	REQUIRE(u.canRedo() == false);

	REQUIRE(u.undo() == true);

	REQUIRE(a == 2);
	REQUIRE(u.canUndo() == true);
	REQUIRE(u.canRedo() == true);

	REQUIRE(u.redo() == true);

	REQUIRE(a == 3);

	REQUIRE(u.canUndo() == true);
	REQUIRE(u.canRedo() == false);

	REQUIRE(u.undo() == true);
	REQUIRE(u.undo() == true);

	REQUIRE(a == 1);

	REQUIRE(u.canUndo() == false);
	REQUIRE(u.canRedo() == true);

	u.commit(undoableAssignment(a, 5));
	REQUIRE(u.canUndo() == true);
	REQUIRE(u.canRedo() == false);

	REQUIRE(a == 5);
}

TEST_CASE("undo-group", "[undo]") {
	UndoManager u;

	// basic group undo, nothing else in undo stack
	int b = 0;
	u.beginGroup();
	u.commit(undoableAssignment(b, 2));
	u.commit(undoableAssignment(b, 3));
	u.endGroup();

	REQUIRE(b == 3);
	REQUIRE(u.canUndo() == true);

	REQUIRE(u.undo() == true);
	REQUIRE(b == 0);
	REQUIRE(u.canUndo() == false);
	REQUIRE(u.canRedo() == true);
	REQUIRE(u.redo() == true);
	REQUIRE(b == 3);

	// now do a test where there is something in the stack
	// before the group undo
	u.commit(undoableAssignment(b, 1));
	u.beginGroup();
	u.commit(undoableAssignment(b, 5));
	u.commit(undoableAssignment(b, 7));
	u.endGroup();

	REQUIRE(b == 7);
	REQUIRE(u.canUndo() == true);
	REQUIRE(u.undo() == true);
	REQUIRE(b == 1);
	REQUIRE(u.canUndo() == true);
	REQUIRE(u.canRedo() == true);
	REQUIRE(u.redo() == true);

	REQUIRE(b == 7);
}
