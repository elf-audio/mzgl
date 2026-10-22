#pragma once

// Editor sizing shared by every plugin format's view (VST3, CLAP, AUv2).
// Logical pixels; the per-platform views multiply by the host's content scale.
// Lives in mzglvst for historical reasons (it started in MzglVST3View.h).

namespace mzglvst {

	struct ViewConfig {
		int defaultWidth  = 1024;
		int defaultHeight = 300;
		int minWidth	  = 512;
		int minHeight	  = 150;
		int maxWidth	  = 4096;
		int maxHeight	  = 1200;
	};

} // namespace mzglvst
