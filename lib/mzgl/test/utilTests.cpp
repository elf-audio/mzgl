#include "tests.h"

#include <fsystem/fsystem.h>
#include <mzgl/util/util.h>

TEST_CASE("trashOrDeleteTest", "[mzgl][util]") {
	auto path = fs::path {tempDir()} / "temp.txt";
	writeStringToFile(path.string(), "test");

	REQUIRE(fs::exists(path));
	deleteOrTrash(path.string());
	REQUIRE_FALSE(fs::exists(path));
	REQUIRE_NOTHROW(deleteOrTrash(path.string()));
}