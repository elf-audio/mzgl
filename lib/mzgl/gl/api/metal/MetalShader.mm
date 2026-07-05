#include "MetalShader.h"
#include "MetalAPI.h"
#include "MetalContext.h"
#include "Graphics.h"
#include "mzAssert.h"
#include <mutex>
#include <map>

#include <TargetConditionals.h>

// which slang variant of the sokol-shdc output to compile
static sg_backend metalShaderBackend() {
#if TARGET_OS_SIMULATOR
	return SG_BACKEND_METAL_SIMULATOR;
#elif TARGET_OS_IOS
	return SG_BACKEND_METAL_IOS;
#else
	return SG_BACKEND_METAL_MACOS;
#endif
}

// buffer-index / setBytes-limit constants shared with MetalVbo.mm live in
// MetalContext.h (mzglMetalVertexBufferIndexBase / mzglMetalSetBytesLimit)

namespace {
	// Process-global cache of compiled MTLFunctions and their pipelines,
	// shared by every Graphics/plugin instance in the process (mirrors the
	// SokolShader cache). Keyed by the ShaderDef's descFunc pointer.
	struct SharedShader {
		id<MTLFunction> vertexFunc	 = nil;
		id<MTLFunction> fragmentFunc = nil;
		int refs					 = 0;
		std::map<size_t, id<MTLRenderPipelineState>> pipelines;
	};

	struct ShaderCache {
		std::mutex mut;
		std::map<ShaderDescFn, SharedShader> entries;

		static ShaderCache &instance() {
			// deliberately leaked: shader destructors run at exit time and
			// must not touch a destroyed mutex
			static ShaderCache *c = new ShaderCache();
			return *c;
		}
	};

	id<MTLFunction> compileMSL(const char *source, const char *entry, const std::string &shaderName) {
		if (source == nullptr) {
			Log::e() << "no Metal source for shader '" << shaderName
					 << "' - were the shaders compiled with the metal slang variants?";
			return nil;
		}
		NSError *err		  = nil;
		id<MTLLibrary> lib	  = [mzglMetal::device() newLibraryWithSource:[NSString stringWithUTF8String:source]
																  options:nil
																	error:&err];
		if (lib == nil) {
			Log::e() << "Metal shader compile failed for '" << shaderName
					 << "': " << (err ? err.localizedDescription.UTF8String : "unknown error");
			return nil;
		}
		id<MTLFunction> fn = [lib newFunctionWithName:[NSString stringWithUTF8String:entry]];
		if (fn == nil) {
			Log::e() << "Metal shader entry '" << entry << "' not found for '" << shaderName << "'";
		}
		return fn;
	}
} // namespace

MetalShader::MetalShader(Graphics &g, const std::string &shaderName)
	: Shader(g) {
	name	  = shaderName;
	auto *api = static_cast<MetalAPI *>(&g.getAPI());
	def		  = api->getShaderDef(shaderName);
	mzAssert(def != nullptr, ("no shader registered with the name '" + shaderName + "'").c_str());

	{
		auto &cache = ShaderCache::instance();
		std::lock_guard<std::mutex> l(cache.mut);
		auto &e		= cache.entries[def->descFunc];
		sharedEntry = &e;
		if (e.refs == 0) {
			const sg_shader_desc *desc = def->descFunc(metalShaderBackend());
			e.vertexFunc			   = compileMSL(desc->vs.source, desc->vs.entry ? desc->vs.entry : "main0", shaderName);
			e.fragmentFunc = compileMSL(desc->fs.source, desc->fs.entry ? desc->fs.entry : "main0", shaderName);
		}
		e.refs++;
	}

	vertUniforms.resize(def->uniformblockSizeFunc(SG_SHADERSTAGE_VS, "vertParams"), 0);
	fragUniforms.resize(def->uniformblockSizeFunc(SG_SHADERSTAGE_FS, "fragParams"), 0);

	colorUniformOffset = uniformLocation("color");
	mvpUniformOffset   = uniformLocation("mvp");
	mzAssert(mvpUniformOffset != nullptr, "mvp uniform not found in shader");
}

MetalShader::~MetalShader() {
	auto &cache = ShaderCache::instance();
	std::lock_guard<std::mutex> l(cache.mut);
	auto *e = static_cast<SharedShader *>(sharedEntry);
	if (e == nullptr) return;
	if (--e->refs <= 0) {
		cache.entries.erase(def->descFunc);
	}
}

void MetalShader::begin() {
	g.currShader = this;
}
void MetalShader::end() {
	g.currShader = nullptr;
}

int MetalShader::attrSlot(const std::string &attrName) const {
	return def->attrSlotFunc(attrName.c_str());
}

uint8_t *MetalShader::uniformLocation(const std::string &uname) {
	int offset = def->uniformOffsetFunc(SG_SHADERSTAGE_VS, "vertParams", uname.c_str());
	if (offset != -1) return vertUniforms.data() + offset;
	offset = def->uniformOffsetFunc(SG_SHADERSTAGE_FS, "fragParams", uname.c_str());
	if (offset != -1) return fragUniforms.data() + offset;
	return nullptr;
}

static MTLVertexFormat toMTLFormat(sg_vertex_format f) {
	switch (f) {
		case SG_VERTEXFORMAT_FLOAT: return MTLVertexFormatFloat;
		case SG_VERTEXFORMAT_FLOAT2: return MTLVertexFormatFloat2;
		case SG_VERTEXFORMAT_FLOAT3: return MTLVertexFormatFloat3;
		case SG_VERTEXFORMAT_FLOAT4: return MTLVertexFormatFloat4;
		default: return MTLVertexFormatInvalid;
	}
}

static int formatSize(sg_vertex_format f) {
	switch (f) {
		case SG_VERTEXFORMAT_FLOAT: return 4;
		case SG_VERTEXFORMAT_FLOAT2: return 8;
		case SG_VERTEXFORMAT_FLOAT3: return 12;
		case SG_VERTEXFORMAT_FLOAT4: return 16;
		default: return 0;
	}
}

bool MetalShader::applyPipelineAndUniforms(const SokolVertexAttr *attrs, int numAttrs) {
	auto *api  = static_cast<MetalAPI *>(&g.getAPI());
	auto &impl = api->impl();
	if (impl.encoder == nil) return false;

	// ---- pipeline lookup/create (keyed on layout + blend state) ----------
	size_t key = 0;
	auto mix   = [&key](size_t v) { key = key * 1000003u ^ v; };
	for (int i = 0; i < numAttrs; i++) {
		const auto &a = attrs[i];
		mix(static_cast<size_t>(a.location));
		mix(static_cast<size_t>(a.format));
		mix(static_cast<size_t>(a.bufferSlot));
		mix(a.perInstance ? 1u : 0u);
		mix(static_cast<size_t>(a.offset));
		mix(static_cast<size_t>(a.bufferStride));
	}
	mix(static_cast<size_t>(g.getBlendMode()));
	mix(g.isBlending() ? 1u : 0u);

	id<MTLRenderPipelineState> pipeline = nil;
	if (lastPipeline != nullptr && lastPipelineKey == key) {
		// common case: same layout/blend as the previous draw with this
		// shader - no lock, no map lookup
		pipeline = (__bridge id<MTLRenderPipelineState>) lastPipeline;
	} else {
		auto &cache = ShaderCache::instance();
		std::lock_guard<std::mutex> l(cache.mut);
		auto *e = static_cast<SharedShader *>(sharedEntry);
		if (e->vertexFunc == nil || e->fragmentFunc == nil) return false;

		auto it = e->pipelines.find(key);
		if (it != e->pipelines.end()) {
			pipeline = it->second;
		} else {
			MTLRenderPipelineDescriptor *pd = [[MTLRenderPipelineDescriptor alloc] init];
			pd.vertexFunction				= e->vertexFunc;
			pd.fragmentFunction				= e->fragmentFunc;
			pd.rasterSampleCount			= mzglMetalSampleCount;

			auto *color		  = pd.colorAttachments[0];
			color.pixelFormat = impl.colorFormat;
			if (g.isBlending()) {
				color.blendingEnabled = YES;
				// mirror the Sokol pipeline's factors (alpha factors are
				// sokol's defaults: src ONE, dst ZERO)
				if (g.getBlendMode() == Graphics::BlendMode::Additive) {
					color.sourceRGBBlendFactor		= MTLBlendFactorOne;
					color.destinationRGBBlendFactor = MTLBlendFactorOne;
				} else {
					color.sourceRGBBlendFactor		= MTLBlendFactorSourceAlpha;
					color.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
				}
				color.sourceAlphaBlendFactor	  = MTLBlendFactorOne;
				color.destinationAlphaBlendFactor = MTLBlendFactorZero;
			}

			MTLVertexDescriptor *vd = [[MTLVertexDescriptor alloc] init];
			for (int i = 0; i < numAttrs; i++) {
				const auto &a						 = attrs[i];
				const int bufIdx					 = mzglMetalVertexBufferIndexBase + a.bufferSlot;
				vd.attributes[a.location].format	 = toMTLFormat(a.format);
				vd.attributes[a.location].offset	 = a.offset;
				vd.attributes[a.location].bufferIndex = bufIdx;
				vd.layouts[bufIdx].stride =
					a.bufferStride > 0 ? a.bufferStride : formatSize(a.format);
				vd.layouts[bufIdx].stepFunction =
					a.perInstance ? MTLVertexStepFunctionPerInstance : MTLVertexStepFunctionPerVertex;
			}
			pd.vertexDescriptor = vd;

			NSError *err = nil;
			pipeline	 = [impl.device newRenderPipelineStateWithDescriptor:pd error:&err];
			if (pipeline == nil) {
				Log::e() << "Metal pipeline creation failed for '" << name
						 << "': " << (err ? err.localizedDescription.UTF8String : "unknown error");
				return false;
			}
			e->pipelines[key] = pipeline;
		}
		lastPipelineKey = key;
		lastPipeline	= (__bridge void *) pipeline;
	}

	// skip the driver's state-change work when the pipeline is already
	// current (UI draws come in long runs of the same pipeline)
	if (impl.curPipeline != (__bridge void *) pipeline) {
		[impl.encoder setRenderPipelineState:pipeline];
		impl.curPipeline = (__bridge void *) pipeline;
	}

	// ---- uniforms ---------------------------------------------------------
	auto mvp = g.getMVP();
	memcpy(mvpUniformOffset, &mvp, sizeof(glm::mat4));
	if (colorUniformOffset != nullptr) {
		memcpy(colorUniformOffset, &g.getColor(), sizeof(glm::vec4));
	}

	auto setStageBytes = [&](const std::vector<uint8_t> &data, bool vertexStage) {
		if (data.empty()) return;
		if (data.size() <= mzglMetalSetBytesLimit) {
			if (vertexStage) {
				[impl.encoder setVertexBytes:data.data() length:data.size() atIndex:0];
			} else {
				[impl.encoder setFragmentBytes:data.data() length:data.size() atIndex:0];
			}
		} else {
			// rare: uniform block bigger than the setBytes limit
			id<MTLBuffer> buf = [impl.device newBufferWithBytes:data.data()
														 length:data.size()
														options:MTLResourceStorageModeShared];
			if (vertexStage) {
				[impl.encoder setVertexBuffer:buf offset:0 atIndex:0];
			} else {
				[impl.encoder setFragmentBuffer:buf offset:0 atIndex:0];
			}
		}
	};
	setStageBytes(vertUniforms, true);
	setStageBytes(fragUniforms, false);
	return true;
}
