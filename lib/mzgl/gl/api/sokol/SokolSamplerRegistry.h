#pragma once

class SokolSamplerRegistry {
public:
	sg_sampler createOrReuseSampler(sg_sampler_desc desc) {
		for (auto &s: samplers) {
			if (samplersEqual(s->desc, desc)) {
				return s->sampler;
			}
		}
		printf("creating new sampler\n");
		samplers.emplace_back(std::make_shared<SamplerAndDesc>(sg_make_sampler(desc), desc));
		return samplers.back()->sampler;
	}

private:
	class SamplerAndDesc {
	public:
		SamplerAndDesc(sg_sampler theSampler, sg_sampler_desc theDesc)
			: sampler(theSampler)
			, desc(theDesc) {}
		sg_sampler sampler;
		sg_sampler_desc desc;
	};
	static bool samplersEqual(const sg_sampler_desc &a, const sg_sampler_desc &b) {
		return a.min_filter == b.min_filter && a.mag_filter == b.mag_filter && a.mipmap_filter == b.mipmap_filter
			   && a.wrap_u == b.wrap_u && a.wrap_v == b.wrap_v && a.wrap_w == b.wrap_w && a.min_lod == b.min_lod
			   && a.max_lod == b.max_lod && a.border_color == b.border_color && a.compare == b.compare
			   && a.max_anisotropy == b.max_anisotropy;
	}

	std::vector<std::shared_ptr<SamplerAndDesc>> samplers;
};