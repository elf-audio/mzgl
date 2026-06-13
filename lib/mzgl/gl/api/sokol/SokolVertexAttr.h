#pragma once
#include "sokol_gfx.h"

// Describes how one vertex buffer maps onto a shader's vertex stage:
//  - location:    the shader attribute slot, resolved by NAME from the shader's
//                 sokol-shdc reflection (so the order attributes are declared in
//                 the .glsl is irrelevant)
//  - format:      the buffer's vertex format
//  - bufferSlot:  index into sg_bindings.vertex_buffers where the data lives
//  - perInstance: whether this buffer steps once per instance
struct SokolVertexAttr {
	int location			= 0;
	sg_vertex_format format = SG_VERTEXFORMAT_INVALID;
	int bufferSlot			= 0;
	bool perInstance		= false;
};
