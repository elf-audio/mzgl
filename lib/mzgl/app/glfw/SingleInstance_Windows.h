#pragma once

#include <functional>
#include <string>
#include <vector>

/**
 * One running copy of a standalone Windows app (opt-in: MZGL_WIN_SINGLE_INSTANCE
 * in CMake, which defines MZGL_SINGLE_INSTANCE).
 *
 * Windows starts a new process every time a file associated with the app is
 * double-clicked. Without this the user ends up with two copies of the app,
 * both on the audio device and both autosaving to the same place. With it, the
 * second launch passes its file(s) to the copy that's already running, brings
 * that one to the front, and exits.
 *
 * Only the standalone runner (WinMain / GLFWAppRunner) uses this. A plug-in
 * build never calls it, so plug-in instances, and a plug-in host running next
 * to the standalone, are unaffected.
 *
 * A launch with any "-flag" on its command line is left alone: it neither
 * hands off nor listens. Test and CI instances are always started with flags,
 * and must be able to run side by side with each other and with the user's own
 * copy without receiving its files.
 */
namespace single_instance {

	// false for a launch that should stay independent (see above). args is argv, including argv[0].
	[[nodiscard]] bool appliesTo(const std::vector<std::string> &args);

	// Call first thing in WinMain. Returns true if another copy is running and
	// has been given this launch's files - the caller should then just exit.
	// Returns false for the first copy (which now owns the instance), and also
	// if the running copy couldn't be reached, so a hung one can't stop the
	// app from starting.
	[[nodiscard]] bool handOffToRunningInstance(const std::vector<std::string> &args);

	// Call once the main window exists, on the thread that pumps its messages.
	// onFile is called on that thread with the UTF-8 path of each file a later
	// launch was asked to open. nativeWindow is the HWND to bring forward.
	void listen(void *nativeWindow, std::function<void(const std::string &)> onFile);

} // namespace single_instance
