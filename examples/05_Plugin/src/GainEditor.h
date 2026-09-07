#pragma once

// The editor: an mzgl App (a PluginEditor) that draws one vertical fader for
// the gain parameter. Used unchanged by every format - each wrapper creates it
// through instantiatePluginEditor() and hosts its view in the DAW's window.
//
// Parameter traffic:
//   UI -> host: effect.updateHostParameter(i, v)  (also sets the value)
//   host -> UI: just read the parameter each frame; the wrappers already wrote
//               host automation into it (Plugin::hostUpdatedParameter).
// While the user drags, beginIgnoringAutomation() stops incoming automation
// from fighting the finger.

#include "PluginEditor.h"
#include "GainPlugin.h"

#include <cstdio>
#include <string>

class GainEditor : public PluginEditor {
public:
	static constexpr int kDefaultWidth	= 240;
	static constexpr int kDefaultHeight = 360;

	GainEditor(Graphics &g, std::shared_ptr<Plugin> plugin)
		: PluginEditor(g, *plugin)
		, plugin(plugin) {}

	std::pair<int, int> getPreferredDimensions() const override { return {kDefaultWidth, kDefaultHeight}; }

	void draw() override {
		g.clear(0.12f, 0.12f, 0.14f);

		const Rectf track = trackRect();
		g.setColor(0.25f);
		g.fill();
		g.drawRoundedRect(track, track.width * 0.5f);

		const float norm = normalised();
		Rectf level		 = track;
		level.height	 = track.height * norm;
		level.y			 = track.bottom() - level.height;
		g.setColor(0.95f, 0.55f, 0.15f);
		g.drawRoundedRect(level, level.width * 0.5f);

		g.setColor(1.f);
		char buf[32];
		std::snprintf(buf, sizeof(buf), "%+.1f dB", gainParam()->get());
		g.drawTextCentred(buf, {g.width * 0.5f, track.y - 20.f * g.pixelScale});
		g.drawTextCentred("GAIN", {g.width * 0.5f, track.bottom() + 24.f * g.pixelScale});
	}

	void touchDown(float x, float y, int id) override {
		Rectf hit = trackRect();
		const float pad = 20.f * g.pixelScale;
		hit.x -= pad;
		hit.width += pad * 2.f;
		if (!hit.inside(x, y)) return;
		dragging = true;
		gainParam()->beginIgnoringAutomation();
		setFromY(y);
	}

	void touchMoved(float x, float y, int id) override {
		if (dragging) setFromY(y);
	}

	void touchUp(float x, float y, int id) override {
		if (!dragging) return;
		dragging = false;
		gainParam()->endIgnoringAutomation();
	}

private:
	std::shared_ptr<Plugin> plugin;
	bool dragging = false;

	std::shared_ptr<PluginParameter> gainParam() const { return plugin->getParam(GainPlugin::kGain); }

	float normalised() const {
		auto p = gainParam();
		return (p->get() - p->from) / (p->to - p->from);
	}

	Rectf trackRect() const {
		// g.width/g.height are in pixels; sizes are in points * pixelScale.
		const float w = 28.f * g.pixelScale;
		const float h = g.height - 120.f * g.pixelScale;
		return {(g.width - w) * 0.5f, 60.f * g.pixelScale, w, h};
	}

	void setFromY(float y) {
		const Rectf track = trackRect();
		float norm		  = 1.f - (y - track.y) / track.height;
		norm			  = norm < 0.f ? 0.f : (norm > 1.f ? 1.f : norm);
		auto p			  = gainParam();
		effect.updateHostParameter(GainPlugin::kGain, p->from + norm * (p->to - p->from));
	}
};
