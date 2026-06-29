#pragma once
#include "sokol_gfx.h"

// Describes how one vertex buffer maps onto a shader's vertex stage:
//  - location:    the shader attribute slot, resolved by NAME from the shader's
//                 sokol-shdc reflection (so the order attributes are declared in
//                 the .glsl is irrelevant)
//  - format:      the buffer's vertex format
//  - bufferSlot:  index into sg_bindings.vertex_buffers where the data lives
//  - perInstance: whether this buffer steps once per instance
//  - offset:      byte offset of this attr within its vertex (for interleaved
//                 buffers where several attrs share one bufferSlot). 0 when each
//                 attr has its own tightly-packed buffer.
//  - bufferStride: byte stride of the shared buffer (interleaved only). 0 lets
//                 sokol auto-compute (correct for the one-buffer-per-attr case).
struct SokolVertexAttr {
	int location			= 0;
	sg_vertex_format format = SG_VERTEXFORMAT_INVALID;
	int bufferSlot			= 0;
	bool perInstance		= false;
	int offset				= 0;
	int bufferStride		= 0;
};
