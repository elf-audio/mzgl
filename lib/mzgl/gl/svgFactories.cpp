//
//  svgFactories.cpp
//  mzgl
//

#include "svgFactories.h"

#include "Geometry.h"

#include <algorithm>
#include <cmath>

namespace svg {
	namespace {
		bool parse(SvgDocument &doc, const std::string &svgCode) {
			return doc.parse(svgCode) && doc.width > 0 && doc.height > 0;
		}
	} // namespace

	float scaleIndependentTolerance(const SvgDocument &doc, float scaleX, float scaleY) {
		const float maxDim = std::max(std::abs(doc.width * scaleX), std::abs(doc.height * scaleY));
		return std::max(maxDim / 2000.f, 1e-4f);
	}

	VboRef createVbo(const SvgDocument &doc,
					 const glm::mat3 &transform,
					 bool ignoreColor,
					 const SvgDocument::Options &options) {
		SvgDocument::Options opts = options;
		opts.colors				  = !ignoreColor;
		Geometry geometry;
		doc.tessellate(transform, geometry, opts);
		auto vbo = Vbo::create();
		vbo->setGeometry(geometry);
		return vbo;
	}

	VboRef createVbo(const SvgDocument &doc, const glm::mat3 &transform, bool ignoreColor) {
		return createVbo(doc, transform, ignoreColor, SvgDocument::Options {});
	}

	VboRef createVboWithMaxDim(const std::string &svgCode, float maxDim, bool ignoreColor) {
		SvgDocument doc;
		if (!parse(doc, svgCode)) return nullptr;
		return createVbo(doc, doc.centredWithMaxDim(maxDim), ignoreColor);
	}

	VboRef createVboWithWidth(const std::string &svgCode, float width, bool ignoreColor) {
		SvgDocument doc;
		if (!parse(doc, svgCode)) return nullptr;
		return createVbo(doc, doc.centredWithWidth(width), ignoreColor);
	}

	VboRef createVboWithHeight(const std::string &svgCode, float height, bool ignoreColor) {
		SvgDocument doc;
		if (!parse(doc, svgCode)) return nullptr;
		return createVbo(doc, doc.centredWithHeight(height), ignoreColor);
	}

	VboRef createVboWithWidthAndHeight(const std::string &svgCode, float width, float height, bool ignoreColor) {
		SvgDocument doc;
		if (!parse(doc, svgCode)) return nullptr;
		return createVbo(doc, doc.centredWithScale(width / doc.width, height / doc.height), ignoreColor);
	}

	VboRef createVboWithScale(const std::string &svgCode, float scale, bool ignoreColor) {
		SvgDocument doc;
		if (!parse(doc, svgCode)) return nullptr;
		SvgDocument::Options options;
		options.tolerance = scaleIndependentTolerance(doc, scale, scale);
		return createVbo(doc, doc.centredWithScale(scale), ignoreColor, options);
	}
} // namespace svg
