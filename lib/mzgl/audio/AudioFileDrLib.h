//
//  AudioFileDrLib.h
//  mzgl
//
//  Platform-neutral audio load via dr_libs (dr_wav / dr_flac / dr_mp3) +
//  the AIFF reader. This is the path that Linux, Windows, and Android
//  use in production. Exposed on every platform so Apple builds can
//  exercise it from tests against the same files.
//

#pragma once

#include "AudioFile.h"

#include <optional>
#include <string>

namespace AudioFile {
	// Stream a file via dr_libs (and the AIFF reader) into a LoadedAudio.
	// Returns false and adds an issue to `out.result` if the reader can't
	// open the file. May throw std::bad_alloc / std::filesystem_error
	// from internal allocations or path handling — callers that need
	// noexcept behaviour should wrap (see loadViaDrLib below).
	bool drLibStreamLoad(const std::string &path,
						 LoadedAudio &out,
						 std::optional<int> resampleTo);
} // namespace AudioFile
