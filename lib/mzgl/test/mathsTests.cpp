#include "tests.h"

#include "maths.h"

TEST_CASE("rad2deg2rad", "[maths]") {
	SECTION("Radians to Degrees") {
		REQUIRE(rad2deg(0.f) == Catch::Approx(0.0));
		REQUIRE(rad2deg(M_PI / 2) == Catch::Approx(90.0));
		REQUIRE(rad2deg(M_PI) == Catch::Approx(180.0));
		REQUIRE(rad2deg(3.f * M_PI / 2) == Catch::Approx(270.0));
		REQUIRE(rad2deg(2.f * M_PI) == Catch::Approx(360.0));
	}

	SECTION("Degrees to Radians") {
		REQUIRE(deg2rad(0.f) == Catch::Approx(0.0));
		REQUIRE(deg2rad(90.f) == Catch::Approx(M_PI / 2));
		REQUIRE(deg2rad(180.f) == Catch::Approx(M_PI));
		REQUIRE(deg2rad(270.f) == Catch::Approx(3 * M_PI / 2));
		REQUIRE(deg2rad(360.f) == Catch::Approx(2 * M_PI));
	}
}