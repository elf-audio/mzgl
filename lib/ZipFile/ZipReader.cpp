#include "ZipReader.h"
#include "ZipIO.h"
#include "miniz.h"
#include "mzgl/util/log.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace {

constexpr uint32_t LOCAL_FILE_HEADER_SIG	= 0x04034b50;
constexpr uint64_t LOCAL_FILE_HEADER_SIZE	= 30;
constexpr uint64_t LOCAL_FILE_HEADER_FN_OFS = 26; // offset of the filename length field

constexpr int COMPRESSION_STORE	  = 0;
constexpr int COMPRESSION_DEFLATE = 8;

uint16_t readLE16(const unsigned char *p) {
	return static_cast<uint16_t>(p[0] | (p[1] << 8));
}
uint32_t readLE32(const unsigned char *p) {
	return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16)
		   | (static_cast<uint32_t>(p[3]) << 24);
}

struct MzReader {
	mz_zip_archive zip;
	FILE *file		 = nullptr;
	bool initialised = false;

	MzReader() { mz_zip_zero_struct(&zip); }
	~MzReader() {
		if (initialised) mz_zip_reader_end(&zip);
		if (file != nullptr) fclose(file);
	}
	MzReader(const MzReader &)			  = delete;
	MzReader &operator=(const MzReader &) = delete;

	// Throws on failure.
	void open(const std::string &zipPath) {
		file = zipio::openFile(fs::path(zipPath), "rb");
		if (file == nullptr) {
			throw std::runtime_error("Can't read zip file '" + zipPath + "'");
		}
		// We look entries up by index, so skip miniz's sorted name index.
		if (!mz_zip_reader_init_cfile(&zip, file, 0, MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY)) {
			throw std::runtime_error("Can't read zip central directory of '" + zipPath
									 + "': " + mz_zip_get_error_string(mz_zip_get_last_error(&zip)));
		}
		initialised = true;
	}

	std::string entryName(mz_uint index) {
		auto len = mz_zip_reader_get_filename(&zip, index, nullptr, 0);
		std::string name(len, '\0');
		if (len > 0) {
			mz_zip_reader_get_filename(&zip, index, name.data(), len);
			name.resize(len - 1);
		}
		return name;
	}
};

} // namespace

// ---------------------------------------------------------------------------
// ZipReaderFile
// ---------------------------------------------------------------------------

ZipReaderFile::ZipReaderFile(const std::string &zipPath, const ZipReader::Entry &entry) {
	if (entry.compression != COMPRESSION_STORE && entry.compression != COMPRESSION_DEFLATE) {
		throw std::runtime_error("Unsupported compression method " + std::to_string(entry.compression)
								 + " for zip entry '" + entry.path + "'");
	}

	zip = zipio::openInput(fs::path(zipPath));
	if (!zip.good()) {
		throw std::runtime_error("Can't read zip file '" + zipPath + "'");
	}

	// Find where the entry's data starts: local header + filename + extra field.
	unsigned char header[LOCAL_FILE_HEADER_SIZE];
	zip.seekg(static_cast<std::streamoff>(entry.offset));
	zip.read(reinterpret_cast<char *>(header), sizeof(header));
	if (!zip.good() || readLE32(header) != LOCAL_FILE_HEADER_SIG) {
		throw std::runtime_error("Bad local file header for zip entry '" + entry.path + "'");
	}
	auto fnLength	 = readLE16(header + LOCAL_FILE_HEADER_FN_OFS);
	auto extraLength = readLE16(header + LOCAL_FILE_HEADER_FN_OFS + 2);

	fileStart = entry.offset + LOCAL_FILE_HEADER_SIZE + fnLength + extraLength;
	fileSize  = entry.size;

	if (entry.compression == COMPRESSION_DEFLATE) {
		// DEFLATE: let miniz inflate the whole entry into memory (it also
		// checks the CRC), then serve reads from there.
		isDeflate = true;
		zip.close();

		MzReader reader;
		reader.open(zipPath);
		if (reader.entryName(entry.index) != entry.path) {
			throw std::runtime_error("Zip entry '" + entry.path + "' moved - zip changed on disk?");
		}
		decompressedData.resize(static_cast<size_t>(fileSize));
		if (fileSize > 0
			&& !mz_zip_reader_extract_to_mem(
				&reader.zip, entry.index, decompressedData.data(), decompressedData.size(), 0)) {
			throw std::runtime_error("Failed to inflate zip entry '" + entry.path
									 + "': " + mz_zip_get_error_string(mz_zip_get_last_error(&reader.zip)));
		}
		memPos = 0;
	} else {
		zip.seekg(static_cast<std::streamoff>(fileStart));
	}
}

uint64_t ZipReaderFile::tell() {
	if (isDeflate) return memPos;
	zip.clear(); // tellg() fails once eof is set
	auto pos = zip.tellg();
	if (pos < 0) return fileSize;
	auto abs = static_cast<uint64_t>(pos);
	return abs < fileStart ? 0 : abs - fileStart;
}

bool ZipReaderFile::seek(int64_t offset, SeekOrigin origin) {
	int64_t newPos = 0;
	switch (origin) {
		case SeekOrigin::Start: newPos = offset; break;
		case SeekOrigin::Current: newPos = static_cast<int64_t>(tell()) + offset; break;
	}
	if (newPos < 0 || static_cast<uint64_t>(newPos) > fileSize) return false;

	if (isDeflate) {
		memPos = static_cast<uint64_t>(newPos);
	} else {
		zip.clear();
		zip.seekg(static_cast<std::streamoff>(fileStart + static_cast<uint64_t>(newPos)));
	}
	return true;
}

size_t ZipReaderFile::read(int8_t *d, size_t sz) {
	uint64_t pos = tell();
	if (pos >= fileSize) return 0;
	size_t toRead = static_cast<size_t>(std::min<uint64_t>(sz, fileSize - pos));

	if (isDeflate) {
		std::memcpy(d, decompressedData.data() + memPos, toRead);
		memPos += toRead;
		return toRead;
	}

	zip.read(reinterpret_cast<char *>(d), static_cast<std::streamsize>(toRead));
	return static_cast<size_t>(zip.gcount());
}

std::vector<int8_t> ZipReaderFile::read() {
	std::vector<int8_t> data;
	std::vector<int8_t> buff(4096);
	while (true) {
		auto amountRead = readSome(buff);
		data.insert(data.end(), buff.begin(), buff.begin() + static_cast<std::ptrdiff_t>(amountRead));
		if (amountRead != buff.size()) break;
	}
	return data;
}

size_t ZipReaderFile::readSome(std::vector<int8_t> &d) {
	return read(d.data(), d.size());
}

void ZipReaderFile::extract(const std::string &path) {
	seek(0);
	std::ofstream f = zipio::openOutput(fs::path(path));
	std::vector<int8_t> buff(4096);
	while (true) {
		auto amountRead = readSome(buff);
		f.write(reinterpret_cast<const char *>(buff.data()), static_cast<std::streamsize>(amountRead));
		if (amountRead != buff.size()) break;
	}
}

// ---------------------------------------------------------------------------
// ZipReader
// ---------------------------------------------------------------------------

ZipReader::ZipReader(const std::string &path)
	: zipPath(path) {
	MzReader reader;
	reader.open(path);

	auto numEntries = mz_zip_reader_get_num_files(&reader.zip);
	entries.reserve(numEntries);
	for (mz_uint i = 0; i < numEntries; i++) {
		mz_zip_archive_file_stat st;
		if (!mz_zip_reader_file_stat(&reader.zip, i, &st)) {
			throw std::runtime_error("Can't read entry " + std::to_string(i) + " of zip '" + path
									 + "': " + mz_zip_get_error_string(mz_zip_get_last_error(&reader.zip)));
		}
		entries.emplace_back(
			reader.entryName(i), st.m_local_header_ofs, st.m_uncomp_size, st.m_comp_size, st.m_method, i);
	}
}

std::shared_ptr<ZipReaderFile> ZipReader::open(const std::string &pathInZip) {
	auto currEntry = findEntry(pathInZip);
	if (!currEntry.valid) return nullptr;
	return std::make_shared<ZipReaderFile>(zipPath, currEntry);
}

std::vector<std::string> ZipReader::list(bool print) {
	std::vector<std::string> ret;
	ret.reserve(entries.size());
	for (auto &s: entries) {
		ret.emplace_back(s.path);
		if (print) {
			Log::d() << s.path << ": " << s.offset << "/" << s.size;
		}
	}
	return ret;
}

ZipReader::Entry ZipReader::findEntry(const std::string &path) {
	for (const auto &e: entries) {
		if (e.path == path) {
			return e;
		}
	}
	return Entry();
}
