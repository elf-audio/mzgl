#include "tests.h"
#include "util.h"

TEST_CASE("getAvailableMemory reports a positive value", "[available-memory]") {
	const auto available = getAvailableMemory();
	REQUIRE(available.has_value());
	REQUIRE(*available > 0);
}
