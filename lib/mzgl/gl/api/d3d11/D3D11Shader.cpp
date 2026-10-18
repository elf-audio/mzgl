#include "D3D11Shader.h"
#include "D3D11API.h"
#include "D3D11Internal.h"
#include "Graphics.h"
#include "mzAssert.h"
#include <d3dcompiler.h>
#include <mutex>
#include <map>
#include <sstream>

namespace {
	// Process-global cache of compiled shaders and their input layouts,
	// shared by every Graphics/plugin instance in the process (mirrors the
	// SokolShader cache). Keyed by the ShaderDef's descFunc pointer, which
	// uniquely identifies the compiled-in shader code independent of
	// registry names. Per-draw uniform state lives in each D3D11Shader, so N
	// instances share one compile, one pair of shader objects and one
	// constant buffer per stage.
	struct SharedShader {
		const sg_shader_desc *desc = nullptr;
		ID3D11VertexShader *vs	   = nullptr;
		ID3D11PixelShader *ps	   = nullptr;
		ID3DBlob *vsBlob		   = nullptr; // kept: CreateInputLayout needs the VS signature
		ID3D11Buffer *vsConstants  = nullptr;
		ID3D11Buffer *psConstants  = nullptr;
		size_t vsConstantBytes	   = 0;
		size_t psConstantBytes	   = 0;
		uint32_t generation		   = 0; // device generation the objects were created for
		bool compileFailed		   = false;
		int refs				   = 0;
		std::map<size_t, ID3D11InputLayout *> layouts;

		void release() {
			for (auto &kv: layouts)
				mzglD3D11Release(kv.second);
			layouts.clear();
			mzglD3D11Release(vs);
			mzglD3D11Release(ps);
			mzglD3D11Release(vsBlob);
			mzglD3D11Release(vsConstants);
			mzglD3D11Release(psConstants);
			compileFailed = false;
		}
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

	ID3DBlob *compileHLSL(const sg_shader_stage_desc &stage, const char *defaultTarget, const std::string &name) {
		if (stage.source == nullptr) {
			Log::e() << "no HLSL source for shader '" << name
					 << "' - were the shaders compiled with the hlsl5 slang variant?";
			return nullptr;
		}
		ID3DBlob *code	 = nullptr;
		ID3DBlob *errors = nullptr;
		// same flags as sokol's D3D11 backend, so the generated code matches
		const HRESULT hr = D3DCompile(stage.source,
									  strlen(stage.source),
									  name.c_str(),
									  nullptr,
									  nullptr,
									  stage.entry ? stage.entry : "main",
									  stage.d3d11_target ? stage.d3d11_target : defaultTarget,
									  D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3,
									  0,
									  &code,
									  &errors);
		if (errors != nullptr) {
			const std::string msg((const char *) errors->GetBufferPointer(), errors->GetBufferSize());
			if (FAILED(hr)) {
				Log::e() << "HLSL compile failed for '" << name << "': " << msg;
			} else {
				Log::d() << "HLSL compile warnings for '" << name << "': " << msg;
			}
			errors->Release();
		}
		if (FAILED(hr)) {
			mzglD3D11Check(hr, "D3DCompile");
			mzglD3D11Release(code);
			return nullptr;
		}
		return code;
	}

	ID3D11Buffer *makeConstantBuffer(ID3D11Device *dev, size_t bytes, const std::string &name) {
		if (bytes == 0) return nullptr;
		D3D11_BUFFER_DESC d = {};
		d.ByteWidth			= (UINT) ((bytes + 15) & ~(size_t) 15);
		d.Usage				= D3D11_USAGE_DYNAMIC;
		d.BindFlags			= D3D11_BIND_CONSTANT_BUFFER;
		d.CPUAccessFlags	= D3D11_CPU_ACCESS_WRITE;
		ID3D11Buffer *buf	= nullptr;
		if (!mzglD3D11Check(dev->CreateBuffer(&d, nullptr, &buf), "CreateBuffer(constants)")) {
			Log::e() << "  for shader '" << name << "'";
			return nullptr;
		}
		return buf;
	}

	// caller holds the cache lock. (Re)compiles the entry for the current
	// device generation; returns true when the shader objects are usable.
	bool ensureCompiled(SharedShader &e, const std::string &name) {
		ID3D11Device *dev = mzglD3D11::device();
		if (dev == nullptr) return false;
		const uint32_t gen = mzglD3D11::generation();
		if (e.generation == gen) return e.vs != nullptr && e.ps != nullptr;
		e.release();
		e.generation = gen;

		ID3DBlob *vsBlob = compileHLSL(e.desc->vs, "vs_5_0", name);
		ID3DBlob *psBlob = compileHLSL(e.desc->fs, "ps_5_0", name);
		bool ok			 = vsBlob != nullptr && psBlob != nullptr;
		if (ok) {
			ok = mzglD3D11Check(
					 dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &e.vs),
					 "CreateVertexShader")
				 && mzglD3D11Check(
					 dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &e.ps),
					 "CreatePixelShader");
		}
		mzglD3D11Release(psBlob);
		if (!ok) {
			mzglD3D11Release(vsBlob);
			e.release();
			e.compileFailed = true;
			Log::e() << "D3D11 shader '" << name << "' is unusable - its draws will be skipped";
			return false;
		}
		e.vsBlob	  = vsBlob;
		e.vsConstants = makeConstantBuffer(dev, e.vsConstantBytes, name);
		e.psConstants = makeConstantBuffer(dev, e.psConstantBytes, name);
		return true;
	}

	DXGI_FORMAT toDXGI(sg_vertex_format f) {
		switch (f) {
			case SG_VERTEXFORMAT_FLOAT: return DXGI_FORMAT_R32_FLOAT;
			case SG_VERTEXFORMAT_FLOAT2: return DXGI_FORMAT_R32G32_FLOAT;
			case SG_VERTEXFORMAT_FLOAT3: return DXGI_FORMAT_R32G32B32_FLOAT;
			case SG_VERTEXFORMAT_FLOAT4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
			default: return DXGI_FORMAT_UNKNOWN;
		}
	}

	void uploadConstants(ID3D11DeviceContext *dc, ID3D11Buffer *buf, const std::vector<uint8_t> &data) {
		if (buf == nullptr || data.empty()) return;
		D3D11_MAPPED_SUBRESOURCE mapped = {};
		if (!mzglD3D11Check(dc->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped), "Map(constants)")) return;
		memcpy(mapped.pData, data.data(), data.size());
		dc->Unmap(buf, 0);
	}
} // namespace

D3D11Shader::D3D11Shader(Graphics &g, const std::string &shaderName)
	: Shader(g) {
	name	  = shaderName;
	auto *api = static_cast<D3D11API *>(&g.getAPI());
	def		  = api->getShaderDef(shaderName);
	mzAssert(def != nullptr, ("no shader registered with the name '" + shaderName + "'").c_str());

	vertUniforms.resize(def->uniformblockSizeFunc(SG_SHADERSTAGE_VS, "vertParams"), 0);
	fragUniforms.resize(def->uniformblockSizeFunc(SG_SHADERSTAGE_FS, "fragParams"), 0);

	{
		auto &cache = ShaderCache::instance();
		std::lock_guard<std::mutex> l(cache.mut);
		// std::map nodes are address-stable, so the entry pointer stays valid
		// for this shader's lifetime (we hold a ref until our destructor)
		auto &e		= cache.entries[def->descFunc];
		sharedEntry = &e;
		if (e.refs == 0) {
			e.desc			  = def->descFunc(SG_BACKEND_D3D11);
			e.vsConstantBytes = vertUniforms.size();
			e.psConstantBytes = fragUniforms.size();
		}
		e.refs++;
		// compile now if there's a device; otherwise the first draw does it
		if (mzglD3D11::device() != nullptr) ensureCompiled(e, name);
	}

	colorUniformOffset = uniformLocation("color");
	mvpUniformOffset   = uniformLocation("mvp");
	mzAssert(mvpUniformOffset != nullptr, "mvp uniform not found in shader");
}

D3D11Shader::~D3D11Shader() {
	auto &cache = ShaderCache::instance();
	std::lock_guard<std::mutex> l(cache.mut);
	auto *e = static_cast<SharedShader *>(sharedEntry);
	if (e == nullptr) return;
	if (--e->refs <= 0) {
		e->release();
		cache.entries.erase(def->descFunc);
	}
}

void D3D11Shader::begin() {
	g.currShader = this;
}
void D3D11Shader::end() {
	g.currShader = nullptr;
}

int D3D11Shader::attrSlot(const std::string &attrName) const {
	return def->attrSlotFunc(attrName.c_str());
}

uint8_t *D3D11Shader::uniformLocation(const std::string &uname) {
	int offset = def->uniformOffsetFunc(SG_SHADERSTAGE_VS, "vertParams", uname.c_str());
	if (offset != -1) return vertUniforms.data() + offset;
	offset = def->uniformOffsetFunc(SG_SHADERSTAGE_FS, "fragParams", uname.c_str());
	if (offset != -1) return fragUniforms.data() + offset;
	return nullptr;
}

bool D3D11Shader::applyPipelineAndUniforms(const SokolVertexAttr *attrs, int numAttrs) {
	auto *api = static_cast<D3D11API *>(&g.getAPI());
	if (!api->inFrame()) return false;
	auto &s	 = mzglD3D11::state();
	auto *e	 = static_cast<SharedShader *>(sharedEntry);
	auto *dc = s.context;

	// ---- compiled objects for the current device --------------------------
	if (e->generation != s.generation || e->vs == nullptr) {
		auto &cache = ShaderCache::instance();
		std::lock_guard<std::mutex> l(cache.mut);
		if (!ensureCompiled(*e, name)) return false;
		lastLayout = nullptr;
	}

	// ---- input layout lookup/create (keyed on the attribute layout) -------
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
	mix(static_cast<size_t>(numAttrs));

	ID3D11InputLayout *layout = nullptr;
	if (lastLayout != nullptr && lastLayoutKey == key && lastLayoutGeneration == e->generation) {
		// common case: same layout as the previous draw with this shader -
		// no lock, no map lookup
		layout = static_cast<ID3D11InputLayout *>(lastLayout);
	} else {
		auto &cache = ShaderCache::instance();
		std::lock_guard<std::mutex> l(cache.mut);
		auto it = e->layouts.find(key);
		if (it != e->layouts.end()) {
			layout = it->second;
		} else {
			D3D11_INPUT_ELEMENT_DESC elems[SG_MAX_VERTEX_ATTRIBUTES] = {};
			for (int i = 0; i < numAttrs; i++) {
				const auto &a  = attrs[i];
				const auto &ad = e->desc->attrs[a.location];
				auto &el	   = elems[i];
				// sokol-shdc's HLSL declares every vertex input as TEXCOORDn
				el.SemanticName		 = ad.sem_name ? ad.sem_name : "TEXCOORD";
				el.SemanticIndex	 = (UINT) ad.sem_index;
				el.Format			 = toDXGI(a.format);
				el.InputSlot		 = (UINT) a.bufferSlot;
				el.AlignedByteOffset = (UINT) a.offset;
				el.InputSlotClass	 = a.perInstance ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
				el.InstanceDataStepRate = a.perInstance ? 1u : 0u;
			}
			const HRESULT hr = s.device->CreateInputLayout(
				elems, (UINT) numAttrs, e->vsBlob->GetBufferPointer(), e->vsBlob->GetBufferSize(), &layout);
			if (FAILED(hr) || layout == nullptr) {
				mzglD3D11Check(hr, "CreateInputLayout");
				Log::e() << "  for shader '" << name << "' - the Vbo's attributes don't match the shader's inputs";
				// cache the failure so a misbehaving draw doesn't retry every frame
				e->layouts[key] = nullptr;
				return false;
			}
			e->layouts[key] = layout;
		}
		if (layout == nullptr) return false; // cached failure
		lastLayoutKey		 = key;
		lastLayout			 = layout;
		lastLayoutGeneration = e->generation;
	}

	// ---- pipeline state, memoised on the immediate context ----------------
	auto &m = s.memo;
	if (m.layout != layout) {
		dc->IASetInputLayout(layout);
		m.layout = layout;
	}
	if (m.vs != e->vs) {
		dc->VSSetShader(e->vs, nullptr, 0);
		m.vs = e->vs;
	}
	if (m.ps != e->ps) {
		dc->PSSetShader(e->ps, nullptr, 0);
		m.ps = e->ps;
	}
	if (m.vsConstants != e->vsConstants) {
		dc->VSSetConstantBuffers(0, 1, &e->vsConstants);
		m.vsConstants = e->vsConstants;
	}
	if (m.psConstants != e->psConstants) {
		dc->PSSetConstantBuffers(0, 1, &e->psConstants);
		m.psConstants = e->psConstants;
	}
	ID3D11BlendState *blend = mzglD3D11::blendState(g.isBlending(), g.getBlendMode());
	if (m.blend != blend) {
		dc->OMSetBlendState(blend, nullptr, 0xffffffffu);
		m.blend = blend;
	}

	// ---- uniforms ---------------------------------------------------------
	auto mvp = g.getMVP();
	memcpy(mvpUniformOffset, &mvp, sizeof(glm::mat4));
	if (colorUniformOffset != nullptr) {
		memcpy(colorUniformOffset, &g.getColor(), sizeof(glm::vec4));
	}
	uploadConstants(dc, e->vsConstants, vertUniforms);
	uploadConstants(dc, e->psConstants, fragUniforms);

	if (s.traceThisFrame) {
		const auto &c = g.getColor();
		std::stringstream out;
		out << "[D3D11 trace] draw shader=" << name << " blend=" << (g.isBlending() ? "on" : "off") << "/"
			<< (int) g.getBlendMode() << " color=(" << c.r << "," << c.g << "," << c.b << "," << c.a
			<< ") tex=" << api->getBoundTexture() << " mask=" << (api->isMaskOn() ? "on" : "off") << " attrs=";
		for (int i = 0; i < numAttrs; i++) {
			out << attrs[i].location << ":" << (int) attrs[i].format << "@" << attrs[i].bufferSlot << " ";
		}
		Log::d() << out.str();
	}
	return true;
}
