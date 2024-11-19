
#include "tests.h"
#include "Rectf.h"

SCENARIO("Rects can be set from the center X", "[rectf]") {
	GIVEN("A rectangle") {
		Rectf rect {0.f, 0.f, 100.f, 50.f};
		WHEN("The rectangle has its center X set") {
			rect.setCentreX(200.f);
			THEN("The X pos should be set appropriately") {
				REQUIRE(rect.x == Catch::Approx(150.f).epsilon(1e-3));
				REQUIRE(rect.width == Catch::Approx(100.f).epsilon(1e-3));
				REQUIRE(rect.height == Catch::Approx(50.f).epsilon(1e-3));
				REQUIRE(rect.y == Catch::Approx(0.f).epsilon(1e-3));
			}
		}
	}
}

SCENARIO("Rects can be set from the center Y", "[rectf]") {
	GIVEN("A rectangle") {
		Rectf rect {0.f, 0.f, 100.f, 50.f};
		WHEN("The rectangle has its center Y set") {
			rect.setCentreY(200.f);
			THEN("The Y pos should be set appropriately") {
				REQUIRE(rect.x == Catch::Approx(0.f).epsilon(1e-3));
				REQUIRE(rect.width == Catch::Approx(100.f).epsilon(1e-3));
				REQUIRE(rect.height == Catch::Approx(50.f).epsilon(1e-3));
				REQUIRE(rect.y == Catch::Approx(175.f).epsilon(1e-3));
			}
		}
	}
}