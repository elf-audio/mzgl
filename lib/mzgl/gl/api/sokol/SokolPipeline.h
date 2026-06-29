#pragma once
#include "Graphics.h"
#include "sokol_gfx.h"
#include "SokolVertexAttr.h"
#include "log.h"
#include <memory>
#include <vector>
class Pipeline;
using PipelineRef = std::shared_ptr<Pipeline>;

class Pipeline {
public:
	static PipelineRef create(sg_shader shd,
							  const std::vector<SokolVertexAttr> &attrs,
							  bool usingIndices,
							  bool blending,
							  Graphics::BlendMode blendMode,
							  sg_primitive_type mode) {
		return std::shared_ptr<Pipeline>(new Pipeline(shd, attrs, usingIndices, blending, blendMode, mode));
	}

	// sg_isvalid() guard: the layer tree can outlive sg_shutdown() on close,
	// so don't touch a dead context.
	~Pipeline() {
		if (sg_isvalid() && pipeline.id != SG_INVALID_ID) {
			sg_destroy_pipeline(pipeline);
		}
	}

	void apply() {
		// In release builds sokol's validation layer is compiled out, so
		// applying a dead/invalid handle is a null deref inside
		// sg_apply_pipeline rather than an error - skip the draw instead.
		if (sg_query_pipeline_state(pipeline) != SG_RESOURCESTATE_VALID) {
			if (!warnedInvalid) {
				warnedInvalid = true;
				Log::e() << "Pipeline::apply(): pipeline handle " << pipeline.id
						 << " is invalid - skipping draws for this pipeline";
			}
			return;
		}
		sg_apply_pipeline(pipeline);
	}

private:
	Pipeline(sg_shader shd,
			 const std::vector<SokolVertexAttr> &attrs,
			 bool usingIndices,
			 bool blending,
			 Graphics::BlendMode blendMode,
			 sg_primitive_type mode) {
		pipelineDesc				= {};
		pipelineDesc.shader			= shd;
		pipelineDesc.index_type		= usingIndices ? SG_INDEXTYPE_UINT32 : SG_INDEXTYPE_NONE;
		pipelineDesc.primitive_type = mode;
		pipelineDesc.sample_count	= sg_query_desc().environment.defaults.sample_count;

		// Each attribute is placed at the shader location resolved by name (a.location)
		// and reads from its own vertex-buffer slot (a.bufferSlot). This decouples the
		// order buffers are bound from the order attributes are declared in the shader.
		for (const auto &a: attrs) {
			pipelineDesc.layout.attrs[a.location].format	   = a.format;
			pipelineDesc.layout.attrs[a.location].buffer_index = a.bufferSlot;
			pipelineDesc.layout.attrs[a.location].offset	   = a.offset;
			// For interleaved buffers several attrs share a slot and must declare
			// the stride explicitly; for the one-buffer-per-attr case stride stays
			// 0 and sokol computes it from the (single) attr's format.
			if (a.bufferStride > 0) {
				pipelineDesc.layout.buffers[a.bufferSlot].stride = a.bufferStride;
			}
			if (a.perInstance) {
				pipelineDesc.layout.buffers[a.bufferSlot].step_func = SG_VERTEXSTEP_PER_INSTANCE;
				pipelineDesc.layout.buffers[a.bufferSlot].step_rate = 1;
			}
		}
		if (blending) {
			pipelineDesc.colors[0].blend.enabled = true;

			if (blendMode == Graphics::BlendMode::Additive) {
				pipelineDesc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_ONE;
				pipelineDesc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE;
			} else if (blendMode == Graphics::BlendMode::Alpha) {
				pipelineDesc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
				pipelineDesc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
			}
		}
		pipeline = sg_make_pipeline(pipelineDesc);
		if (pipeline.id == SG_INVALID_ID) {
			Log::e() << "sg_make_pipeline failed - pipeline pool exhausted?";
		}
	}
	sg_pipeline pipeline;
	sg_pipeline_desc pipelineDesc;
	bool warnedInvalid = false;
};
