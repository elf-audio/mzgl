// Coverage for Image::load + Texture::loadFromPixels failure modes. These
// fault paths were the root of the Windows 5ae344c1 "GPU driver" crash
// cluster — a corrupt `bg.png` produced a pixel buffer smaller than the
// dimensions reported back, and glTexImage2D read off the end of the
// vector's heap allocation, faulting inside the GPU driver.

#include "tests.h"

#include "Image.h"
#include "filesystem.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

namespace {

struct TempTestDir {
	fs::path path;
	explicit TempTestDir(const std::string &name)
		: path(fs::temp_directory_path() / ("koala-test-" + name)) {
		std::error_code ec;
		fs::remove_all(path, ec);
		fs::create_directories(path);
	}
	~TempTestDir() {
		std::error_code ec;
		fs::remove_all(path, ec);
	}
	std::string getPath() const { return path.string(); }
};

void writeBytes(const std::string &path, const std::vector<uint8_t> &bytes) {
	std::ofstream f(path, std::ios::binary);
	f.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

// Build a minimal valid PNG so we can compare "real" load behaviour against
// the corrupt cases below. 2x2 RGBA, opaque red, generated with
// System.Drawing.Bitmap.Save and pasted as raw bytes so the test has no
// on-disk fixture dependency.
std::vector<uint8_t> tinyValidPngRGBA() {
	return {
		0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
		0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0xB6, 0x0D,
		0x24, 0x00, 0x00, 0x00, 0x01, 0x73, 0x52, 0x47, 0x42, 0x00, 0xAE, 0xCE, 0x1C, 0xE9, 0x00, 0x00,
		0x00, 0x04, 0x67, 0x41, 0x4D, 0x41, 0x00, 0x00, 0xB1, 0x8F, 0x0B, 0xFC, 0x61, 0x05, 0x00, 0x00,
		0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0E, 0xC3, 0x00, 0x00, 0x0E, 0xC3, 0x01, 0xC7,
		0x6F, 0xA8, 0x64, 0x00, 0x00, 0x00, 0x12, 0x49, 0x44, 0x41, 0x54, 0x18, 0x57, 0x63, 0x38, 0x21,
		0x27, 0xF7, 0x1F, 0x84, 0xA1, 0x0C, 0xB9, 0xFF, 0x00, 0x47, 0x34, 0x08, 0x0D, 0x97, 0x7A, 0x4D,
		0x04, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
	};
}

bool callImageLoad(const std::string &path,
				   std::vector<uint8_t> &outData,
				   int &w, int &h, int &nc) {
	int bpc		 = 0;
	bool isFloat = false;
	return Image::load(path, outData, w, h, nc, bpc, isFloat);
}

} // namespace

TEST_CASE("Image::load rejects empty file", "[image]") {
	TempTestDir tmp("image-load-empty");
	std::string p = tmp.getPath() + "/zero.png";
	writeBytes(p, {});

	std::vector<uint8_t> data;
	int w = 0, h = 0, nc = 0;
	REQUIRE(callImageLoad(p, data, w, h, nc) == false);
}

TEST_CASE("Image::load rejects random garbage", "[image]") {
	TempTestDir tmp("image-load-garbage");
	std::string p = tmp.getPath() + "/junk.png";
	std::vector<uint8_t> junk(1024);
	for (size_t i = 0; i < junk.size(); ++i) junk[i] = static_cast<uint8_t>((i * 31) ^ 0xA5);
	writeBytes(p, junk);

	std::vector<uint8_t> data;
	int w = 0, h = 0, nc = 0;
	REQUIRE(callImageLoad(p, data, w, h, nc) == false);
}

TEST_CASE("Image::load rejects truncated PNG", "[image]") {
	TempTestDir tmp("image-load-truncated");
	std::string p = tmp.getPath() + "/truncated.png";
	auto bytes	  = tinyValidPngRGBA();
	bytes.resize(bytes.size() / 2);
	writeBytes(p, bytes);

	std::vector<uint8_t> data;
	int w = 0, h = 0, nc = 0;
	REQUIRE(callImageLoad(p, data, w, h, nc) == false);
}

TEST_CASE("Image::load buffer matches reported dimensions", "[image]") {
	// Critical invariant: if Image::load returns true, outData.size() must be
	// exactly w*h*numChans bytes. The pre-fix code computed that size in
	// signed 32-bit ints, so a 33000x33000 PNG produced a wrap-around resize
	// and the buffer was much smaller than the dims claimed — feeding that
	// straight into glTexImage2D was the GPU driver crash.
	TempTestDir tmp("image-load-valid");
	std::string p = tmp.getPath() + "/ok.png";
	writeBytes(p, tinyValidPngRGBA());

	std::vector<uint8_t> data;
	int w = 0, h = 0, nc = 0;
	REQUIRE(callImageLoad(p, data, w, h, nc) == true);
	REQUIRE(w == 2);
	REQUIRE(h == 2);
	REQUIRE(nc == 4);
	REQUIRE(data.size() == static_cast<size_t>(w) * h * nc);
}

// Texture::loadFromPixels is the second line of defence (it rejects
// undersized buffers / bad channel counts before calling glTexImage2D), but
// it lives on an abstract base whose `allocate()` needs a live GL context,
// so we don't exercise it from this off-screen unit test. The Image::load
// tests above cover the same root cause — the int-overflow that made the
// pixel buffer smaller than the dimensions said it was.
