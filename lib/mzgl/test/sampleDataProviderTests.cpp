#include "tests.h"

#include "SampleData.h"
#include "SampleProvider.h"

#include <cstdint>
#include <memory>
#include <vector>

struct SampleDataProviderFixture {
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

	SampleData filledRamp(SampleFormat fmt, size_t numSamples, float start, float step) const {
		auto sd = makeSized(fmt, numSamples);
		float v = start;
		for (size_t i = 0; i < numSamples; ++i) {
			sd.assignValue(static_cast<int>(i), v);
			v += step;
		}
		return sd;
	}
};

SCENARIO_METHOD(SampleDataProviderFixture,
				"SampleDataProvider::size() mirrors the underlying SampleData",
				"[sample-data-provider]") {
	GIVEN("a SampleData with four samples and a provider over it") {
		auto sd = makeSized(SampleFormat::F32, 4);
		SampleDataProvider provider(sd);
		THEN("provider.size() equals data.size()") {
			REQUIRE(provider.size() == sd.size());
			REQUIRE(provider.size() == 4u);
		}
	}

	GIVEN("an empty SampleData and a provider over it") {
		SampleData sd;
		SampleDataProvider provider(sd);
		THEN("provider.size() is zero") {
			REQUIRE(provider.size() == 0u);
		}
	}
}

SCENARIO_METHOD(SampleDataProviderFixture,
				"SampleDataProvider::operator[] returns the same float values SampleData does",
				"[sample-data-provider]") {
	for (auto fmt : {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("a 5-sample " + formatName(fmt) + " ramp and a provider over it") {
			auto sd = filledRamp(fmt, 5, -0.5f, 0.25f);
			SampleDataProvider provider(sd);
			THEN("provider[i] equals sd[i] for every index") {
				for (size_t i = 0; i < sd.size(); ++i) {
					REQUIRE(provider[static_cast<int>(i)] == sd[i]);
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleDataProviderFixture,
				"SampleDataProvider::splice fills the output buffer with the requested half-open range",
				"[sample-data-provider]") {
	for (auto fmt : {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("a 6-sample " + formatName(fmt) + " ramp and a provider over it") {
			auto sd = filledRamp(fmt, 6, 0.0f, 0.1f); // 0.0, 0.1, 0.2, 0.3, 0.4, 0.5
			SampleDataProvider provider(sd);

			WHEN("splice(2, 5, outBuff) is called on a pre-existing buffer") {
				FloatBuffer outBuff;
				outBuff.resize(99); // start with a wrong size to confirm resize happens
				provider.splice(2, 5, outBuff);

				THEN("outBuff has size end - start") {
					REQUIRE(outBuff.size() == 3u);
				}
				THEN("outBuff contains the values from the half-open source range") {
					REQUIRE(outBuff[0] == Catch::Approx(0.2f).margin(toleranceFor(fmt)));
					REQUIRE(outBuff[1] == Catch::Approx(0.3f).margin(toleranceFor(fmt)));
					REQUIRE(outBuff[2] == Catch::Approx(0.4f).margin(toleranceFor(fmt)));
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleDataProviderFixture,
				"SampleDataProvider::getSamples(FloatBuffer&) fills a pre-sized buffer starting at startPos",
				"[sample-data-provider]") {
	for (auto fmt : {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("a 5-sample " + formatName(fmt) + " ramp and a 3-slot output buffer") {
			auto sd = filledRamp(fmt, 5, 0.0f, 0.1f); // 0.0, 0.1, 0.2, 0.3, 0.4
			SampleDataProvider provider(sd);
			FloatBuffer outBuff;
			outBuff.resize(3);

			WHEN("getSamples(1, outBuff) is called") {
				provider.getSamples(1, outBuff);

				THEN("the buffer keeps its original size") {
					REQUIRE(outBuff.size() == 3u);
				}
				THEN("the buffer contains samples starting at startPos") {
					REQUIRE(outBuff[0] == Catch::Approx(0.1f).margin(toleranceFor(fmt)));
					REQUIRE(outBuff[1] == Catch::Approx(0.2f).margin(toleranceFor(fmt)));
					REQUIRE(outBuff[2] == Catch::Approx(0.3f).margin(toleranceFor(fmt)));
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleDataProviderFixture,
				"SampleDataProvider::getSamples(float *) fills a raw pointer with `length` samples",
				"[sample-data-provider]") {
	for (auto fmt : {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("a 5-sample " + formatName(fmt) + " ramp and a stack-allocated 3-float buffer") {
			auto sd = filledRamp(fmt, 5, 0.0f, 0.1f);
			SampleDataProvider provider(sd);
			float outArr[3] = {0.f, 0.f, 0.f};

			WHEN("getSamples(1, 3, outArr) is called") {
				provider.getSamples(1, 3, outArr);

				THEN("outArr contains the three samples starting at index 1") {
					REQUIRE(outArr[0] == Catch::Approx(0.1f).margin(toleranceFor(fmt)));
					REQUIRE(outArr[1] == Catch::Approx(0.2f).margin(toleranceFor(fmt)));
					REQUIRE(outArr[2] == Catch::Approx(0.3f).margin(toleranceFor(fmt)));
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleDataProviderFixture,
				"SampleDataProvider::findMinMax reports the extrema over a half-open range",
				"[sample-data-provider]") {
	GIVEN("an F32 SampleData with mixed positive and negative values") {
		auto sd = makeSized(SampleFormat::F32, 5);
		sd.assignValue(0, 0.2f);
		sd.assignValue(1, -0.6f);
		sd.assignValue(2, 0.9f);
		sd.assignValue(3, -0.1f);
		sd.assignValue(4, 0.0f);
		SampleDataProvider provider(sd);

		WHEN("findMinMax(0, 5, min, max) is called over the full range") {
			float min = 99.f;
			float max = -99.f;
			provider.findMinMax(0, 5, min, max);
			THEN("min and max reflect the extrema of the samples") {
				REQUIRE(min == Catch::Approx(-0.6f));
				REQUIRE(max == Catch::Approx(0.9f));
			}
		}

		WHEN("findMinMax(1, 3, min, max) is called over a subrange") {
			float min = 99.f;
			float max = -99.f;
			provider.findMinMax(1, 3, min, max);
			THEN("min and max reflect only the values in [1, 3)") {
				REQUIRE(min == Catch::Approx(-0.6f));
				REQUIRE(max == Catch::Approx(0.9f));
			}
		}
	}

	GIVEN("a silent F32 SampleData") {
		auto sd = makeSized(SampleFormat::F32, 4);
		SampleDataProvider provider(sd);

		WHEN("findMinMax is called over the full range") {
			float min = 99.f;
			float max = -99.f;
			provider.findMinMax(0, 4, min, max);
			THEN("both min and max are zero") {
				REQUIRE(min == 0.0f);
				REQUIRE(max == 0.0f);
			}
		}
	}
}

SCENARIO_METHOD(SampleDataProviderFixture,
				"SampleDataProvider::isFloat reflects whether the underlying format is F32",
				"[sample-data-provider]") {
	GIVEN("an F32 SampleData and a provider over it") {
		auto sd = makeSized(SampleFormat::F32, 4);
		SampleDataProvider provider(sd);
		THEN("isFloat() returns true") {
			REQUIRE(provider.isFloat());
		}
	}

	for (auto fmt : {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24}) {
		GIVEN("a " + formatName(fmt) + " SampleData and a provider over it") {
			auto sd = makeSized(fmt, 4);
			SampleDataProvider provider(sd);
			THEN("isFloat() returns false") {
				REQUIRE_FALSE(provider.isFloat());
			}
		}
	}
}

SCENARIO_METHOD(SampleDataProviderFixture,
				"SampleDataProvider::shallowCopy returns a new SampleProvider with the same view",
				"[sample-data-provider]") {
	GIVEN("a 4-sample F32 SampleData and a provider over it") {
		auto sd = filledRamp(SampleFormat::F32, 4, 0.1f, 0.1f);
		SampleDataProvider provider(sd);

		WHEN("shallowCopy() is called") {
			std::unique_ptr<SampleProvider> copy(provider.shallowCopy());

			THEN("the returned pointer is non-null") {
				REQUIRE(copy != nullptr);
			}
			THEN("the copy reports the same size") {
				REQUIRE(copy->size() == provider.size());
			}
			THEN("the copy returns the same values as the original") {
				for (size_t i = 0; i < provider.size(); ++i) {
					REQUIRE((*copy)[static_cast<int>(i)] == provider[static_cast<int>(i)]);
				}
			}
			THEN("the copy's isFloat() matches the original") {
				REQUIRE(copy->isFloat() == provider.isFloat());
			}
		}
	}
}

SCENARIO_METHOD(SampleDataProviderFixture,
				"SampleDataProvider is a live view: changes to the underlying data are visible",
				"[sample-data-provider]") {
	GIVEN("a 3-sample F32 SampleData and a provider constructed before any writes") {
		auto sd = makeSized(SampleFormat::F32, 3);
		SampleDataProvider provider(sd);

		WHEN("the underlying SampleData is mutated after the provider is constructed") {
			sd.assignValue(0, 0.25f);
			sd.assignValue(1, -0.5f);
			sd.assignValue(2, 0.75f);

			THEN("provider reads the new values, not a snapshot of the old ones") {
				REQUIRE(provider[0] == 0.25f);
				REQUIRE(provider[1] == -0.5f);
				REQUIRE(provider[2] == 0.75f);
			}
		}
	}
}

SCENARIO_METHOD(SampleDataProviderFixture,
				"SampleDataProvider works polymorphically through a SampleProvider base reference",
				"[sample-data-provider]") {
	GIVEN("a 3-sample I16 SampleData and a SampleProvider& bound to a SampleDataProvider") {
		auto sd = filledRamp(SampleFormat::I16, 3, 0.0f, 0.25f);
		SampleDataProvider concrete(sd);
		SampleProvider &provider = concrete;

		THEN("the base-class virtual calls route into SampleDataProvider's overrides") {
			REQUIRE(provider.size() == 3u);
			REQUIRE_FALSE(provider.isFloat());
			REQUIRE(provider[0] == Catch::Approx(0.0f).margin(i16tolerance));
			REQUIRE(provider[1] == Catch::Approx(0.25f).margin(i16tolerance));
			REQUIRE(provider[2] == Catch::Approx(0.5f).margin(i16tolerance));
		}
	}
}
