#pragma once

#if defined(__clang__)
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif
#include "pluginterfaces/gui/iplugview.h"
#include "base/source/fobject.h"
#if defined(__clang__)
#	pragma clang diagnostic pop
#endif

#include <memory>

class Plugin;
class PluginEditor;
class Graphics;
class EventDispatcher;

class MZGLEffectVST3;

class MZGLEffectVST3View
	: public Steinberg::FObject
	, public Steinberg::IPlugView {
public:
	explicit MZGLEffectVST3View(MZGLEffectVST3 *processor);
	~MZGLEffectVST3View() override;

	Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override;
	Steinberg::uint32 PLUGIN_API addRef() override { return FObject::addRef(); }
	Steinberg::uint32 PLUGIN_API release() override { return FObject::release(); }

	Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) override;
	Steinberg::tresult PLUGIN_API attached(void *parent, Steinberg::FIDString type) override;
	Steinberg::tresult PLUGIN_API removed() override;

	Steinberg::tresult PLUGIN_API onWheel(float distance) override;
	Steinberg::tresult PLUGIN_API onKeyDown(Steinberg::char16 key,
											Steinberg::int16 keyCode,
											Steinberg::int16 modifiers) override;
	Steinberg::tresult PLUGIN_API onKeyUp(Steinberg::char16 key,
										  Steinberg::int16 keyCode,
										  Steinberg::int16 modifiers) override;
	Steinberg::tresult PLUGIN_API onFocus(Steinberg::TBool state) override;
	Steinberg::tresult PLUGIN_API setFrame(Steinberg::IPlugFrame *frame) override;

	Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect *newSize) override;
	Steinberg::tresult PLUGIN_API getSize(Steinberg::ViewRect *size) override;

	Steinberg::tresult PLUGIN_API canResize() override;
	Steinberg::tresult PLUGIN_API checkSizeConstraint(Steinberg::ViewRect *rect) override;

protected:
	MZGLEffectVST3 *processor		 = nullptr;
	Steinberg::IPlugFrame *plugFrame = nullptr;
	Steinberg::ViewRect rect;

	std::shared_ptr<Graphics> graphics;
	std::shared_ptr<PluginEditor> pluginEditor;
	std::shared_ptr<EventDispatcher> eventDispatcher;

	void *nativeView = nullptr;

	static constexpr int kDefaultWidth	= 320;
	static constexpr int kDefaultHeight = 568;

	int viewWidth  = kDefaultWidth;
	int viewHeight = kDefaultHeight;
};