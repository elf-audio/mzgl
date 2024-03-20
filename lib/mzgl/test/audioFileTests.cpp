#include "tests.h"

#include "filesystem.h"
#include "AudioFile.h"
#include <optional>

std::vector<std::string> split(const std::string &str, char delimiter) {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(str);
	while (std::getline(tokenStream, token, delimiter)) {
		token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());
		tokens.push_back(token);
	}
	return tokens;
}

std::string remove(const std::string &source, const std::string &substring) {
	auto str = source;
	auto pos = str.find(substring);
	if (pos != std::string::npos) {
		str.erase(pos, substring.length());
	}
	return str;
}

struct TestFile {
	enum class Endian { little, big };
	enum class Channels { mono, stereo };

	static std::optional<TestFile> fromPath(const fs::path &audioFilePath) {
		auto tokens = split(audioFilePath.stem().string(), '-');
		if (tokens.size() != 4) {
			return std::nullopt;
		}

		std::string sampleRate = remove(tokens[0], "Hz");
		std::string bitDepth   = remove(tokens[1], "Bits");
		std::string endian	   = tokens[2];
		std::string channels   = tokens[3];

		return TestFile {audioFilePath,
						 std::stoi(sampleRate),
						 std::stoi(bitDepth),
						 endian == "LE" ? Endian::little : Endian::big,
						 channels == "Mono" ? Channels::mono : Channels::stereo};
	}

	static std::string toString(Endian endian) {
		return endian == Endian::little ? "Little endian" : "Big endian";
	}

	static std::string toString(Channels channels) { return channels == Channels::mono ? "Mono" : "Stereo"; }
	static int toInt(Channels channels) { return channels == Channels::mono ? 1 : 2; }

	void describe() const {
		std::cout << filePath << std::endl;
		std::cout << "    " << sampleRate << std::endl;
		std::cout << "    " << bitDepth << std::endl;
		std::cout << "    " << TestFile::toString(endian) << std::endl;
		std::cout << "    " << TestFile::toString(channels) << std::endl;
	}

	void load() { REQUIRE(AudioFile::load(filePath, buffer, &outNumChannels, &outSampleRate)); }
	void loadResampled() {
		outSampleRate = sampleRate * 2;
		REQUIRE(AudioFile::loadResampled(filePath, buffer, outSampleRate, &outNumChannels));
	}

	FloatBuffer getFrequencyBuffer() {
		if (channels == Channels::stereo) {
			FloatBuffer left;
			FloatBuffer right;
			buffer.splitStereo(left, right);
			return left;
		}
		return buffer;
	}

	void test() {
		REQUIRE(outNumChannels == toInt(channels));
		REQUIRE((outSampleRate == sampleRate || outSampleRate == sampleRate * 2));
		REQUIRE_FALSE(buffer.empty());
		REQUIRE_FALSE(std::all_of(
			std::begin(buffer), std::end(buffer), [](auto &&sample) { return std::fabs(sample) <= 0.f; }));
		REQUIRE(std::all_of(std::begin(buffer), std::end(buffer), [](auto &&sample) {
			return sample <= 1.5f && sample >= -1.5f;
		}));
	}

	fs::path filePath;
	int sampleRate;
	int bitDepth;
	Endian endian;
	Channels channels;

	FloatBuffer buffer;
	int outNumChannels = 0;
	int outSampleRate  = 0;
};

class ValidFileScanner {
public:
	explicit ValidFileScanner(const fs::path &testFilesDirectory = fs::current_path() / "test-files"
																   / "valid-files") {
		for (const auto &entry: fs::directory_iterator(testFilesDirectory)) {
			if (auto testFile = TestFile::fromPath(entry.path()); testFile.has_value())
				testFiles.push_back(*testFile);
		}
	}

	std::vector<TestFile> testFiles;
};

struct InvalidTestFile {
	void load() {
		REQUIRE_FALSE(AudioFile::load(filePath, buffer, &outNumChannels, &outSampleRate));
		REQUIRE_NOTHROW(AudioFile::load(filePath, buffer, &outNumChannels, &outSampleRate));
	}
	void loadResampled() {
		REQUIRE_FALSE(AudioFile::loadResampled(filePath, buffer, 88200, &outNumChannels));
		REQUIRE_NOTHROW(AudioFile::loadResampled(filePath, buffer, 88200, &outNumChannels));
	}

	fs::path filePath;
	FloatBuffer buffer;
	int outNumChannels = 0;
	int outSampleRate  = 0;
};

class InvalidFileScanner {
public:
	explicit InvalidFileScanner(const fs::path &testFilesDirectory = fs::current_path() / "test-files"
																	 / "invalid-files") {
		for (const auto &entry: fs::directory_iterator(testFilesDirectory)) {
			testFiles.emplace_back(InvalidTestFile {entry.path()});
		}
	}
	std::vector<InvalidTestFile> testFiles;
};

class CorruptFileScanner {
public:
	explicit CorruptFileScanner(const fs::path &testFilesDirectory = fs::current_path() / "test-files"
																	 / "bad-files") {
		for (const auto &entry: fs::directory_iterator(testFilesDirectory)) {
			testFiles.emplace_back(InvalidTestFile {entry.path()});
		}
	}
	std::vector<InvalidTestFile> testFiles;
};

SCENARIO("Valid files can be loaded", "[audio-file]") {
	GIVEN("A list of test files") {
		ValidFileScanner scanner;
		for (auto testFile: scanner.testFiles) {
			WHEN("File at " + testFile.filePath.string() + " is loaded") {
				testFile.load();
				THEN("It validates as loaded properly") {
					testFile.test();
				}
			}
		}
	}
}

SCENARIO("Valid files can be loaded and resampled", "[audio-file]") {
	GIVEN("A list of test files") {
		ValidFileScanner scanner;
		for (auto testFile: scanner.testFiles) {
			WHEN("File at " + testFile.filePath.string() + " is loaded and resampled") {
				testFile.loadResampled();
				THEN("It validates as loaded properly") {
					testFile.test();
				}
			}
		}
	}
}

SCENARIO("Invalid files are not loaded", "[audio-file]") {
	GIVEN("A list of invalid files") {
		InvalidFileScanner scanner;
		for (auto testFile: scanner.testFiles) {
			WHEN("File at " + testFile.filePath.string() + " is loaded it should fail and not throw") {
				testFile.load();
			}
		}
	}
}

SCENARIO("Invalid files are not loaded during resampling", "[audio-file]") {
	GIVEN("A list of invalid files") {
		InvalidFileScanner scanner;
		for (auto testFile: scanner.testFiles) {
			WHEN("File at " + testFile.filePath.string()
				 + " is loaded and resampled it should fail and not throw") {
				testFile.loadResampled();
			}
		}
	}
}

SCENARIO("Corrupted files are not loaded", "[audio-file]") {
	GIVEN("A list of corrupted files") {
		CorruptFileScanner scanner;
		for (auto testFile: scanner.testFiles) {
			WHEN("File at " + testFile.filePath.string() + " is loaded it should fail and not throw") {
				testFile.load();
			}
		}
	}
}

SCENARIO("Corrupted files are not loaded during resampling", "[audio-file]") {
	GIVEN("A list of corrupted files") {
		CorruptFileScanner scanner;
		for (auto testFile: scanner.testFiles) {
			WHEN("File at " + testFile.filePath.string()
				 + " is loaded and resampled it should fail and not throw") {
				testFile.loadResampled();
			}
		}
	}
}

SCENARIO("Non existent files dont load", "[audio-file]") {
	GIVEN("A file that doesnt exist") {
		fs::path doesntExist {"/foo/bar/baz/qux/garfield.wav"};
		FloatBuffer buffer;
		int channels;
		int sampleRate;

		WHEN("File at " + doesntExist.string() + " is loaded it should fail and not throw") {
			REQUIRE_FALSE(AudioFile::load(doesntExist, buffer, &channels, &sampleRate));
		}
		AND_WHEN("File at " + doesntExist.string() + " is loaded and resampled it should fail and not throw") {
			REQUIRE_FALSE(AudioFile::loadResampled(doesntExist, buffer, 88200, &channels));
		}
	}
}

void writeTestFile(const fs::path &filePath) {
	std::ofstream file(filePath);
	REQUIRE(file.is_open());
	static constexpr char validWaveFileHeader[44] = {
		'R',  'I',	'F',  'F',	0x00, 0x00, 0x00, 0x00, 'W',  'A',	'V',  'E',	'f',  'm',	't',
		' ',  0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x64, 0x56, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 'd',  'a',	't',  'a',	0x00, 0x00, 0x00, 0x00};
	file.write(validWaveFileHeader, sizeof(validWaveFileHeader));
	file.close();
}

void makeFileWriteOnly(const fs::path &filePath) {
	fs::permissions(filePath, fs::perms::owner_write, fs::perm_options::replace);
}

void removeFile(const fs::path &filePath) {
	REQUIRE(fs::remove(filePath));
}

SCENARIO("Files that cant be read fail to load", "[audio-file]") {
	GIVEN("A file that has the wrong permissions") {
		auto filePath = fs::current_path() / "write-only.wav";
		FloatBuffer buffer;
		int channels;
		int sampleRate;

		writeTestFile(filePath);
		makeFileWriteOnly(filePath);

		WHEN("The file is loaded it should fail") {
			REQUIRE_FALSE(AudioFile::load(filePath, buffer, &channels, &sampleRate));
			removeFile(filePath);
		}
	}
}

SCENARIO("Files that cant be read fail to load and resample", "[audio-file]") {
	GIVEN("A file that has the wrong permissions") {
		auto filePath = fs::current_path() / "write-only.wav";
		FloatBuffer buffer;
		int channels;
		int sampleRate;

		writeTestFile(filePath);
		makeFileWriteOnly(filePath);

		WHEN("The file is loaded it should fail") {
			REQUIRE_FALSE(AudioFile::loadResampled(filePath, buffer, 88200, &channels));
			removeFile(filePath);
		}
	}
}