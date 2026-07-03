#include "SokolBufferTracker.h"

// Platforms with a usable backtrace + symbolication facility. Android's bionic
// has no execinfo.h and emscripten has no native stacks, so they get the stubs
// at the bottom.
#if defined(_WIN32) || defined(__APPLE__) || (defined(__linux__) && !defined(__ANDROID__))
#	define MZGL_SOKOL_BUFFER_TRACKING 1
#endif

#ifdef MZGL_SOKOL_BUFFER_TRACKING

#	ifdef _WIN32
#		ifndef NOMINMAX
#			define NOMINMAX
#		endif
#		ifndef WIN32_LEAN_AND_MEAN
#			define WIN32_LEAN_AND_MEAN
#		endif
#		include <windows.h>
#		include <dbghelp.h>
#		pragma comment(lib, "dbghelp.lib")
#	else
#		include <execinfo.h>
#		include <cxxabi.h>
#		include <dlfcn.h>
#	endif

#	include <mutex>
#	include <unordered_map>
#	include <array>
#	include <vector>
#	include <cstdint>
#	include <cstdlib>
#	include <cstring>
#	include <cstdio>
#	include <sstream>
#	include <algorithm>

namespace SokolBufferTracker {

	namespace {
		// off by default; flip programmatically or launch with
		// MZGL_TRACK_GPU_BUFFERS=1 in the environment
		bool g_enabled = getenv("MZGL_TRACK_GPU_BUFFERS") != nullptr;
	}

	void setTrackingEnabled(bool enabled) {
		g_enabled = enabled;
	}
	bool trackingEnabled() {
		return g_enabled;
	}

	constexpr int kMaxFrames = 26;

	namespace {

		struct Stack {
			std::array<void *, kMaxFrames> frames {};
			int n = 0;
			bool operator==(const Stack &o) const {
				return n == o.n && std::memcmp(frames.data(), o.frames.data(), n * sizeof(void *)) == 0;
			}
		};

		struct StackHash {
			size_t operator()(const Stack &s) const {
				size_t h = 1469598103934665603ull;
				for (int i = 0; i < s.n; i++) {
					h ^= reinterpret_cast<size_t>(s.frames[i]);
					h *= 1099511628211ull;
				}
				return h;
			}
		};

		struct Rec {
			Stack stack;
			int size		  = 0;
			bool pooled		  = false;
			int64_t lastDrawn = -1; // frame number this buffer was last bound for a draw
			int64_t createdAt = 0;
		};

		struct State {
			std::mutex mut;
			std::unordered_map<uint32_t, Rec> live;
			int64_t frame		  = 0;
			bool listenerAttached = false;
			sg_commit_listener listener {};
		};

		State &state() {
			// Deliberately leaked: track/untrack get called from exit-time destructors
			// (e.g. a global Graphics tearing down its SokolBufferPool), which would
			// otherwise touch this after its own static destructor has run.
			static State *s = new State();
			return *s;
		}

		int captureStack(void **frames, int maxFrames) {
#	ifdef _WIN32
			return (int) RtlCaptureStackBackTrace(1, (ULONG) maxFrames, frames, nullptr);
#	else
			return backtrace(frames, maxFrames);
#	endif
		}

		std::string symbolicate(void *addr) {
#	ifdef _WIN32
			static std::once_flag symInit;
			std::call_once(symInit, [] {
				SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
				SymInitialize(GetCurrentProcess(), nullptr, TRUE);
			});
			char buf[sizeof(SYMBOL_INFO) + 512];
			auto *sym		  = reinterpret_cast<SYMBOL_INFO *>(buf);
			sym->SizeOfStruct = sizeof(SYMBOL_INFO);
			sym->MaxNameLen	  = 511;
			DWORD64 displacement = 0;
			if (SymFromAddr(GetCurrentProcess(), (DWORD64) addr, &displacement, sym)) {
				char out[600];
				snprintf(out, sizeof(out), "%s +%lld", sym->Name, (long long) displacement);
				return out;
			}
#	else
			Dl_info info;
			if (dladdr(addr, &info) && info.dli_sname != nullptr) {
				int status		= 0;
				char *demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
				std::string name = (status == 0 && demangled != nullptr) ? demangled : info.dli_sname;
				if (demangled != nullptr) free(demangled);
				char off[32];
				snprintf(off, sizeof(off), " +%ld", (long) ((uintptr_t) addr - (uintptr_t) info.dli_saddr));
				return name + off;
			}
#	endif
			char fallback[32];
			snprintf(fallback, sizeof(fallback), "%p", addr);
			return fallback;
		}

		// Bump the frame counter on every sg_commit so lastDrawn ages are meaningful.
		void ensureFrameListener() {
			auto &s = state();
			if (s.listenerAttached || !sg_isvalid()) return;
			s.listener.func		 = [](void *ud) { static_cast<State *>(ud)->frame++; };
			s.listener.user_data = &s;
			sg_add_commit_listener(s.listener);
			s.listenerAttached = true;
		}

	} // namespace

	void track(sg_buffer b, int size, bool pooled) {
		if (!g_enabled) return;
		if (b.id == 0) return;
		Rec r;
		r.size	  = size;
		r.pooled  = pooled;
		r.stack.n = captureStack(r.stack.frames.data(), kMaxFrames);
		auto &s	  = state();
		std::lock_guard<std::mutex> l(s.mut);
		r.createdAt	 = s.frame;
		s.live[b.id] = r;
	}

	void untrack(sg_buffer b) {
		if (!g_enabled) return;
		if (b.id == 0) return;
		auto &s = state();
		std::lock_guard<std::mutex> l(s.mut);
		s.live.erase(b.id);
	}

	void touch(sg_buffer b) {
		if (!g_enabled) return;
		if (b.id == 0) return;
		ensureFrameListener();
		auto &s = state();
		std::lock_guard<std::mutex> l(s.mut);
		auto it = s.live.find(b.id);
		if (it != s.live.end()) it->second.lastDrawn = s.frame;
	}

	std::string dump() {
		struct Group {
			int count	  = 0;
			int64_t bytes = 0;
			int pooled	  = 0;
			int never	  = 0; // never drawn
			int recent	  = 0; // drawn in last 60 frames
			int64_t ageSum = 0; // sum of (now - createdAt); low avg => churning every frame
			Stack stack;
		};
		std::unordered_map<Stack, Group, StackHash> groups;
		int total		   = 0;
		int64_t bytes	   = 0;
		int totalPooled	   = 0;
		int drawnLastFrame = 0, drawn60 = 0, drawn300 = 0, neverDrawn = 0;
		int64_t frameNow = 0;
		{
			auto &s = state();
			std::lock_guard<std::mutex> l(s.mut);
			total	 = (int) s.live.size();
			frameNow = s.frame;
			for (auto &[id, rec]: s.live) {
				auto &g = groups[rec.stack];
				g.stack = rec.stack;
				g.count++;
				g.bytes += rec.size;
				bytes += rec.size;
				if (rec.pooled) {
					g.pooled++;
					totalPooled++;
				}
				g.ageSum += frameNow - rec.createdAt;
				if (rec.lastDrawn < 0) {
					neverDrawn++;
					g.never++;
				} else {
					auto age = frameNow - rec.lastDrawn;
					if (age <= 1) drawnLastFrame++;
					if (age <= 60) {
						drawn60++;
						g.recent++;
					}
					if (age <= 300) drawn300++;
				}
			}
		}
		std::vector<Group> sorted;
		sorted.reserve(groups.size());
		for (auto &[k, g]: groups)
			sorted.push_back(g);
		std::sort(sorted.begin(), sorted.end(), [](const Group &a, const Group &b) {
			return a.count > b.count;
		});

		std::ostringstream out;
		out << "=== SokolBufferTracker: " << total << " live buffers, " << bytes / 1024 << " KB total, "
			<< totalPooled << " pool-owned, " << groups.size() << " unique stacks | drawn: last-frame "
			<< drawnLastFrame << ", last-60f " << drawn60 << ", last-300f " << drawn300 << ", never "
			<< neverDrawn << " (frame " << frameNow << ") ===\n\n";
		int gi = 0;
		for (auto &g: sorted) {
			out << "--- group " << gi++ << ": " << g.count << " buffers, " << g.bytes / 1024 << " KB"
				<< (g.pooled ? (", " + std::to_string(g.pooled) + " pooled") : "") << ", " << g.never
				<< " never-drawn, " << g.recent << " drawn-recently, avg-age "
				<< (g.count ? g.ageSum / g.count : 0) << "f ---\n";
			// skip frame 0 (the capture helper itself)
			for (int i = 1; i < g.stack.n; i++) {
				out << "  [" << i << "] " << g.stack.frames[i] << " " << symbolicate(g.stack.frames[i])
					<< "\n";
			}
			out << "\n";
		}
		return out.str();
	}

} // namespace SokolBufferTracker

#else // no backtrace facility on this platform - stubs

namespace SokolBufferTracker {
	void setTrackingEnabled(bool) {}
	bool trackingEnabled() {
		return false;
	}
	void track(sg_buffer, int, bool) {}
	void untrack(sg_buffer) {}
	void touch(sg_buffer) {}
	std::string dump() {
		return "SokolBufferTracker not supported on this platform\n";
	}
} // namespace SokolBufferTracker

#endif
