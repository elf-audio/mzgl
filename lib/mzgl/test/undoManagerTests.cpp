#include "tests.h"
#include "UndoManager.h"

TEST_CASE("undo", "[undo]") {
	UndoManager u;

	int d = 0;
	REQUIRE(u.size() == 0);
	u.commit(undoableAssignment(d, 3));
	REQUIRE(d == 3);
	REQUIRE(u.canUndo() == true);
	REQUIRE(u.undo() == true);
	REQUIRE(d == 0);
	REQUIRE(u.canUndo() == false);
	REQUIRE(u.undo() == false);

	float f = 0.f;
	float g = 1.f;
	u.commit(undoableAssignment(f, 0.25f));
	u.commit(undoableAssignment(g, 0.5f));
	u.commit(undoableAssignment(f, 0.75f));

	REQUIRE(u.size() == 3);
	REQUIRE(f == 0.75f);
	REQUIRE(g == 0.5f);

	REQUIRE(u.undo() == true);
	REQUIRE(u.undo() == true);
	REQUIRE(u.undo() == true);

	REQUIRE(f == 0.f);
	REQUIRE(g == 1.f);
	REQUIRE(u.canRedo() == true);
	REQUIRE(u.redo());

	REQUIRE(f == 0.25f);
	REQUIRE(g == 1.f);

	// now there are 2 more redo positions
	// but if we commit that should wipe
	// out the redos.
	u.commit(undoableAssignment(f, 0.75f));
	REQUIRE(u.canRedo() == false);
	REQUIRE(u.size() == 2);
}

TEST_CASE("memory limit eviction", "[undo][memory]") {
	UndoManager u(1000); // 1KB limit

	int dummy = 0;

	// Add items totaling 800 bytes
	u.commit([&]() { dummy = 1; }, [&]() { dummy = 0; }, 400);
	u.commit([&]() { dummy = 2; }, [&]() { dummy = 0; }, 400);
	REQUIRE(u.size() == 2);
	REQUIRE(u.getMemoryUsage() == 800);

	// Add item that pushes over limit
	u.commit([&]() { dummy = 3; }, [&]() { dummy = 0; }, 300);

	// First item should be evicted
	REQUIRE(u.size() == 2);
	REQUIRE(u.getMemoryUsage() == 700);

	// Verify dummy has the right value (last commit was executed)
	REQUIRE(dummy == 3);
}

TEST_CASE("oversized single item", "[undo][memory]") {
	UndoManager u(1000); // 1KB limit

	int dummy = 0;

	// Commit item larger than limit
	u.commit([&]() { dummy = 1; }, [&]() { dummy = 0; }, 2000);

	// Should keep it
	REQUIRE(u.size() == 1);
	REQUIRE(u.getMemoryUsage() == 2000);
	REQUIRE(dummy == 1);

	// Can't add another until this is removed (by eviction)
	u.commit([&]() { dummy = 2; }, [&]() { dummy = 0; }, 100);
	REQUIRE(u.size() == 1);
	REQUIRE(u.getMemoryUsage() == 100);
	REQUIRE(dummy == 2);
}

TEST_CASE("group memory accumulation", "[undo][memory]") {
	UndoManager u(10000); // 10KB limit

	int dummy = 0;

	u.beginGroup();
	u.commit([&]() { dummy += 1; }, [&]() { dummy -= 1; }, 100);
	u.commit([&]() { dummy += 2; }, [&]() { dummy -= 2; }, 200);
	u.commit([&]() { dummy += 3; }, [&]() { dummy -= 3; }, 300);
	u.endGroup();

	// Should be 1 item in stack with accumulated memory
	REQUIRE(u.size() == 1);
	REQUIRE(u.getMemoryUsage() == 600);
	REQUIRE(dummy == 6); // 1 + 2 + 3

	// Test undo
	u.undo();
	REQUIRE(dummy == 0);
	REQUIRE(u.getMemoryUsage() == 600); // Memory still tracked

	// Test redo
	u.redo();
	REQUIRE(dummy == 6);
}

TEST_CASE("branching removes memory", "[undo][memory]") {
	UndoManager u(10000); // 10KB limit

	int dummy = 0;

	u.commit([&]() { dummy = 1; }, [&]() { dummy = 0; }, 100);
	u.commit([&]() { dummy = 2; }, [&]() { dummy = 1; }, 200);
	u.commit([&]() { dummy = 3; }, [&]() { dummy = 2; }, 300);
	REQUIRE(u.getMemoryUsage() == 600);

	u.undo();
	u.undo();
	// Memory still 600 (items still in stack)
	REQUIRE(u.getMemoryUsage() == 600);
	REQUIRE(dummy == 1);

	// Branch: add new item (should remove items with 200 and 300 bytes)
	u.commit([&]() { dummy = 4; }, [&]() { dummy = 1; }, 150);

	// Should remove the 200 and 300 byte items
	REQUIRE(u.size() == 2);
	REQUIRE(u.getMemoryUsage() == 250); // 100 + 150
	REQUIRE(dummy == 4);
}

TEST_CASE("memory tracking with memorySize=0 (backwards compatibility)", "[undo][memory]") {
	UndoManager u(1000); // 1KB limit

	int dummy = 0;

	// Add many items with memorySize=0 (no tracking)
	for (int i = 0; i < 50; i++) {
		u.commit([&, i]() { dummy = i; }, [&]() { dummy = 0; });
	}

	// Should only enforce count limit (MAX_UNDO_LEVELS = 40)
	REQUIRE(u.size() == 40);
	REQUIRE(u.getMemoryUsage() == 0);
	REQUIRE(dummy == 49);
}

TEST_CASE("memory limit API", "[undo][memory]") {
	UndoManager u(5000); // 5KB initial limit

	REQUIRE(u.getMemoryLimit() == 5000);
	REQUIRE(u.getMemoryUsage() == 0);

	int dummy = 0;

	// Add items
	u.commit([&]() { dummy = 1; }, [&]() { dummy = 0; }, 2000);
	u.commit([&]() { dummy = 2; }, [&]() { dummy = 1; }, 2000);
	REQUIRE(u.size() == 2);
	REQUIRE(u.getMemoryUsage() == 4000);

	// Lower the limit - should trigger eviction
	u.setMemoryLimit(3000);
	REQUIRE(u.size() == 1);
	REQUIRE(u.getMemoryUsage() == 2000);
	REQUIRE(dummy == 2);

	// Raise the limit
	u.setMemoryLimit(10000);
	REQUIRE(u.getMemoryLimit() == 10000);
}

TEST_CASE("gesture memory accumulation", "[undo][memory]") {
	UndoManager u(10000); // 10KB limit

	int dummy = 0;

	u.beginGesture();
	u.commit([&]() { dummy = 1; }, [&]() { dummy = 0; }, 100);
	u.commit([&]() { dummy = 2; }, [&]() { dummy = 1; }, 200);
	u.commit([&]() { dummy = 3; }, [&]() { dummy = 2; }, 300);
	u.endGesture();

	// Should be 1 item in stack with accumulated memory
	REQUIRE(u.size() == 1);
	REQUIRE(u.getMemoryUsage() == 600);
	REQUIRE(dummy == 3);

	// Test undo
	u.undo();
	REQUIRE(dummy == 0);

	// Test redo
	u.redo();
	REQUIRE(dummy == 3);
}
