#include "tests.h"

#include "SampleData.h"

#include <cstdint>
#include <vector>

struct SampleDataFixture {
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

	SampleData makeEmpty(SampleFormat fmt) const { return SampleData(std::vector<uint8_t> {}, fmt); }

	SampleData makeSized(SampleFormat fmt, size_t numSamples) const {
		auto sd = makeEmpty(fmt);
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

SCENARIO_METHOD(SampleDataFixture, "A default-constructed SampleData is empty", "[sample-data]") {
	GIVEN("a default SampleData") {
		SampleData sd;
		THEN("size() is zero") {
			REQUIRE(sd.size() == 0);
		}
		THEN("empty() is true") {
			REQUIRE(sd.empty());
		}
		THEN("byteSize() is zero") {
			REQUIRE(sd.byteSize() == 0);
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"Constructing from a FloatBuffer yields an F32-tagged buffer with the same values",
				"[sample-data]") {
	GIVEN("a FloatBuffer with three values") {
		FloatBuffer fb;
		fb.push_back(-0.75f);
		fb.push_back(0.0f);
		fb.push_back(0.5f);

		WHEN("a SampleData is constructed from the FloatBuffer by const-ref") {
			SampleData sd(fb);
			THEN("its format is F32") {
				REQUIRE(sd.getFormat() == SampleFormat::F32);
			}
			THEN("size() matches the source length") {
				REQUIRE(sd.size() == 3);
			}
			THEN("operator[] returns each source value exactly") {
				REQUIRE(sd[0] == -0.75f);
				REQUIRE(sd[1] == 0.0f);
				REQUIRE(sd[2] == 0.5f);
			}
		}

		WHEN("a SampleData is constructed from a moved FloatBuffer") {
			FloatBuffer copy = fb;
			SampleData sd(std::move(copy));
			THEN("its format is F32") {
				REQUIRE(sd.getFormat() == SampleFormat::F32);
			}
			THEN("size() and values match the source") {
				REQUIRE(sd.size() == 3);
				REQUIRE(sd[0] == -0.75f);
				REQUIRE(sd[1] == 0.0f);
				REQUIRE(sd[2] == 0.5f);
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"Constructing from an Int16Buffer yields an I16-tagged buffer",
				"[sample-data]") {
	GIVEN("an Int16Buffer with three 16-bit samples") {
		Int16Buffer ib;
		ib.d = {-16384, 0, 16384};

		WHEN("a SampleData is constructed from it") {
			SampleData sd(ib);
			THEN("its format is I16") {
				REQUIRE(sd.getFormat() == SampleFormat::I16);
			}
			THEN("size() matches the source length") {
				REQUIRE(sd.size() == 3);
			}
			THEN("operator[] returns approximately the same floats Int16Buffer would") {
				REQUIRE(sd[0] == Catch::Approx(ib[0]).margin(i16tolerance));
				REQUIRE(sd[1] == Catch::Approx(ib[1]).margin(i16tolerance));
				REQUIRE(sd[2] == Catch::Approx(ib[2]).margin(i16tolerance));
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"Constructing from raw bytes adopts the given format and byte count",
				"[sample-data]") {
	GIVEN("six raw bytes tagged as I16") {
		std::vector<uint8_t> bytes = {0x00, 0x00, 0xff, 0x7f, 0x00, 0x80};
		const size_t inputBytes	   = bytes.size();
		WHEN("the SampleData is constructed from those bytes") {
			SampleData sd(std::move(bytes), SampleFormat::I16);
			THEN("getFormat() returns the supplied format") {
				REQUIRE(sd.getFormat() == SampleFormat::I16);
			}
			THEN("byteSize() matches the input byte count") {
				REQUIRE(sd.byteSize() == inputBytes);
			}
			THEN("size() == byteSize() / bytesPerSample()") {
				REQUIRE(sd.size() == inputBytes / sd.bytesPerSample());
			}
		}
	}

	GIVEN("eight raw bytes tagged as F32") {
		std::vector<uint8_t> bytes(8, 0);
		WHEN("the SampleData is constructed from those bytes") {
			SampleData sd(std::move(bytes), SampleFormat::F32);
			THEN("its size is two samples") {
				REQUIRE(sd.size() == 2);
			}
			THEN("bytesPerSample() == 4 for F32") {
				REQUIRE(sd.bytesPerSample() == 4);
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"bytesPerSample() agrees with the format tag for every format",
				"[sample-data]") {
	for (auto fmt: {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("an empty buffer in format " + formatName(fmt)) {
			auto sd = makeEmpty(fmt);
			THEN("bytesPerSample() == bytesPerSample(fmt)") {
				REQUIRE(sd.bytesPerSample() == bytesPerSample(fmt));
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"size() and byteSize() stay mutually consistent after resize",
				"[sample-data]") {
	for (auto fmt: {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("an empty buffer in format " + formatName(fmt)) {
			auto sd = makeEmpty(fmt);
			WHEN("resized to N samples") {
				const size_t N = 64;
				sd.resize(N);
				THEN("size() reports N") {
					REQUIRE(sd.size() == N);
				}
				THEN("byteSize() == N * bytesPerSample()") {
					REQUIRE(sd.byteSize() == N * sd.bytesPerSample());
				}
				THEN("empty() is false") {
					REQUIRE_FALSE(sd.empty());
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture, "clear() empties the buffer", "[sample-data]") {
	GIVEN("a non-empty SampleData") {
		auto sd = makeSized(SampleFormat::F32, 10);
		REQUIRE_FALSE(sd.empty());
		WHEN("clear() is called") {
			sd.clear();
			THEN("size() is zero") {
				REQUIRE(sd.size() == 0);
			}
			THEN("empty() is true") {
				REQUIRE(sd.empty());
			}
			THEN("byteSize() is zero") {
				REQUIRE(sd.byteSize() == 0);
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture, "resize grows with the default value and shrinks", "[sample-data]") {
	for (auto fmt: {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("a " + formatName(fmt) + " SampleData containing one written sample") {
			auto sd = makeSized(fmt, 1);
			sd.assignValue(0, 0.5f);
			WHEN("resized larger with default value 0") {
				sd.resize(4);
				THEN("size() reflects the new length") {
					REQUIRE(sd.size() == 4);
				}
				THEN("the previously-written sample is preserved") {
					REQUIRE(sd[0] == Catch::Approx(0.5f).margin(toleranceFor(fmt)));
				}
				THEN("appended tail samples are zero") {
					REQUIRE(sd[1] == Catch::Approx(0.0f).margin(toleranceFor(fmt)));
					REQUIRE(sd[2] == Catch::Approx(0.0f).margin(toleranceFor(fmt)));
					REQUIRE(sd[3] == Catch::Approx(0.0f).margin(toleranceFor(fmt)));
				}
			}
		}

		GIVEN("a four-sample " + formatName(fmt) + " buffer") {
			auto sd = filledRamp(fmt, 4, 0.1f, 0.1f);
			WHEN("resized smaller") {
				sd.resize(2);
				THEN("size() is the new smaller value") {
					REQUIRE(sd.size() == 2);
				}
				THEN("retained samples still match their pre-resize values") {
					REQUIRE(sd[0] == Catch::Approx(0.1f).margin(toleranceFor(fmt)));
					REQUIRE(sd[1] == Catch::Approx(0.2f).margin(toleranceFor(fmt)));
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture, "resize with a non-zero default fills new tail samples", "[sample-data]") {
	for (auto fmt: {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("an empty " + formatName(fmt) + " buffer") {
			auto sd = makeEmpty(fmt);
			WHEN("resized to 5 samples with default 0.25f") {
				sd.resize(5, 0.25f);
				THEN("every sample reads back as ~0.25f") {
					for (size_t i = 0; i < sd.size(); ++i) {
						REQUIRE(sd[i] == Catch::Approx(0.25f).margin(toleranceFor(fmt)));
					}
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture, "reserve increases capacity without changing size", "[sample-data]") {
	GIVEN("an empty F32 SampleData") {
		auto sd = makeEmpty(SampleFormat::F32);
		WHEN("reserve(128) is called") {
			sd.reserve(128);
			THEN("size() is still zero") {
				REQUIRE(sd.size() == 0);
			}
			THEN("capacity() is at least 128") {
				REQUIRE(sd.capacity() >= 128);
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"assignValue then operator[] round-trips within encoding precision",
				"[sample-data]") {
	const std::vector<float> testValues = {-1.0f, -0.75f, -0.5f, -0.1f, 0.0f, 0.1f, 0.5f, 0.75f, 1.0f};

	for (auto fmt: {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("a " + formatName(fmt) + " SampleData with one slot per test value") {
			auto sd = makeSized(fmt, testValues.size());
			WHEN("every test value is written and read back") {
				for (size_t i = 0; i < testValues.size(); ++i) {
					sd.assignValue(static_cast<int>(i), testValues[i]);
				}
				THEN("each read-back float is within the format's encoding tolerance") {
					for (size_t i = 0; i < testValues.size(); ++i) {
						REQUIRE(sd[i] == Catch::Approx(testValues[i]).margin(toleranceFor(fmt)));
					}
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture, "Integer formats clamp out-of-range floats on assignValue", "[sample-data]") {
	for (auto fmt: {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24}) {
		GIVEN("a two-sample " + formatName(fmt) + " buffer") {
			auto sd = makeSized(fmt, 2);
			WHEN("an out-of-range positive and negative value are assigned") {
				sd.assignValue(0, 2.0f);
				sd.assignValue(1, -2.0f);
				THEN("they read back clamped to [-1, 1]") {
					REQUIRE(sd[0] == Catch::Approx(1.0f).margin(toleranceFor(fmt)));
					REQUIRE(sd[1] == Catch::Approx(-1.0f).margin(toleranceFor(fmt)));
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture, "F32 stores values verbatim without clamping", "[sample-data]") {
	GIVEN("an F32 SampleData") {
		auto sd = makeSized(SampleFormat::F32, 2);
		WHEN("values outside [-1, 1] are assigned") {
			sd.assignValue(0, 2.5f);
			sd.assignValue(1, -3.25f);
			THEN("operator[] returns the exact stored values") {
				REQUIRE(sd[0] == 2.5f);
				REQUIRE(sd[1] == -3.25f);
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"Typed pointer accessors expose the same values operator[] does",
				"[sample-data]") {
	GIVEN("an F32 buffer with three known floats") {
		auto sd = makeSized(SampleFormat::F32, 3);
		sd.assignValue(0, -0.5f);
		sd.assignValue(1, 0.25f);
		sd.assignValue(2, 1.0f);
		THEN("asF32() returns a non-null pointer matching operator[]") {
			const float *p = sd.asF32();
			REQUIRE(p != nullptr);
			REQUIRE(p[0] == sd[0]);
			REQUIRE(p[1] == sd[1]);
			REQUIRE(p[2] == sd[2]);
		}
	}

	GIVEN("an I16 buffer with two known samples") {
		auto sd = makeSized(SampleFormat::I16, 2);
		sd.assignValue(0, -0.5f);
		sd.assignValue(1, 0.25f);
		THEN("asI16() gives a non-null typed pointer consistent with operator[]") {
			const int16_t *p = sd.asI16();
			REQUIRE(p != nullptr);
			REQUIRE(static_cast<float>(p[0]) / 32767.0f == Catch::Approx(sd[0]).margin(i16tolerance));
			REQUIRE(static_cast<float>(p[1]) / 32767.0f == Catch::Approx(sd[1]).margin(i16tolerance));
		}
	}

	GIVEN("an I8 buffer with two known samples") {
		auto sd = makeSized(SampleFormat::I8, 2);
		sd.assignValue(0, -1.0f);
		sd.assignValue(1, 0.5f);
		THEN("asI8() gives a non-null typed pointer consistent with operator[]") {
			const int8_t *p = sd.asI8();
			REQUIRE(p != nullptr);
			REQUIRE(static_cast<float>(p[0]) / 127.0f == Catch::Approx(sd[0]).margin(i8tolerance));
			REQUIRE(static_cast<float>(p[1]) / 127.0f == Catch::Approx(sd[1]).margin(i8tolerance));
		}
	}

	GIVEN("an I24 buffer with two known samples") {
		auto sd = makeSized(SampleFormat::I24, 2);
		sd.assignValue(0, -0.5f);
		sd.assignValue(1, 0.25f);
		THEN("asI24() gives a non-null pointer to 3-bytes-per-sample storage") {
			const uint8_t *p = sd.asI24();
			REQUIRE(p != nullptr);
			auto decode = [](const uint8_t *q) {
				int32_t v = (int32_t(int8_t(q[2])) << 16) | (int32_t(q[1]) << 8) | int32_t(q[0]);
				return v * (1.0f / 8388608.0f);
			};
			REQUIRE(decode(p + 0) == Catch::Approx(sd[0]).margin(i24tolerance));
			REQUIRE(decode(p + 3) == Catch::Approx(sd[1]).margin(i24tolerance));
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"bytes() returns a non-null pointer to byteSize() bytes when non-empty",
				"[sample-data]") {
	GIVEN("a non-empty buffer") {
		auto sd = makeSized(SampleFormat::I16, 4);
		THEN("bytes() is non-null") {
			REQUIRE(sd.bytes() != nullptr);
		}
		THEN("byteSize() agrees with size() * bytesPerSample()") {
			REQUIRE(sd.byteSize() == sd.size() * sd.bytesPerSample());
		}
	}
}

SCENARIO_METHOD(SampleDataFixture, "toFloatBuffer copies sample values out as floats", "[sample-data]") {
	GIVEN("an F32 SampleData") {
		auto sd = makeSized(SampleFormat::F32, 3);
		sd.assignValue(0, -0.25f);
		sd.assignValue(1, 0.0f);
		sd.assignValue(2, 0.5f);
		WHEN("toFloatBuffer() is called") {
			FloatBuffer fb = sd.toFloatBuffer();
			THEN("size matches") {
				REQUIRE(fb.size() == sd.size());
			}
			THEN("values are bit-exact for F32") {
				REQUIRE(fb[0] == -0.25f);
				REQUIRE(fb[1] == 0.0f);
				REQUIRE(fb[2] == 0.5f);
			}
		}
	}

	GIVEN("an I16 SampleData") {
		auto sd = makeSized(SampleFormat::I16, 3);
		sd.assignValue(0, -0.5f);
		sd.assignValue(1, 0.25f);
		sd.assignValue(2, 1.0f);
		WHEN("toFloatBuffer() is called") {
			FloatBuffer fb = sd.toFloatBuffer();
			THEN("size matches and values are within I16 precision") {
				REQUIRE(fb.size() == sd.size());
				REQUIRE(fb[0] == Catch::Approx(-0.5f).margin(i16tolerance));
				REQUIRE(fb[1] == Catch::Approx(0.25f).margin(i16tolerance));
				REQUIRE(fb[2] == Catch::Approx(1.0f).margin(i16tolerance));
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"append into an empty SampleData adopts the source format and values",
				"[sample-data]") {
	GIVEN("an empty SampleData and a non-empty I16 source") {
		SampleData dst;
		auto src = makeSized(SampleFormat::I16, 3);
		src.assignValue(0, -0.5f);
		src.assignValue(1, 0.0f);
		src.assignValue(2, 0.5f);

		WHEN("the source is appended to the destination") {
			dst.append(src);
			THEN("the destination takes on the source's format") {
				REQUIRE(dst.getFormat() == SampleFormat::I16);
			}
			THEN("the destination has the same size and values as the source") {
				REQUIRE(dst.size() == src.size());
				REQUIRE(dst[0] == Catch::Approx(src[0]).margin(i16tolerance));
				REQUIRE(dst[1] == Catch::Approx(src[1]).margin(i16tolerance));
				REQUIRE(dst[2] == Catch::Approx(src[2]).margin(i16tolerance));
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture, "append between same-format SampleData concatenates", "[sample-data]") {
	for (auto fmt: {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("two " + formatName(fmt) + " SampleData buffers") {
			auto a = filledRamp(fmt, 2, 0.1f, 0.2f);
			auto b = filledRamp(fmt, 3, -0.2f, -0.1f);
			WHEN("b is appended to a") {
				a.append(b);
				THEN("the size grows by b's size") {
					REQUIRE(a.size() == 5);
				}
				THEN("the format is unchanged") {
					REQUIRE(a.getFormat() == fmt);
				}
				THEN("values from a are preserved at the front") {
					REQUIRE(a[0] == Catch::Approx(0.1f).margin(toleranceFor(fmt)));
					REQUIRE(a[1] == Catch::Approx(0.3f).margin(toleranceFor(fmt)));
				}
				THEN("values from b are present in order at the back") {
					REQUIRE(a[2] == Catch::Approx(-0.2f).margin(toleranceFor(fmt)));
					REQUIRE(a[3] == Catch::Approx(-0.3f).margin(toleranceFor(fmt)));
					REQUIRE(a[4] == Catch::Approx(-0.4f).margin(toleranceFor(fmt)));
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"append across formats transcodes into the destination's format",
				"[sample-data]") {
	GIVEN("an F32 destination and an I16 source") {
		auto dst = makeSized(SampleFormat::F32, 1);
		dst.assignValue(0, 0.5f);
		auto src = makeSized(SampleFormat::I16, 2);
		src.assignValue(0, -0.25f);
		src.assignValue(1, 0.75f);

		WHEN("the source is appended") {
			dst.append(src);
			THEN("the destination format stays F32") {
				REQUIRE(dst.getFormat() == SampleFormat::F32);
			}
			THEN("size grows by the source size") {
				REQUIRE(dst.size() == 3);
			}
			THEN("the original destination value survives") {
				REQUIRE(dst[0] == 0.5f);
			}
			THEN("the appended values are present within the worst-case precision") {
				REQUIRE(dst[1] == Catch::Approx(-0.25f).margin(i16tolerance));
				REQUIRE(dst[2] == Catch::Approx(0.75f).margin(i16tolerance));
			}
		}
	}

	GIVEN("an I16 destination and an F32 source") {
		auto dst = makeSized(SampleFormat::I16, 1);
		dst.assignValue(0, 0.5f);
		auto src = makeSized(SampleFormat::F32, 2);
		src.assignValue(0, -0.25f);
		src.assignValue(1, 0.75f);

		WHEN("the source is appended") {
			dst.append(src);
			THEN("the destination format stays I16") {
				REQUIRE(dst.getFormat() == SampleFormat::I16);
			}
			THEN("size grows by the source size") {
				REQUIRE(dst.size() == 3);
			}
			THEN("the new tail values are present within I16 precision") {
				REQUIRE(dst[0] == Catch::Approx(0.5f).margin(i16tolerance));
				REQUIRE(dst[1] == Catch::Approx(-0.25f).margin(i16tolerance));
				REQUIRE(dst[2] == Catch::Approx(0.75f).margin(i16tolerance));
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture, "append(FloatBuffer) appends float samples", "[sample-data]") {
	GIVEN("an F32 SampleData and a FloatBuffer") {
		auto sd = makeSized(SampleFormat::F32, 1);
		sd.assignValue(0, 0.1f);
		FloatBuffer fb;
		fb.push_back(0.2f);
		fb.push_back(0.3f);

		WHEN("the FloatBuffer is appended") {
			sd.append(fb);
			THEN("size grows by the FloatBuffer size") {
				REQUIRE(sd.size() == 3);
			}
			THEN("values are preserved exactly in F32") {
				REQUIRE(sd[0] == 0.1f);
				REQUIRE(sd[1] == 0.2f);
				REQUIRE(sd[2] == 0.3f);
			}
		}
	}

	GIVEN("an I16 SampleData and a FloatBuffer") {
		auto sd = makeSized(SampleFormat::I16, 1);
		sd.assignValue(0, 0.1f);
		FloatBuffer fb;
		fb.push_back(0.2f);
		fb.push_back(-0.3f);

		WHEN("the FloatBuffer is appended") {
			sd.append(fb);
			THEN("the destination format stays I16") {
				REQUIRE(sd.getFormat() == SampleFormat::I16);
			}
			THEN("size grows correctly and values are within I16 precision") {
				REQUIRE(sd.size() == 3);
				REQUIRE(sd[1] == Catch::Approx(0.2f).margin(i16tolerance));
				REQUIRE(sd[2] == Catch::Approx(-0.3f).margin(i16tolerance));
			}
		}
	}

	GIVEN("an empty SampleData and a non-empty FloatBuffer") {
		SampleData sd;
		FloatBuffer fb;
		fb.push_back(0.5f);
		fb.push_back(-0.5f);

		WHEN("the FloatBuffer is appended") {
			sd.append(fb);
			THEN("the destination format becomes F32") {
				REQUIRE(sd.getFormat() == SampleFormat::F32);
			}
			THEN("values are present exactly") {
				REQUIRE(sd.size() == 2);
				REQUIRE(sd[0] == 0.5f);
				REQUIRE(sd[1] == -0.5f);
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"splice extracts a half-open sample range into a new SampleData",
				"[sample-data]") {
	for (auto fmt: {SampleFormat::I8, SampleFormat::I16, SampleFormat::I24, SampleFormat::F32}) {
		GIVEN("a 5-sample " + formatName(fmt) + " ramp") {
			auto sd = filledRamp(fmt, 5, 0.0f, 0.2f);

			WHEN("splice(1, 4) is called") {
				SampleData out = sd.splice(1, 4);
				THEN("the result has the same format") {
					REQUIRE(out.getFormat() == fmt);
				}
				THEN("the result has size (end - start)") {
					REQUIRE(out.size() == 3);
				}
				THEN("the result contains the half-open sample range") {
					REQUIRE(out[0] == Catch::Approx(0.2f).margin(toleranceFor(fmt)));
					REQUIRE(out[1] == Catch::Approx(0.4f).margin(toleranceFor(fmt)));
					REQUIRE(out[2] == Catch::Approx(0.6f).margin(toleranceFor(fmt)));
				}
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"mix performs per-sample addition with another same-format SampleData",
				"[sample-data]") {
	GIVEN("two F32 SampleData of the same length") {
		auto a = makeSized(SampleFormat::F32, 3);
		a.assignValue(0, 0.1f);
		a.assignValue(1, -0.2f);
		a.assignValue(2, 0.3f);
		auto b = makeSized(SampleFormat::F32, 3);
		b.assignValue(0, 0.05f);
		b.assignValue(1, 0.4f);
		b.assignValue(2, -0.1f);

		WHEN("b is mixed into a") {
			a.mix(b);
			THEN("each sample is the sum of the two inputs") {
				REQUIRE(a[0] == Catch::Approx(0.15f));
				REQUIRE(a[1] == Catch::Approx(0.2f));
				REQUIRE(a[2] == Catch::Approx(0.2f));
			}
		}
	}

	GIVEN("a non-empty SampleData and an empty other") {
		auto a = makeSized(SampleFormat::F32, 2);
		a.assignValue(0, 0.5f);
		a.assignValue(1, -0.5f);
		SampleData empty;

		WHEN("the empty other is mixed in") {
			a.mix(empty);
			THEN("a is unchanged") {
				REQUIRE(a[0] == 0.5f);
				REQUIRE(a[1] == -0.5f);
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture, "normalizeAudio leaves silent buffers unchanged", "[sample-data]") {
	GIVEN("a SampleData full of zeros") {
		auto sd = makeSized(SampleFormat::F32, 4);
		WHEN("normalizeAudio() is called") {
			sd.normalizeAudio();
			THEN("every sample is still zero") {
				for (size_t i = 0; i < sd.size(); ++i)
					REQUIRE(sd[i] == 0.0f);
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"normalizeAudio is a no-op when a sample already reaches full scale",
				"[sample-data]") {
	GIVEN("a SampleData whose peak is exactly 1.0") {
		auto sd = makeSized(SampleFormat::F32, 3);
		sd.assignValue(0, 0.3f);
		sd.assignValue(1, 1.0f);
		sd.assignValue(2, -0.5f);
		WHEN("normalizeAudio() is called") {
			sd.normalizeAudio();
			THEN("values are unchanged") {
				REQUIRE(sd[0] == 0.3f);
				REQUIRE(sd[1] == 1.0f);
				REQUIRE(sd[2] == -0.5f);
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"normalizeAudio scales sub-peak buffers so the loudest sample hits full scale",
				"[sample-data]") {
	GIVEN("a buffer whose peak magnitude is 0.5") {
		auto sd = makeSized(SampleFormat::F32, 4);
		sd.assignValue(0, 0.1f);
		sd.assignValue(1, 0.5f);
		sd.assignValue(2, -0.25f);
		sd.assignValue(3, 0.0f);
		WHEN("normalizeAudio() is called") {
			sd.normalizeAudio();
			THEN("the peak is now ~1.0 and other samples scale by the same factor") {
				REQUIRE(sd[0] == Catch::Approx(0.2f).margin(1e-6f));
				REQUIRE(sd[1] == Catch::Approx(1.0f).margin(1e-6f));
				REQUIRE(sd[2] == Catch::Approx(-0.5f).margin(1e-6f));
				REQUIRE(sd[3] == Catch::Approx(0.0f).margin(1e-6f));
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"findFirstOnset returns nullopt when nothing exceeds the threshold",
				"[sample-data]") {
	GIVEN("a silent buffer") {
		auto sd = makeSized(SampleFormat::F32, 8);
		WHEN("findFirstOnset is called with any positive threshold") {
			auto r = sd.findFirstOnset(1, 0.0001f);
			THEN("the result is nullopt") {
				REQUIRE_FALSE(r.has_value());
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"findFirstOnset returns the first frame index whose magnitude exceeds the threshold",
				"[sample-data]") {
	GIVEN("a mono buffer with the first loud sample at frame 5") {
		auto sd = makeSized(SampleFormat::F32, 8);
		sd.assignValue(5, 0.4f);
		WHEN("findFirstOnset(1, 0.1) is called") {
			auto r = sd.findFirstOnset(1, 0.1f);
			THEN("it returns frame 5") {
				REQUIRE(r.has_value());
				REQUIRE(*r == 5u);
			}
		}
	}

	GIVEN("a stereo buffer where only the R channel of frame 3 is loud") {
		auto sd = makeSized(SampleFormat::F32, 8);
		sd.assignValue(7, 0.5f);
		WHEN("findFirstOnset(2, 0.1) is called") {
			auto r = sd.findFirstOnset(2, 0.1f);
			THEN("the returned index is a frame index (3), not a sample index") {
				REQUIRE(r.has_value());
				REQUIRE(*r == 3u);
			}
		}
	}
}

SCENARIO_METHOD(SampleDataFixture,
				"findFirstOnset uses a strict-greater comparison against the threshold",
				"[sample-data]") {
	GIVEN("a buffer whose only non-zero sample equals the threshold exactly") {
		auto sd = makeSized(SampleFormat::F32, 4);
		sd.assignValue(2, 0.25f);
		WHEN("findFirstOnset is called with threshold 0.25") {
			auto r = sd.findFirstOnset(1, 0.25f);
			THEN("nothing is detected") {
				REQUIRE_FALSE(r.has_value());
			}
		}
		WHEN("findFirstOnset is called with a threshold just below 0.25") {
			auto r = sd.findFirstOnset(1, 0.24f);
			THEN("the loud sample is detected") {
				REQUIRE(r.has_value());
				REQUIRE(*r == 2u);
			}
		}
	}
}
