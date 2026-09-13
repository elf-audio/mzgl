//
//  ZipFile.cpp
//  Koala Sampler
//
//  Created by Marek Bereza on 06/01/2020.
//  Copyright © 2020 Marek Bereza. All rights reserved.
//
//  Thin wrapper over miniz (lib/miniz). All file I/O goes through FILE* /
//  fstreams that we open ourselves so paths work the same way (incl. wide
//  paths on Windows) as the rest of the codebase.
//

#include "ZipFile.h"
#include "ZipIO.h"
#include "miniz.h"
#include "mzgl/util/log.h"

#include <algorithm>
#include <cstring>
#include <ctime>
#include <sys/stat.h>

namespace {

mz_uint compressionLevel(ZipFile::Compression compression) {
	switch (compression) {
		case ZipFile::Compression::NONE: return MZ_NO_COMPRESSION;
		case ZipFile::Compression::FASTER: return MZ_BEST_SPEED;
		case ZipFile::Compression::SMALLER: return MZ_BEST_COMPRESSION;
	}
	return MZ_BEST_COMPRESSION;
}

std::string lastError(mz_zip_archive &zip) {
	return mz_zip_get_error_string(mz_zip_get_last_error(&zip));
}

// Modification time of a file, or nullopt if it can't be read. Passed through
// to the zip entry so extracted files keep their timestamps.
bool fileModTime(const fs::path &path, time_t &outTime) {
#ifdef _WIN32
	struct _stat64 st;
	if (_wstat64(path.wstring().c_str(), &st) != 0) return false;
#else
	struct stat st;
	if (stat(path.string().c_str(), &st) != 0) return false;
#endif
	outTime = st.st_mtime;
	return true;
}

// ---------------------------------------------------------------------------
// Writing
// ---------------------------------------------------------------------------

struct Writer {
	mz_zip_archive zip;
	bool initialised = false;

	Writer() { mz_zip_zero_struct(&zip); }
	~Writer() {
		if (initialised) mz_zip_writer_end(&zip);
	}
	Writer(const Writer &)			  = delete;
	Writer &operator=(const Writer &) = delete;

	bool addFile(const fs::path &filePath, const std::string &nameInZip, mz_uint level) {
		FILE *f = zipio::openFile(filePath, "rb");
		if (f == nullptr) {
			Log::e() << "ZipFile: can't open '" << filePath.string() << "' for reading";
			return false;
		}

		std::error_code ec;
		auto size = fs::file_size(filePath, ec);
		if (ec) {
			Log::e() << "ZipFile: can't stat '" << filePath.string() << "': " << ec.message();
			fclose(f);
			return false;
		}

		time_t modTime		   = 0;
		const time_t *modTimeP = fileModTime(filePath, modTime) ? &modTime : nullptr;

		bool ok = mz_zip_writer_add_cfile(
			&zip, nameInZip.c_str(), f, size, modTimeP, nullptr, 0, level, nullptr, 0, nullptr, 0);
		fclose(f);
		if (!ok) {
			Log::e() << "ZipFile: failed to add '" << nameInZip << "': " << lastError(zip);
		}
		return ok;
	}

	// Adds every file under dir (recursively). Entry names are relative to
	// dir, use '/' separators and there are no explicit directory entries -
	// this matches what the previous (zipper) implementation produced.
	bool addDirectory(const fs::path &dir, const std::string &prefix, mz_uint level) {
		std::error_code ec;
		for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
			fs::path childPath = it->path();
			std::string name   = prefix + childPath.filename().string();
			if (it->is_directory()) {
				if (!addDirectory(childPath, name + "/", level)) return false;
			} else if (it->is_regular_file()) {
				if (!addFile(childPath, name, level)) return false;
			}
		}
		if (ec) {
			Log::e() << "ZipFile: error iterating '" << dir.string() << "': " << ec.message();
			return false;
		}
		return true;
	}

	bool finalize() {
		if (!mz_zip_writer_finalize_archive(&zip)) {
			Log::e() << "ZipFile: failed to finalize archive: " << lastError(zip);
			return false;
		}
		return true;
	}
};

// Write callback that appends into a std::vector - miniz writes local headers
// as zero placeholders and later seeks back to fill them in, so writes may land
// at any offset <= current size.
size_t vectorWrite(void *opaque, mz_uint64 fileOfs, const void *buf, size_t n) {
	auto &out	 = *static_cast<std::vector<unsigned char> *>(opaque);
	auto endOfs = static_cast<size_t>(fileOfs) + n;
	if (endOfs > out.size()) {
		if (endOfs > out.capacity()) {
			out.reserve(std::max(endOfs, out.capacity() * 2));
		}
		out.resize(endOfs);
	}
	std::memcpy(out.data() + fileOfs, buf, n);
	return n;
}

bool isValidSourceDir(const fs::path &dirToZip) {
	if (!fs::is_directory(dirToZip)) {
		Log::e() << "ZipFile: '" << dirToZip.string() << "' is not a directory";
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// Reading
// ---------------------------------------------------------------------------

struct Reader {
	mz_zip_archive zip;
	FILE *file		 = nullptr;
	bool initialised = false;

	Reader() { mz_zip_zero_struct(&zip); }
	~Reader() {
		if (initialised) mz_zip_reader_end(&zip);
		if (file != nullptr) fclose(file);
	}
	Reader(const Reader &)			  = delete;
	Reader &operator=(const Reader &) = delete;

	bool open(const fs::path &zipPath) {
		file = zipio::openFile(zipPath, "rb");
		if (file == nullptr) {
			Log::e() << "ZipFile: can't open '" << zipPath.string() << "' for reading";
			return false;
		}
		if (!mz_zip_reader_init_cfile(&zip, file, 0, 0)) {
			Log::e() << "ZipFile: '" << zipPath.string() << "' is not a readable zip: " << lastError(zip);
			return false;
		}
		initialised = true;
		return true;
	}

	bool open(const std::vector<unsigned char> &data) {
		if (!mz_zip_reader_init_mem(&zip, data.data(), data.size(), 0)) {
			Log::e() << "ZipFile: in-memory data is not a readable zip: " << lastError(zip);
			return false;
		}
		initialised = true;
		return true;
	}

	mz_uint numEntries() { return mz_zip_reader_get_num_files(&zip); }

	// Full entry name (mz_zip_archive_file_stat::m_filename truncates long names).
	std::string entryName(mz_uint index) {
		auto len = mz_zip_reader_get_filename(&zip, index, nullptr, 0);
		std::string name(len, '\0');
		if (len > 0) {
			mz_zip_reader_get_filename(&zip, index, name.data(), len);
			name.resize(len - 1); // drop the terminating NUL
		}
		return name;
	}

	std::vector<std::string> list() {
		std::vector<std::string> names;
		auto n = numEntries();
		names.reserve(n);
		for (mz_uint i = 0; i < n; i++) {
			names.push_back(entryName(i));
		}
		return names;
	}

	// Returns the index of the entry or -1. Case sensitive, exact match.
	int find(const std::string &nameInZip) {
		return mz_zip_reader_locate_file(&zip, nameInZip.c_str(), nullptr, MZ_ZIP_FLAG_CASE_SENSITIVE);
	}

	// Extracts an entry straight into outData (resized to fit).
	template <typename Container>
	bool extractToContainer(const std::string &nameInZip, Container &outData) {
		int index = find(nameInZip);
		if (index < 0) return false;

		mz_zip_archive_file_stat st;
		if (!mz_zip_reader_file_stat(&zip, static_cast<mz_uint>(index), &st)) {
			Log::e() << "ZipFile: can't stat '" << nameInZip << "': " << lastError(zip);
			return false;
		}
		outData.resize(static_cast<size_t>(st.m_uncomp_size));
		if (st.m_uncomp_size == 0) return true;

		if (!mz_zip_reader_extract_to_mem(
				&zip, static_cast<mz_uint>(index), outData.data(), outData.size(), 0)) {
			Log::e() << "ZipFile: failed to extract '" << nameInZip << "': " << lastError(zip);
			outData.clear();
			return false;
		}
		return true;
	}

	static size_t streamWrite(void *opaque, mz_uint64 /*fileOfs*/, const void *buf, size_t n) {
		auto &out = *static_cast<std::ofstream *>(opaque);
		out.write(static_cast<const char *>(buf), static_cast<std::streamsize>(n));
		return out.good() ? n : 0;
	}

	// Refuse entry names that would escape outDir.
	static bool isSafeEntryName(const std::string &name) {
		if (name.empty()) return false;
		if (name[0] == '/' || name[0] == '\\') return false;
		if (name.size() > 1 && name[1] == ':') return false; // C:...
		size_t pos = 0;
		while (pos <= name.size()) {
			auto next = name.find_first_of("/\\", pos);
			if (next == std::string::npos) next = name.size();
			if (name.compare(pos, next - pos, "..") == 0) return false;
			pos = next + 1;
		}
		return true;
	}

	bool extractAll(const fs::path &outDir) {
		auto n = numEntries();
		for (mz_uint i = 0; i < n; i++) {
			auto name = entryName(i);
			if (!isSafeEntryName(name)) {
				Log::e() << "ZipFile: refusing to extract unsafe entry name '" << name << "'";
				return false;
			}
			fs::path outPath = outDir / name;

			std::error_code ec;
			if (mz_zip_reader_is_file_a_directory(&zip, i) || name.back() == '/') {
				fs::create_directories(outPath, ec);
				if (ec) {
					Log::e() << "ZipFile: can't create directory '" << outPath.string() << "': " << ec.message();
					return false;
				}
				continue;
			}

			auto parent = outPath.parent_path();
			if (!parent.empty()) {
				fs::create_directories(parent, ec);
				if (ec) {
					Log::e() << "ZipFile: can't create directory '" << parent.string() << "': " << ec.message();
					return false;
				}
			}

			std::ofstream out = zipio::openOutput(outPath);
			if (!out.good()) {
				Log::e() << "ZipFile: can't open '" << outPath.string() << "' for writing";
				return false;
			}
			if (!mz_zip_reader_extract_to_callback(&zip, i, streamWrite, &out, 0)) {
				Log::e() << "ZipFile: failed to extract '" << name << "': " << lastError(zip);
				return false;
			}
			out.close();
			if (!out.good()) {
				Log::e() << "ZipFile: error writing '" << outPath.string() << "'";
				return false;
			}
		}
		return true;
	}
};

} // namespace

// ---------------------------------------------------------------------------
// ZipFile
// ---------------------------------------------------------------------------

bool ZipFile::zip(const fs::path &dirToZip, const fs::path &outZipFile, Compression compression) {
	if (!isValidSourceDir(dirToZip)) return false;

	FILE *out = zipio::openFile(outZipFile, "wb");
	if (out == nullptr) {
		Log::e() << "ZipFile: can't open '" << outZipFile.string() << "' for writing";
		return false;
	}

	bool ok = false;
	{
		Writer writer;
		if (mz_zip_writer_init_cfile(&writer.zip, out, 0)) {
			writer.initialised = true;
			ok = writer.addDirectory(dirToZip, "", compressionLevel(compression)) && writer.finalize();
		} else {
			Log::e() << "ZipFile: failed to init writer: " << lastError(writer.zip);
		}
	}

	if (fclose(out) != 0) {
		Log::e() << "ZipFile: error closing '" << outZipFile.string() << "'";
		ok = false;
	}

	if (!ok) {
		std::error_code ec;
		fs::remove(outZipFile, ec);
	}
	return ok;
}

bool ZipFile::zip(const fs::path &dirToZip, std::vector<unsigned char> &outZipData, Compression compression) {
	outZipData.clear();
	if (!isValidSourceDir(dirToZip)) return false;

	Writer writer;
	writer.zip.m_pWrite		= vectorWrite;
	writer.zip.m_pIO_opaque = &outZipData;
	if (!mz_zip_writer_init_v2(&writer.zip, 0, 0)) {
		Log::e() << "ZipFile: failed to init memory writer: " << lastError(writer.zip);
		return false;
	}
	writer.initialised = true;

	bool ok = writer.addDirectory(dirToZip, "", compressionLevel(compression)) && writer.finalize();
	if (!ok) outZipData.clear();
	return ok;
}

bool ZipFile::unzip(const fs::path &zipFile, const fs::path &outDir) {
	Reader reader;
	return reader.open(zipFile) && reader.extractAll(outDir);
}

bool ZipFile::unzip(const std::vector<unsigned char> &inZipData, const fs::path &outDir) {
	Reader reader;
	return reader.open(inZipData) && reader.extractAll(outDir);
}

std::vector<std::string> ZipFile::listZip(const std::vector<uint8_t> &inZipData) {
	Reader reader;
	if (!reader.open(inZipData)) return {};
	return reader.list();
}

std::vector<std::string> ZipFile::listZip(const fs::path &pathToZip) {
	Reader reader;
	if (!reader.open(pathToZip)) return {};
	return reader.list();
}

bool ZipFile::getTextFileFromZip(const fs::path &pathToZip, const fs::path &filePath, std::string &outData) {
	Reader reader;
	if (!reader.open(pathToZip)) return false;
	return reader.extractToContainer(filePath.generic_string(), outData);
}

bool ZipFile::getBinaryFileFromZip(const fs::path &pathToZip,
								   const fs::path &filePath,
								   std::vector<unsigned char> &data) {
	Reader reader;
	if (!reader.open(pathToZip)) return false;
	return reader.extractToContainer(filePath.generic_string(), data);
}
