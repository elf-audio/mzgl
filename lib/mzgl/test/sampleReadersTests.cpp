#include "tests.h"

#include "SampleData.h"
#include "SampleFormat.h"
#include "SampleReaders.h"

#include <cstdint>
#include <vector>

struct SampleReadersFixture {
	static constexpr float i8tolerance	= 1.0f / 127.0f + 1.0f / 128.0f + 1e-5f;
	static constexpr float i16tolerance = 1.0f / 32767.0f + 1.0f / 32768.0f + 1e-6f;
	static constexpr float i24tolerance = 1.0f / 8388607.0f + 1.0f / 8388608.0f + 1e-7f;

	float toleranceFor(SampleFormat f) const {
		switch (f) {
			case SampleFormat::I8: return i8tolerance;
			case SampleFormat::I16: return i16tolerance;
			case SampleFormat::I24: return i24tolerance;
			case SampleFormat::F32: return 0.0f;
		}
		return 0.0f;
	}

	std::string formatName(SampleFormat f) const {
		switch (f) {
			case SampleFormat::I8: return "I8";
			case SampleFormat::I16: return "I16";
			case SampleFormat::I24: return "I24";
			case SampleFormat::F32: return "F32";
		}
		return "?";
	}

	SampleData makeSized(SampleFormat fmt, size_t numSamples) const {
		SampleData sd(std::vector<uint8_t> {}, fmt);
		sd.resize(numSamples);
		return sd;
	}
};

SCENARIO_METHOD(SampleReadersFixture,
				"Per-format scalar write followed by scalar read round-trips within encoding precision",
				"[sample-readers]") {
	const std::vector<float> testValues = {-1.0f, -0.75f, -0.5f, -0.1f, 0.0f, 0.1f, 0.5f, 0.75f, 1.0f};

	for (auto fmt : {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("a " + formatName(fmt) + " SampleData with one slot per test value") {
			auto sd		   = makeSized(fmt, testValues.size());
			auto writeFn   = sample_readers::resolveWriteSample(fmt);
			auto readFn	   = sample_readers::resolveReadSample(fmt);

			WHEN("each test value is written via the resolved write function") {
				for (size_t i = 0; i < testValues.size(); ++i) {
					writeFn(sd, static_cast<int>(i), testValues[i]);
				}
				THEN("the resolved read function returns each value within encoding tolerance") {
					for (size_t i = 0; i < testValues.size(); ++i) {
						REQUIRE(readFn(sd, static_cast<int>(i))
								== Catch::Approx(testValues[i]).margin(toleranceFor(fmt)));
					}
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleReadersFixture,
				"Scalar reads agree with SampleData::operator[] for each format",
				"[sample-readers]") {
	for (auto fmt : {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("a " + formatName(fmt) + " SampleData populated via assignValue") {
			auto sd		 = makeSized(fmt, 4);
			sd.assignValue(0, -0.5f);
			sd.assignValue(1, 0.0f);
			sd.assignValue(2, 0.25f);
			sd.assignValue(3, 1.0f);

			auto readFn	 = sample_readers::resolveReadSample(fmt);

			THEN("the resolved read function returns the same values as operator[]") {
				for (size_t i = 0; i < sd.size(); ++i) {
					REQUIRE(readFn(sd, static_cast<int>(i)) == sd[i]);
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleReadersFixture,
				"Integer-format scalar writes clamp out-of-range values",
				"[sample-readers]") {
	for (auto fmt : {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24}) {
		GIVEN("a two-sample " + formatName(fmt) + " buffer") {
			auto sd		 = makeSized(fmt, 2);
			auto writeFn = sample_readers::resolveWriteSample(fmt);
			auto readFn	 = sample_readers::resolveReadSample(fmt);

			WHEN("an out-of-range positive and negative value are written") {
				writeFn(sd, 0, 2.0f);
				writeFn(sd, 1, -2.0f);

				THEN("the read-back values are clamped to [-1, 1]") {
					REQUIRE(readFn(sd, 0) == Catch::Approx(1.0f).margin(toleranceFor(fmt)));
					REQUIRE(readFn(sd, 1) == Catch::Approx(-1.0f).margin(toleranceFor(fmt)));
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleReadersFixture,
				"F32 scalar write stores values verbatim without clamping",
				"[sample-readers]") {
	GIVEN("an F32 SampleData") {
		auto sd		 = makeSized(SampleFormat::F32, 2);
		auto writeFn = sample_readers::resolveWriteSample(SampleFormat::F32);
		auto readFn	 = sample_readers::resolveReadSample(SampleFormat::F32);

		WHEN("values outside [-1, 1] are written") {
			writeFn(sd, 0, 2.5f);
			writeFn(sd, 1, -3.25f);

			THEN("the read function returns the values verbatim") {
				REQUIRE(readFn(sd, 0) == 2.5f);
				REQUIRE(readFn(sd, 1) == -3.25f);
			}
		}
	}
}

SCENARIO_METHOD(SampleReadersFixture,
				"readFrame on a mono SampleData duplicates the sample into both L and R",
				"[sample-readers]") {
	for (auto fmt : {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("a 3-frame mono " + formatName(fmt) + " SampleData with known sample values") {
			auto sd = makeSized(fmt, 3);
			sd.assignValue(0, 0.1f);
			sd.assignValue(1, -0.4f);
			sd.assignValue(2, 0.7f);

			auto frameReadFn = sample_readers::resolveReadFrame(fmt);

			WHEN("each frame is read with numChans = 1") {
				const std::vector<float> expected = {0.1f, -0.4f, 0.7f};
				for (int f = 0; f < 3; ++f) {
					float L = 0.f;
					float R = 0.f;
					frameReadFn(sd, f, 1, L, R);

					THEN("L equals the sample value (frame " + std::to_string(f) + ")") {
						REQUIRE(L == Catch::Approx(expected[f]).margin(toleranceFor(fmt)));
					}
					THEN("R equals L (mono: R = L) (frame " + std::to_string(f) + ")") {
						REQUIRE(R == L);
					}
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleReadersFixture,
				"readFrame on a stereo SampleData splits channel 0 into L and channel 1 into R",
				"[sample-readers]") {
	for (auto fmt : {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("a 2-frame stereo " + formatName(fmt) + " SampleData with distinct L and R per frame") {
			auto sd = makeSized(fmt, 4); // 2 frames * 2 channels
			sd.assignValue(0, 0.3f);	 // frame 0, L
			sd.assignValue(1, -0.3f);	 // frame 0, R
			sd.assignValue(2, 0.6f);	 // frame 1, L
			sd.assignValue(3, -0.6f);	 // frame 1, R

			auto frameReadFn = sample_readers::resolveReadFrame(fmt);

			WHEN("frame 0 is read with numChans = 2") {
				float L = 0.f;
				float R = 0.f;
				frameReadFn(sd, 0, 2, L, R);
				THEN("L matches channel 0") {
					REQUIRE(L == Catch::Approx(0.3f).margin(toleranceFor(fmt)));
				}
				THEN("R matches channel 1") {
					REQUIRE(R == Catch::Approx(-0.3f).margin(toleranceFor(fmt)));
				}
			}

			WHEN("frame 1 is read with numChans = 2") {
				float L = 0.f;
				float R = 0.f;
				frameReadFn(sd, 1, 2, L, R);
				THEN("L matches channel 0 of frame 1") {
					REQUIRE(L == Catch::Approx(0.6f).margin(toleranceFor(fmt)));
				}
				THEN("R matches channel 1 of frame 1") {
					REQUIRE(R == Catch::Approx(-0.6f).margin(toleranceFor(fmt)));
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleReadersFixture,
				"resolveReadSample returns the function appropriate for the given format",
				"[sample-readers]") {
	for (auto fmt : {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("a " + formatName(fmt) + " SampleData populated with a known value") {
			auto sd = makeSized(fmt, 1);
			sd.assignValue(0, 0.5f);

			WHEN("a read function is resolved via resolveReadSample") {
				auto readFn = sample_readers::resolveReadSample(fmt);

				THEN("the function pointer is non-null") {
					REQUIRE(readFn != nullptr);
				}
				THEN("the resolved function reads the value within encoding tolerance") {
					REQUIRE(readFn(sd, 0) == Catch::Approx(0.5f).margin(toleranceFor(fmt)));
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleReadersFixture,
				"resolveWriteSample returns the function appropriate for the given format",
				"[sample-readers]") {
	for (auto fmt : {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("an empty " + formatName(fmt) + " SampleData with one slot") {
			auto sd = makeSized(fmt, 1);

			WHEN("a write function is resolved via resolveWriteSample") {
				auto writeFn = sample_readers::resolveWriteSample(fmt);

				THEN("the function pointer is non-null") {
					REQUIRE(writeFn != nullptr);
				}
				THEN("the resolved function writes a value that operator[] reads back") {
					writeFn(sd, 0, -0.25f);
					REQUIRE(sd[0] == Catch::Approx(-0.25f).margin(toleranceFor(fmt)));
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleReadersFixture,
				"resolveReadFrame returns the function appropriate for the given format",
				"[sample-readers]") {
	for (auto fmt : {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("a 1-frame stereo " + formatName(fmt) + " SampleData with distinct L and R") {
			auto sd = makeSized(fmt, 2);
			sd.assignValue(0, 0.4f);
			sd.assignValue(1, -0.8f);

			WHEN("a frame-read function is resolved") {
				auto frameReadFn = sample_readers::resolveReadFrame(fmt);

				THEN("the function pointer is non-null") {
					REQUIRE(frameReadFn != nullptr);
				}
				THEN("the resolved function reads L and R from the right channels") {
					float L = 0.f;
					float R = 0.f;
					frameReadFn(sd, 0, 2, L, R);
					REQUIRE(L == Catch::Approx(0.4f).margin(toleranceFor(fmt)));
					REQUIRE(R == Catch::Approx(-0.8f).margin(toleranceFor(fmt)));
				}
			}
		}
	}
}
