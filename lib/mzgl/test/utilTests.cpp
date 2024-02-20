#include "tests.h"

#include "filesystem.h"
#include "util.h"
TEST_CASE("trashOrDeleteTest", "[mzgl][util]") {
	auto path = tempDir() + "/tmp.txt";

	writeStringToFile(path, "test");

	REQUIRE(fs::exists(path) == true);
	deleteOrTrash(path);
	REQUIRE(fs::exists(path) == false);
	REQUIRE_NOTHROW(deleteOrTrash(path));
}