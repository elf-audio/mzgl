#include "WaylandTouch.h"

#define GLFW_EXPOSE_NATIVE_WAYLAND
#include "glfw3.h"
#include <GLFW/glfw3native.h>
#include <wayland-client.h>

#include <algorithm>
#include <cstring>
#include <map>
#include <vector>

struct Seat {
	WaylandTouch::Impl *impl {nullptr};
	uint32_t name {0};
	wl_seat *seat {nullptr};
	wl_touch *touch {nullptr};
};

struct WaylandTouch::Impl {
	wl_surface *surface {nullptr};
	wl_registry *registry {nullptr};
	Callback callback;
	std::vector<std::unique_ptr<Seat>> seats;

	struct Point {
		double x;
		double y;
	};
	// only touches that started on our surface
	std::map<int32_t, Point> active;

	~Impl() {
		// no callbacks from here: the app may already be gone
		for (auto &s: seats) {
			destroySeat(*s);
		}
		if (registry != nullptr) wl_registry_destroy(registry);
	}

	void cancelAll() {
		// mzgl has no touch-cancel event, so end each touch where it last was
		auto touches = std::move(active);
		active.clear();
		for (auto &[id, p]: touches) {
			callback(Phase::up, id, p.x, p.y);
		}
	}

	static void destroySeat(Seat &s) {
		if (s.touch != nullptr) {
			if (wl_touch_get_version(s.touch) >= WL_TOUCH_RELEASE_SINCE_VERSION) {
				wl_touch_release(s.touch);
			} else {
				wl_touch_destroy(s.touch);
			}
			s.touch = nullptr;
		}
		if (s.seat != nullptr) {
			if (wl_seat_get_version(s.seat) >= WL_SEAT_RELEASE_SINCE_VERSION) {
				wl_seat_release(s.seat);
			} else {
				wl_seat_destroy(s.seat);
			}
			s.seat = nullptr;
		}
	}
};

namespace {
	using Impl = WaylandTouch::Impl;

	void touchDown(
		void *data, wl_touch *, uint32_t, uint32_t, wl_surface *surface, int32_t id, wl_fixed_t x, wl_fixed_t y) {
		auto *impl = static_cast<Seat *>(data)->impl;
		if (surface != impl->surface) return; // e.g. a decoration surface
		Impl::Point p {wl_fixed_to_double(x), wl_fixed_to_double(y)};
		impl->active[id] = p;
		impl->callback(WaylandTouch::Phase::down, id, p.x, p.y);
	}

	void touchUp(void *data, wl_touch *, uint32_t, uint32_t, int32_t id) {
		auto *impl = static_cast<Seat *>(data)->impl;
		auto it	   = impl->active.find(id);
		if (it == impl->active.end()) return;
		auto p = it->second;
		impl->active.erase(it);
		impl->callback(WaylandTouch::Phase::up, id, p.x, p.y);
	}

	void touchMotion(void *data, wl_touch *, uint32_t, int32_t id, wl_fixed_t x, wl_fixed_t y) {
		auto *impl = static_cast<Seat *>(data)->impl;
		auto it	   = impl->active.find(id);
		if (it == impl->active.end()) return;
		it->second = {wl_fixed_to_double(x), wl_fixed_to_double(y)};
		impl->callback(WaylandTouch::Phase::moved, id, it->second.x, it->second.y);
	}

	void touchFrame(void *, wl_touch *) {
	}

	void touchCancel(void *data, wl_touch *) {
		static_cast<Seat *>(data)->impl->cancelAll();
	}

	void touchShape(void *, wl_touch *, int32_t, wl_fixed_t, wl_fixed_t) {
	}
	void touchOrientation(void *, wl_touch *, int32_t, wl_fixed_t) {
	}

	const wl_touch_listener touchListener {
		touchDown, touchUp, touchMotion, touchFrame, touchCancel, touchShape, touchOrientation};

	void seatCapabilities(void *data, wl_seat *seat, uint32_t caps) {
		auto *s		   = static_cast<Seat *>(data);
		const bool has = (caps & WL_SEAT_CAPABILITY_TOUCH) != 0;
		if (has && s->touch == nullptr) {
			s->touch = wl_seat_get_touch(seat);
			wl_touch_add_listener(s->touch, &touchListener, s);
		} else if (!has && s->touch != nullptr) {
			s->impl->cancelAll();
			wl_touch_destroy(s->touch);
			s->touch = nullptr;
		}
	}

	void seatName(void *, wl_seat *, const char *) {
	}

	const wl_seat_listener seatListener {seatCapabilities, seatName};

	void
		registryGlobal(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
		if (strcmp(interface, wl_seat_interface.name) != 0) return;
		auto *impl = static_cast<Impl *>(data);
		auto s	   = std::make_unique<Seat>();
		s->impl	   = impl;
		s->name	   = name;
		// v5 has wl_touch.release; shape/orientation (v6) aren't needed
		s->seat = static_cast<wl_seat *>(
			wl_registry_bind(registry, name, &wl_seat_interface, std::min<uint32_t>(version, 5)));
		wl_seat_add_listener(s->seat, &seatListener, s.get());
		impl->seats.push_back(std::move(s));
	}

	void registryGlobalRemove(void *data, wl_registry *, uint32_t name) {
		auto *impl = static_cast<Impl *>(data);
		auto it =
			std::find_if(impl->seats.begin(), impl->seats.end(), [name](auto &s) { return s->name == name; });
		if (it == impl->seats.end()) return;
		if ((*it)->touch != nullptr) impl->cancelAll();
		Impl::destroySeat(**it);
		impl->seats.erase(it);
	}

	const wl_registry_listener registryListener {registryGlobal, registryGlobalRemove};
} // namespace

auto WaylandTouch::create(GLFWwindow *window, Callback callback) -> std::unique_ptr<WaylandTouch> {
	if (glfwGetPlatform() != GLFW_PLATFORM_WAYLAND) return nullptr;
	auto *display = glfwGetWaylandDisplay();
	auto *surface = glfwGetWaylandWindow(window);
	if (display == nullptr || surface == nullptr) return nullptr;

	auto impl	   = std::make_unique<Impl>();
	impl->surface  = surface;
	impl->callback = std::move(callback);
	impl->registry = wl_display_get_registry(display);
	wl_registry_add_listener(impl->registry, &registryListener, impl.get());
	// the seat globals arrive on GLFW's next event dispatch
	return std::unique_ptr<WaylandTouch>(new WaylandTouch(std::move(impl)));
}

WaylandTouch::WaylandTouch(std::unique_ptr<Impl> impl)
	: impl(std::move(impl)) {
}

WaylandTouch::~WaylandTouch() = default;
