#include "tests.h"

#include "filesystem.h"
#include "util.h"

TEST_CASE("trashOrDeleteTest", "[mzgl][util]") {
	auto path = fs::path {tempDir()} / "temp.txt";
	writeStringToFile(path, "test");

	REQUIRE(fs::exists(path));
	deleteOrTrash(path);
	REQUIRE_FALSE(fs::exists(path));
	REQUIRE_NOTHROW(deleteOrTrash(path));
}