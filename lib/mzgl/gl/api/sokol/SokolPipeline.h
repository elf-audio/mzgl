#pragma once
#include "Graphics.h"
#include "sokol_gfx.h"
#include <memory>
#include <vector>
class Pipeline;
using PipelineRef = std::shared_ptr<Pipeline>;

class Pipeline {
public:
	static PipelineRef create(sg_shader shd,
							  const std::vector<sg_vertex_format> &attrs,
							  bool usingIndices,
							  bool blending,
							  Graphics::BlendMode blendMode,
							  sg_primitive_type mode,
							  bool isInstancing) {
		return std::shared_ptr<Pipeline>(
			new Pipeline(shd, attrs, usingIndices, blending, blendMode, mode, isInstancing));
	}

	void apply() { sg_apply_pipeline(pipeline); }

private:
	Pipeline(sg_shader shd,
			 const std::vector<sg_vertex_format> &attrs,
			 bool usingIndices,
			 bool blending,
			 Graphics::BlendMode blendMode,
			 sg_primitive_type mode,
			 bool isInstancing) {
		pipelineDesc = {
			.index_type		= usingIndices ? SG_INDEXTYPE_UINT32 : SG_INDEXTYPE_NONE,
			.primitive_type = mode,
			.shader			= shd,
			.sample_count	= 4,
		};

		for (int i = 0; i < attrs.size(); i++) {
			pipelineDesc.layout.attrs[i] = {.format = attrs[i], .buffer_index = i};
		}
		if (isInstancing) {
			int instanceIndexBuffer									   = attrs.size() - 1;
			pipelineDesc.layout.buffers[instanceIndexBuffer].step_func = SG_VERTEXSTEP_PER_INSTANCE;
			pipelineDesc.layout.buffers[instanceIndexBuffer].step_rate = 1;
		}
		if (blending) {
			pipelineDesc.colors[0].blend = (sg_blend_state) {
				.enabled = true,
			};

			if (blendMode == Graphics::BlendMode::Additive) {
				pipelineDesc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_ONE;
				pipelineDesc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE;
			} else if (blendMode == Graphics::BlendMode::Alpha) {
				pipelineDesc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
				pipelineDesc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
			}
		}
		pipeline = sg_make_pipeline(pipelineDesc);
	}
	sg_pipeline pipeline;
	sg_pipeline_desc pipelineDesc;
};