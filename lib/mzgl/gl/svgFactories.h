//
//  svgFactories.h
//  mzgl
//
//  SVG in, Vbo out. The artwork is centred on the origin and tessellated for
//  the size asked for, in pixels - so ask for the size it will be drawn at,
//  rather than building it at one size and scaling it with g.scale().
//

#pragma once

#include "Graphics.h"
#include "SvgDocument.h"
#include "Vbo.h"

#include <glm/glm.hpp>
#include <string>

namespace svg {
	// ignoreColor leaves the colours out of the vbo, for icons tinted with g.setColor().
	// All of these return nullptr if the svg doesn't parse.
	VboRef createVboWithMaxDim(const std::string &svgCode, float maxDim, bool ignoreColor);
	VboRef createVboWithWidth(const std::string &svgCode, float width, bool ignoreColor);
	VboRef createVboWithHeight(const std::string &svgCode, float height, bool ignoreColor);
	VboRef createVboWithWidthAndHeight(const std::string &svgCode, float width, float height, bool ignoreColor);

	// A multiple of the document's own size. This one doesn't know how big the
	// result is in pixels, so it's tessellated to a tolerance relative to the
	// artwork (see scaleIndependentTolerance) - prefer the ones above.
	VboRef createVboWithScale(const std::string &svgCode, float scale, bool ignoreColor);

	// For anything else: transform maps the document (viewBox origin at 0,0) to
	// where it's drawn, in pixels. Build one from SvgDocument::centredWith...()
	VboRef createVbo(const SvgDocument &doc, const glm::mat3 &transform, bool ignoreColor);
	VboRef createVbo(const SvgDocument &doc,
					 const glm::mat3 &transform,
					 bool ignoreColor,
					 const SvgDocument::Options &options);

	// For geometry whose final size on screen isn't known when it's built: as
	// smooth as SvgDocument's default at 500 px across, whatever the scale.
	float scaleIndependentTolerance(const SvgDocument &doc, float scaleX, float scaleY);
} // namespace svg
