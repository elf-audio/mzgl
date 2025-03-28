#include "tests.h"
#include "Graphics.h"

bool approxEquals(float one, float two) {
	return fabs(one - two) < 1e-2;
}

bool equals(const glm::vec4 &one, const glm::vec4 &two) {
	return approxEquals(one.x, two.x) && approxEquals(one.y, two.y) && approxEquals(one.z, two.z)
		   && approxEquals(one.a, two.a);
}

TEST_CASE("Validation functions", "[colors]") {
	SECTION("Validate isHexColor") {
		REQUIRE(isHexColor("#FFFFFF"));
		REQUIRE(isHexColor("#FFF"));
		REQUIRE_FALSE(isHexColor("FFF"));
		REQUIRE_FALSE(isHexColor("#GGGGGG"));
	}

	SECTION("Validate isRGBColor") {
		REQUIRE(isRGBColor("rgb(255, 255, 255)"));
		REQUIRE(isRGBColor("rgb(0,0,0)"));
		REQUIRE(isRGBColor("rgb(256, 0, 0)"));
		REQUIRE_FALSE(isRGBColor("rgb(abc)"));
		REQUIRE_FALSE(isRGBColor("rgb(255,255,255"));
		REQUIRE_FALSE(isRGBColor("rgba(255,255,255"));
	}

	SECTION("Validate isRGBAColour") {
		REQUIRE(isRGBAColor("rgba(255, 255, 255, 1)"));
		REQUIRE(isRGBAColor("rgba(0, 0, 0, 0.5)"));
		REQUIRE(isRGBAColor("rgba(255, 255, 255)"));
		REQUIRE_FALSE(isRGBAColor("rgba(255, 255, 255, x)"));
		REQUIRE_FALSE(isRGBAColor("rgba(255, 255, 255, -0.1)"));
		REQUIRE_FALSE(isRGBAColor("rgba(abc)"));
		REQUIRE_FALSE(isRGBAColor("rgba(255,255,255"));
		REQUIRE_FALSE(isRGBAColor("rgb(255,255,255"));
	}
}

TEST_CASE("Hex color conversion", "[colours]") {
	SECTION("Convert integer hex to glm::vec4") {
		REQUIRE(equals(hexColor(0xFFFFFF), glm::vec4(1, 1, 1, 1)));
		REQUIRE(equals(hexColor(0xFF0000), glm::vec4(1, 0, 0, 1)));
		REQUIRE(equals(hexColor(0x000000), glm::vec4(0, 0, 0, 1)));
	}

	SECTION("Convert string hex to glm::vec4") {
		REQUIRE(equals(hexColor("#FFFFFF"), glm::vec4(1, 1, 1, 1)));
		REQUIRE(equals(hexColor("#FF0000"), glm::vec4(1, 0, 0, 1)));
		REQUIRE(equals(hexColor("#000000"), glm::vec4(0, 0, 0, 1)));
	}
}

TEST_CASE("SVG hex color conversion", "[colours]") {
	SECTION("Convert valid SVG hex color to glm::vec4") {
		REQUIRE(equals(svgHexColor("#FFFFFF"), glm::vec4(1, 1, 1, 1)));
		REQUIRE(equals(svgHexColor("#FF0000"), glm::vec4(1, 0, 0, 1)));
	}
}

TEST_CASE("Named colors", "[colours]") {
	SECTION("Convert named colors to glm::vec4") {
		REQUIRE(equals(namedColor("black"), glm::vec4(0, 0, 0, 1)));
		REQUIRE(equals(namedColor("white"), glm::vec4(1, 1, 1, 1)));
		REQUIRE(equals(namedColor("red"), glm::vec4(1, 0, 0, 1)));
		REQUIRE(equals(namedColor("green"), glm::vec4(0, 0.5, 0, 1)));
		REQUIRE(equals(namedColor("blue"), glm::vec4(0, 0, 1, 1)));
	}

	SECTION("Handle unsupported named colors") {
		REQUIRE(equals(namedColor("unknown"), glm::vec4(1, 1, 1, 1)));
	}
}

TEST_CASE("RGB and RGBA color parsing", "[colours]") {
	SECTION("Convert rgb() to glm::vec4") {
		REQUIRE(equals(rgbColor("rgb(255, 0, 0)"), glm::vec4(1, 0, 0, 1)));
		REQUIRE(equals(rgbColor("rgb(0, 255, 0)"), glm::vec4(0, 1, 0, 1)));
		REQUIRE(equals(rgbColor("rgb(0, 0, 255)"), glm::vec4(0, 0, 1, 1)));
	}

	SECTION("Convert rgba() to glm::vec4") {
		REQUIRE(equals(rgbaColor("rgba(255, 0, 0, 0.5)"), glm::vec4(1, 0, 0, 0.5)));
		REQUIRE(equals(rgbaColor("rgba(0, 255, 0, 1)"), glm::vec4(0, 1, 0, 1)));
	}
}