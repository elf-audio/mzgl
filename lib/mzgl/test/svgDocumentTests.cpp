#include "tests.h"

#include "Graphics.h"
#include "Geometry.h"
#include "SvgDocument.h"

#include <cmath>
#include <string>

namespace {
	const float pi = 3.14159265f;

	std::string svg(const std::string &body, const std::string &attrs = "viewBox=\"0 0 100 100\"") {
		return "<svg xmlns=\"http://www.w3.org/2000/svg\" " + attrs + ">" + body + "</svg>";
	}

	// tessellates at 1:1 with the viewBox origin at 0,0
	Geometry tessellate(const std::string &code,
						float scale		= 1,
						float tolerance = 0.05f) {
		SvgDocument doc;
		REQUIRE(doc.parse(code));
		SvgDocument::Options options;
		options.tolerance	 = tolerance;
		Geometry geom;
		doc.tessellate(glm::mat3 {scale, 0, 0, 0, scale, 0, 0, 0, 1}, geom, options);
		REQUIRE(geom.indices.size() % 3 == 0);
		REQUIRE(geom.cols.size() == geom.verts.size());
		for (auto i: geom.indices)
			REQUIRE(i < geom.verts.size());
		return geom;
	}

	// overlapping triangles count twice, which is what catches a wrong fill rule
	float area(const Geometry &g) {
		float total = 0;
		for (size_t i = 0; i + 2 < g.indices.size(); i += 3) {
			const auto a = g.verts[g.indices[i]], b = g.verts[g.indices[i + 1]], c = g.verts[g.indices[i + 2]];
			total += std::abs((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)) * 0.5f;
		}
		return total;
	}

	float pathArea(const std::string &d, const std::string &attrs = "") {
		return area(tessellate(svg("<path d=\"" + d + "\" " + attrs + "/>")));
	}
} // namespace

TEST_CASE("svg document: basic shapes", "[svg-doc]") {
	SECTION("rect is two triangles, no triangulator needed") {
		SvgDocument doc;
		REQUIRE(doc.parse(svg("<rect x=\"10\" y=\"20\" width=\"30\" height=\"40\"/>")));
		Geometry g;
		SvgDocument::Stats stats;
		doc.tessellate(glm::mat3(1), g, {}, &stats);
		REQUIRE(g.verts.size() == 4);
		REQUIRE(g.indices.size() == 6);
		REQUIRE(stats.convexFills == 1);
		REQUIRE(stats.triangulated == 0);
		REQUIRE(area(g) == Catch::Approx(1200));
	}
	SECTION("circle") {
		const auto g = tessellate(svg("<circle cx=\"50\" cy=\"50\" r=\"40\"/>"));
		REQUIRE(area(g) == Catch::Approx(pi * 1600).epsilon(0.005));
	}
	SECTION("ellipse, rounded rect, polygon") {
		REQUIRE(area(tessellate(svg("<ellipse cx=\"50\" cy=\"50\" rx=\"40\" ry=\"20\"/>")))
				== Catch::Approx(pi * 800).epsilon(0.005));
		REQUIRE(area(tessellate(svg("<rect width=\"100\" height=\"50\" rx=\"10\"/>")))
				== Catch::Approx(5000 - (400 - pi * 100)).epsilon(0.005));
		REQUIRE(area(tessellate(svg("<polygon points=\"0,0 100,0 100,100\"/>"))) == Catch::Approx(5000));
	}
}

TEST_CASE("svg document: vertex count follows output size, not authoring units", "[svg-doc]") {
	const auto code	 = svg("<circle cx=\"50\" cy=\"50\" r=\"50\"/>");
	const auto small = tessellate(code, 0.2f, 0.25f);
	const auto big	 = tessellate(code, 10.f, 0.25f);
	REQUIRE(small.verts.size() < 24);
	REQUIRE(big.verts.size() > small.verts.size() * 3);
	REQUIRE(big.verts.size() < 200);
	REQUIRE(area(big) == Catch::Approx(pi * 500 * 500).epsilon(0.002));
}

TEST_CASE("svg document: path commands", "[svg-doc]") {
	SECTION("relative and absolute agree") {
		REQUIRE(pathArea("M10 10 H20 V20 H10 Z") == Catch::Approx(100));
		REQUIRE(pathArea("m10 10 h10 v10 h-10 z") == Catch::Approx(100));
		REQUIRE(pathArea("m10,10 10,0 0,10 -10,0z") == Catch::Approx(100)); // implicit linetos
	}
	SECTION("numbers run together") {
		REQUIRE(pathArea("M10-10L20-10 20 0 10 0z") == Catch::Approx(100));
		REQUIRE(pathArea("M.5.5l10 0 0 10-10 0z") == Catch::Approx(100));
		REQUIRE(pathArea("M1e1 1e1 h1E1 v10 h-10 z") == Catch::Approx(100));
	}
	SECTION("a subpath after z starts where the last one started") {
		REQUIRE(pathArea("M0 0 h10 v10 h-10 z h-10 v-10 h10 z") == Catch::Approx(200));
	}
	SECTION("arcs") {
		REQUIRE(pathArea("M0 50 A50 50 0 1 1 100 50 A50 50 0 1 1 0 50 Z") == Catch::Approx(pi * 2500).epsilon(0.005));
		// flags with no separators
		REQUIRE(pathArea("M0 50a50 50 0 11100 0a50 50 0 11-100 0z") == Catch::Approx(pi * 2500).epsilon(0.005));
	}
	SECTION("quadratic and smooth curves") {
		// parabola segment: 2/3 of the bounding rectangle
		REQUIRE(pathArea("M0 0 Q50 100 100 0 Z") == Catch::Approx(100 * 50 * 2.f / 3.f).epsilon(0.005));
		// two lobes either side of the closing line: the outline crosses itself
		REQUIRE(pathArea("M0 0 Q25 50 50 0 T100 0 Z") == Catch::Approx(2 * 50 * 25 * 2.f / 3.f).epsilon(0.005));
		REQUIRE(pathArea("M0 0 C0 50 50 50 50 0 S100 -50 100 0 Z") > 0);
	}
}

TEST_CASE("svg document: fill rules", "[svg-doc]") {
	const std::string sameWay  = "M0 0 H100 V100 H0 Z M30 30 H70 V70 H30 Z";
	const std::string opposite = "M0 0 H100 V100 H0 Z M30 30 V70 H70 V30 Z";
	const std::string nested   = "M0 0 H100 V100 H0 Z M20 20 H80 V80 H20 Z M40 40 H60 V60 H40 Z";

	{
		auto fill = [&](const std::string &d, const std::string &rule) {
			return area(tessellate(svg("<path d=\"" + d + "\" fill-rule=\"" + rule + "\"/>")));
		};
		REQUIRE(fill(sameWay, "evenodd") == Catch::Approx(10000 - 1600));
		REQUIRE(fill(sameWay, "nonzero") == Catch::Approx(10000));
		REQUIRE(fill(opposite, "evenodd") == Catch::Approx(10000 - 1600));
		REQUIRE(fill(opposite, "nonzero") == Catch::Approx(10000 - 1600));
		REQUIRE(fill(nested, "evenodd") == Catch::Approx(10000 - 3600 + 400));
		// two separate islands in one path
		REQUIRE(fill("M0 0 H10 V10 H0 Z M50 50 H60 V60 H50 Z", "nonzero") == Catch::Approx(200));
	}
}

TEST_CASE("svg document: strokes", "[svg-doc]") {
	auto stroke = [](const std::string &attrs) {
		return area(tessellate(
			svg("<path d=\"M10 50 H90\" fill=\"none\" stroke=\"#fff\" stroke-width=\"10\" " + attrs + "/>")));
	};
	REQUIRE(stroke("") == Catch::Approx(800));
	REQUIRE(stroke("stroke-linecap=\"square\"") == Catch::Approx(900));
	REQUIRE(stroke("stroke-linecap=\"round\"") == Catch::Approx(800 + pi * 25).epsilon(0.01));

	SECTION("closed square, mitred: outer 110, inner 90") {
		const auto g = tessellate(svg("<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"none\" "
									  "stroke=\"#fff\" stroke-width=\"10\"/>",
									  "viewBox=\"-10 -10 120 120\""));
		REQUIRE(area(g) == Catch::Approx(110 * 110 - 90 * 90));
	}
	SECTION("a sharp corner past the miter limit is bevelled") {
		auto extent = [](const std::string &attrs) {
			const auto g = tessellate(svg("<path d=\"M0 40 L100 50 L0 60\" fill=\"none\" stroke=\"#fff\" "
										  "stroke-width=\"4\" "
										  + attrs + "/>",
										  "viewBox=\"0 0 400 100\""));
			float maxX = 0;
			for (auto v: g.verts)
				maxX = std::max(maxX, v.x);
			return maxX;
		};
		REQUIRE(extent("") < 104);
		REQUIRE(extent("stroke-miterlimit=\"100\"") > 115);
		REQUIRE(extent("stroke-linejoin=\"round\"") == Catch::Approx(102).margin(0.1));
	}
	SECTION("stroke width scales with the transform") {
		const auto g = tessellate(svg("<path d=\"M10 50 H90\" stroke=\"#fff\" stroke-width=\"10\"/>"), 3);
		REQUIRE(area(g) == Catch::Approx(800 * 9));
	}
}

TEST_CASE("svg document: style and structure", "[svg-doc]") {
	SECTION("default fill is black, fill=none inherits from the group") {
		auto g = tessellate(svg("<rect width=\"10\" height=\"10\"/>"));
		REQUIRE(g.cols[0] == glm::vec4(0, 0, 0, 1));
		g = tessellate(svg("<g fill=\"none\"><rect width=\"10\" height=\"10\"/></g>"));
		REQUIRE(g.verts.empty());
		g = tessellate(svg("<g fill=\"white\" opacity=\"0.5\"><rect width=\"10\" height=\"10\" fill-opacity=\"0.5\"/></g>"));
		REQUIRE(g.cols[0] == glm::vec4(1, 1, 1, 0.25f));
	}
	SECTION("style attribute beats presentation attributes") {
		const auto g = tessellate(svg("<rect width=\"10\" height=\"10\" fill=\"#f00\" style=\"fill: #00ff00; opacity:.5\"/>"));
		REQUIRE(g.cols[0] == glm::vec4(0, 1, 0, 0.5f));
	}
	SECTION("transforms") {
		auto g = tessellate(svg("<g transform=\"translate(10 20) scale(2)\"><rect width=\"10\" height=\"10\"/></g>"));
		REQUIRE(area(g) == Catch::Approx(400));
		REQUIRE(g.verts[0] == glm::vec2(10, 20));
		g = tessellate(svg("<rect width=\"10\" height=\"10\" transform=\"rotate(90, 0, 0)\"/>"));
		REQUIRE(g.verts[1].x == Catch::Approx(0).margin(1e-4));
		REQUIRE(g.verts[1].y == Catch::Approx(10));
	}
	SECTION("viewBox origin is subtracted, dimensions come from it") {
		SvgDocument doc;
		REQUIRE(doc.parse(svg("<rect x=\"5\" y=\"5\" width=\"10\" height=\"10\"/>", "viewBox=\"5 5 24.5 12\"")));
		REQUIRE(doc.width == 24.5f);
		REQUIRE(doc.height == 12.f);
		Geometry g;
		doc.tessellate(glm::mat3(1), g);
		REQUIRE(g.verts[0] == glm::vec2(0, 0));
		// centred, twice the size
		g.clear();
		doc.tessellate(doc.centredWithScale(2), g);
		REQUIRE(g.verts[0] == glm::vec2(-24.5f, -12.f));
	}
	SECTION("no viewBox falls back to width/height") {
		SvgDocument doc;
		REQUIRE(doc.parse(svg("<rect width=\"1\" height=\"1\"/>", "width=\"32px\" height=\"16\"")));
		REQUIRE(doc.width == 32);
		REQUIRE(doc.height == 16);
	}
	SECTION("use, defs") {
		const auto g = tessellate(svg("<defs><rect id=\"r\" width=\"10\" height=\"10\"/></defs>"
									  "<use href=\"#r\" x=\"20\" fill=\"#f00\"/><use xlink:href=\"#r\" y=\"50\"/>"));
		REQUIRE(area(g) == Catch::Approx(200));
		REQUIRE(g.verts[0] == glm::vec2(20, 0));
	}
}

TEST_CASE("svg document: malformed input doesn't crash", "[svg-doc]") {
	for (const char *d: {"",
						 "M",
						 "M 10",
						 "L 5 5",
						 "z",
						 "M0 0 C 1",
						 "M0 0 A",
						 "M0 0 A 10 10 0 2 2 5 5",
						 "M0 0 L nan nan",
						 "10 10 20 20",
						 "M0 0 L10 10 X 5 5",
						 "M0 0 L0 0 L0 0 Z",
						 "M0 0 H10 H20 H5 Z",
						 "M0 0 L10 0 L5 0 L0 0 Z",
						 "M1e999 0 L0 1e999 L5 5 Z",
						 "M0 0 A0 0 0 0 0 10 10 Z"}) {
		tessellate(svg(std::string("<path stroke=\"#000\" stroke-linejoin=\"round\" d=\"") + d + "\"/>"));
	}
	SvgDocument doc;
	REQUIRE_FALSE(doc.parse("<svg><path"));
	REQUIRE_FALSE(doc.parse(""));
	REQUIRE(doc.parse("<svg/>"));
	REQUIRE(doc.empty());
	REQUIRE(doc.parse(svg("<use href=\"#missing\"/><g id=\"a\"><use href=\"#a\"/></g><rect/><circle/>"
						  "<polygon points=\"1\"/><path/><rect width=\"-1\" height=\"5\"/>"
						  "<g transform=\"rotate(\"><rect width=\"1\" height=\"1\" fill=\"url(#g)\"/></g>")));
	Geometry g;
	doc.tessellate(glm::mat3(1), g);
	doc.tessellate(glm::mat3(0), g); // everything collapses to a point
}
