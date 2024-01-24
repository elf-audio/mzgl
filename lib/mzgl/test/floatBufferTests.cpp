#include "tests.h"

#include <mzgl/util/FloatBuffer.h>

SCENARIO("FloatBuffer can be created and accessed", "[float-buffer]") {
	GIVEN("A FloatBuffer") {
		FloatBuffer buf(10);
		REQUIRE(buf.size() == 10);
		REQUIRE(buf[0] == 0.0f);
		REQUIRE(buf[9] == 0.0f);
		WHEN("Values are set") {
			buf[0] = 1.0f;
			buf[9] = 2.0f;
			THEN("Values are retrieved") {
				REQUIRE(buf[0] == 1.0f);
				REQUIRE(buf[9] == 2.0f);
			}
		}
	}
}

SCENARIO("FloatBuffer can be interpolated", "[float-buffer]") {
	GIVEN("A FloatBuffer") {
		FloatBuffer buf(10);
		for (int i = 0; i < 10; i++) {
			buf[i] = i;
		}
		WHEN("Interpolated") {
			float val = buf.interpolate(0.5);
			THEN("Value is interpolated") {
				REQUIRE(val == 0.5);
			}
		}

		buf[3 * 2] = 0.f;
		buf[4 * 2] = 1.f;

		buf[3 * 2 + 1] = 5.f;
		buf[4 * 2 + 1] = 6.f;

		WHEN("Stereo interpolated") {
			float L, R;
			buf.interpolateStereo(3.5, L, R);
			THEN("Values are interpolated") {
				REQUIRE(L == 0.5);
				REQUIRE(R == 5.5);
			}
		}
	}
}