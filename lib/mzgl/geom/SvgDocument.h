//
//  SvgDocument.h
//  mzgl
//
//  SVG -> triangles.
//
//  The document keeps its paths as curves, so parsing happens once and
//  tessellate() can be called for any size: curves are flattened in OUTPUT
//  space to a tolerance, which is what keeps the vertex count proportional to
//  how big the thing is actually drawn rather than to the units the artwork
//  happened to be authored in.
//

#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <vector>

class Geometry;

class SvgDocument {
public:
	struct Options {
		// max distance, in output units, between a curve and its flattened polyline
		float tolerance = 0.25f;
		// false leaves Geometry::cols empty (for icons tinted with g.setColor)
		bool colors = true;
	};

	struct Stats {
		size_t shapes		 = 0;
		size_t convexFills	 = 0; // fills that skipped the triangulator
		size_t triangulated	 = 0; // fills that needed it
		size_t outlinePoints = 0; // flattened points, before triangulation
	};

	bool parse(std::string_view svg);
	bool load(const std::string &path);

	// viewBox size (or width/height when there's no viewBox)
	float width	 = 0;
	float height = 0;

	[[nodiscard]] bool empty() const { return shapes.empty(); }

	// transform maps document space (viewBox origin at 0,0) to output space.
	// Appends to geom.
	void tessellate(const glm::mat3 &transform,
					Geometry &geom,
					const Options &options,
					Stats *stats = nullptr) const;
	void tessellate(const glm::mat3 &transform, Geometry &geom) const { tessellate(transform, geom, Options {}); }

	// the transforms SvgVbo uses: artwork centred on the origin, then sized
	[[nodiscard]] glm::mat3 centredWithScale(float sx, float sy) const;
	[[nodiscard]] glm::mat3 centredWithScale(float s) const { return centredWithScale(s, s); }
	[[nodiscard]] glm::mat3 centredWithWidth(float w) const { return centredWithScale(w / width); }
	[[nodiscard]] glm::mat3 centredWithHeight(float h) const { return centredWithScale(h / height); }
	[[nodiscard]] glm::mat3 centredWithMaxDim(float d) const {
		return centredWithScale(d / (width > height ? width : height));
	}

	// bytes held by the parsed document
	[[nodiscard]] size_t memoryUsage() const;

	enum class Cap : uint8_t { Butt, Square, Round };
	enum class Join : uint8_t { Miter, Round, Bevel };
	enum Cmd : uint8_t { MoveTo, LineTo, CubicTo, Close };

	struct Affine {
		// | a c e |
		// | b d f |
		float a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;
		[[nodiscard]] glm::vec2 apply(float x, float y) const { return {a * x + c * y + e, b * x + d * y + f}; }
		[[nodiscard]] Affine operator*(const Affine &o) const {
			return {a * o.a + c * o.b,
					b * o.a + d * o.b,
					a * o.c + c * o.d,
					b * o.c + d * o.d,
					a * o.e + c * o.f + e,
					b * o.e + d * o.f + f};
		}
	};

	struct Shape {
		uint32_t firstCmd	= 0;
		uint32_t numCmds	= 0;
		uint32_t firstCoord = 0;
		Affine transform;
		glm::vec4 fill {0, 0, 0, 1};
		glm::vec4 stroke {0, 0, 0, 1};
		float strokeWidth = 1;
		float miterLimit  = 4;
		bool hasFill	  = false;
		bool hasStroke	  = false;
		bool evenOdd	  = false;
		Cap cap			  = Cap::Butt;
		Join join		  = Join::Miter;
	};

private:
	std::vector<uint8_t> cmds;
	std::vector<float> coords;
	std::vector<Shape> shapes;

	friend struct SvgParser;
};
