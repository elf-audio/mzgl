#pragma once

// Editor sizing shared by every plugin format's view (VST3, CLAP, AUv2).
// Logical pixels; the per-platform views multiply by the host's content scale.
// Lives in mzglvst for historical reasons (it started in MzglVST3View.h).

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace mzglvst {

	struct ViewConfig {
		int defaultWidth  = 1024;
		int defaultHeight = 300;
		int minWidth	  = 512;
		int minHeight	  = 150;
		int maxWidth	  = 4096;
		int maxHeight	  = 1200;
		// width / height to hold the view at while the user resizes (0 = free).
		// Keep default/min/max on the same ratio; the views' size-constraint
		// callbacks shrink the host's proposed rect onto it (constrainViewSize below).
		float aspectRatio = 0.f;
	};

	// Open at the size the plugin instance remembers (Plugin::getSavedEditorSize,
	// logical pixels) instead of the configured default, clamped to the limits.
	inline void applySavedEditorSize(ViewConfig &config, const std::optional<std::pair<int, int>> &saved) {
		if (!saved || saved->first <= 0 || saved->second <= 0) return;
		config.defaultWidth	 = std::max(config.minWidth, std::min(config.maxWidth, saved->first));
		config.defaultHeight = std::max(config.minHeight, std::min(config.maxHeight, saved->second));
	}

	// Shared by every format's view: clamp a proposed (w, h) to the limits,
	// then - if the config fixes an aspect ratio - shrink it onto that ratio (the
	// largest rect of the right shape that fits inside the proposal, as JUCE does).
	// The limits are passed in rather than read off the config because the
	// Windows/Linux views pre-scale them by the host's content scale.
	// The result depends only on the proposal, never on the current size: a rule
	// like "keep whichever edge the host moved" oscillates, because hosts keep
	// proposing the mouse rect every frame while our own onSize() keeps changing
	// what "moved" means, so the view flips between the width-fit and height-fit
	// answers.
	inline void
		constrainViewSize(const ViewConfig &config, int minW, int minH, int maxW, int maxH, int &w, int &h) {
		w = std::max(minW, std::min(maxW, w));
		h = std::max(minH, std::min(maxH, h));
		if (config.aspectRatio <= 0.f) return;

		const float aspect = config.aspectRatio;
		// Already on the ratio to within a pixel: hand it back untouched, so the
		// rect we return is a fixed point when the host re-asks with it.
		if (std::abs(h - w / aspect) < 1.f) return;
		if (w / (float) h > aspect) {
			w = (int) (h * aspect + 0.5f); // too wide: width follows height
		} else {
			h = (int) (w / aspect + 0.5f); // too tall: height follows width
		}
		// Re-clamp without breaking the ratio: whichever limit bites wins.
		if (h > maxH) {
			h = maxH;
			w = (int) (h * aspect + 0.5f);
		}
		if (w > maxW) {
			w = maxW;
			h = (int) (w / aspect + 0.5f);
		}
		if (h < minH) {
			h = minH;
			w = (int) (h * aspect + 0.5f);
		}
		if (w < minW) {
			w = minW;
			h = (int) (w / aspect + 0.5f);
		}
	}

} // namespace mzglvst
