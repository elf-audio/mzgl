#include "tests.h"

#include "FloatBuffer.h"

SCENARIO("FloatBuffer can be created and accessed", "[float-buffer]") {
	GIVEN("A FloatBuffer") {
		FloatBuffer buf(10);
		REQUIRE(buf.size() == 10);
		REQUIRE(buf[0] == 0.0f);
		REQUIRE(buf[9] == 0.0f);
		WHEN("Values are set") {
			buf[0] = 1.0f;
			buf[9] = 2.0f;
			THEN("Values are retrieved") {
				REQUIRE(buf[0] == 1.0f);
				REQUIRE(buf[9] == 2.0f);
			}
		}
	}
}

SCENARIO("FloatBuffer can be interpolated", "[float-buffer]") {
	GIVEN("A FloatBuffer") {
		FloatBuffer buf(10);
		for (int i = 0; i < 10; i++) {
			buf[i] = i;
		}
		WHEN("Interpolated") {
			float val = buf.interpolate(0.5);
			THEN("Value is interpolated") {
				REQUIRE(val == 0.5);
			}
		}

		buf[3 * 2] = 0.f;
		buf[4 * 2] = 1.f;

		buf[3 * 2 + 1] = 5.f;
		buf[4 * 2 + 1] = 6.f;

		WHEN("Stereo interpolated") {
			float L, R;
			buf.interpolateStereo(3.5, L, R);
			THEN("Values are interpolated") {
				REQUIRE(L == 0.5);
				REQUIRE(R == 5.5);
			}
		}
	}
}
// stereoGain: every stereo pair scaled, nothing written past the end, for every block size.
// Block sizes that aren't a multiple of 4 frames happen on Android (Oboe picks the callback
// size) and that is exactly where the NEON tail loop used to go wrong.
namespace {
	constexpr float stereoGainSentinel = 12345.f;
	constexpr size_t stereoGainPadding = 16;

	std::vector<float> stereoGainInput(size_t size) {
		std::vector<float> v(size + stereoGainPadding, stereoGainSentinel);
		for (size_t i = 0; i < size; ++i)
			v[i] = 1.f + static_cast<float>(i);
		return v;
	}

	void checkStereoGainResult(
		const std::vector<float> &in, const std::vector<float> &out, size_t size, float l, float r) {
		for (size_t i = 0; i + 1 < size; i += 2) {
			INFO("size " << size << " index " << i);
			CHECK(out[i] == Catch::Approx(in[i] * l));
			CHECK(out[i + 1] == Catch::Approx(in[i + 1] * r));
		}
		if (size % 2 == 1) {
			INFO("odd trailing float at " << size - 1);
			CHECK(out[size - 1] == in[size - 1]);
		}
		for (size_t i = size; i < in.size(); ++i) {
			INFO("size " << size << " past-the-end index " << i);
			CHECK(out[i] == stereoGainSentinel);
		}
	}
} // namespace

TEST_CASE("FloatBuffer::stereoGain scales every pair for any block size", "[float-buffer][stereoGain]") {
	const float l = 0.5f, r = 0.25f;
	for (size_t size = 0; size <= 40; ++size) {
		const auto in = stereoGainInput(size);
		FloatBuffer b(in.data(), size);
		b.stereoGain(l, r);
		std::vector<float> out(in);
		std::copy(b.begin(), b.end(), out.begin());
		checkStereoGainResult(in, out, size, l, r);
	}
}

TEST_CASE("floatbuffer_kernels::stereoGainScalar", "[float-buffer][stereoGain]") {
	const float l = 0.5f, r = 0.25f;
	for (size_t size = 0; size <= 40; ++size) {
		const auto in = stereoGainInput(size);
		auto out	  = in;
		floatbuffer_kernels::stereoGainScalar(out.data(), size, l, r);
		checkStereoGainResult(in, out, size, l, r);
	}
}

#ifdef __ARM_NEON
TEST_CASE("floatbuffer_kernels::stereoGainNeon matches scalar and stays in bounds", "[float-buffer][stereoGain]") {
	const float l = 0.5f, r = 0.25f;
	for (size_t size = 0; size <= 40; ++size) {
		const auto in = stereoGainInput(size);
		auto out	  = in;
		floatbuffer_kernels::stereoGainNeon(out.data(), size, l, r);
		checkStereoGainResult(in, out, size, l, r);
	}
}
#endif

namespace {
	FloatBuffer fb(std::vector<float> v) {
		return FloatBuffer(v.data(), v.size());
	}

	FloatBuffer makeRamp(size_t size, float start = 1.f) {
		FloatBuffer b(size);
		for (size_t i = 0; i < size; ++i)
			b[i] = start + static_cast<float>(i);
		return b;
	}
} // namespace

TEST_CASE("FloatBuffer min/max family on positive, negative and mixed data", "[float-buffer][minmax]") {
	SECTION("all negative: max must not be numeric_limits<float>::min()") {
		FloatBuffer b = fb({-3.f, -1.f, -7.f, -2.f});
		CHECK(b.maxValue() == -1.f);
		CHECK(b.findAbsMaxPos() == 2);
		float mn = 0, mx = 0;
		b.getMinMax(mn, mx);
		CHECK(mn == -7.f);
		CHECK(mx == -1.f);
	}
	SECTION("all positive") {
		FloatBuffer b = fb({3.f, 1.f, 7.f, 2.f});
		CHECK(b.maxValue() == 7.f);
		float mn = 0, mx = 0;
		b.getMinMax(mn, mx);
		CHECK(mn == 1.f);
		CHECK(mx == 7.f);
	}
	SECTION("mixed, odd length so SIMD tails are exercised") {
		FloatBuffer b = fb({0.5f, -2.f, 1.5f, -0.25f, 1.f});
		CHECK(b.maxValue() == 1.5f);
		CHECK(b.findAbsMaxPos() == 1);
	}
	SECTION("empty buffer does not crash the reducers") {
		FloatBuffer b;
		float mn = 0, mx = 0;
		b.getMinMax(mn, mx);
		CHECK(mn > mx); // sentinel "nothing seen" state
	}
}

TEST_CASE("FloatBuffer normalize variants", "[float-buffer][normalize]") {
	SECTION("normalizeAudio on an all-negative signal peaks at -1") {
		FloatBuffer b = fb({-0.1f, -0.5f, -0.25f});
		b.normalizeAudio();
		CHECK(b[1] == Catch::Approx(-1.f));
		CHECK(b[0] == Catch::Approx(-0.2f));
	}
	SECTION("normalizeAudio on a positive-peaked signal") {
		FloatBuffer b = fb({0.1f, -0.2f, 0.4f});
		b.normalizeAudio();
		CHECK(b[2] == Catch::Approx(1.f));
		CHECK(b[1] == Catch::Approx(-0.5f));
	}
	SECTION("normalize(min,max) maps the range, including all-negative input") {
		FloatBuffer b = fb({-4.f, -2.f, -3.f});
		b.normalize(0.f, 1.f);
		CHECK(b[0] == Catch::Approx(0.f));
		CHECK(b[1] == Catch::Approx(1.f));
		CHECK(b[2] == Catch::Approx(0.5f));
	}
	SECTION("normalize of a constant or empty buffer does not produce NaN") {
		FloatBuffer b = fb({0.3f, 0.3f, 0.3f});
		b.normalize(-1.f, 1.f);
		for (auto v: b)
			CHECK(!std::isnan(v));
		FloatBuffer e;
		e.normalize();
		CHECK(e.empty());
	}
}

TEST_CASE("FloatBuffer fadeIn / fadeOut", "[float-buffer][fade]") {
	SECTION("stereo fadeIn scales the first frames only") {
		FloatBuffer b(10, 1.f); // 5 stereo frames
		b.fadeIn(4, 2);
		CHECK(b[0] == 0.f);
		CHECK(b[1] == 0.f);
		CHECK(b[2] == Catch::Approx(0.25f));
		CHECK(b[3] == Catch::Approx(0.25f));
		CHECK(b[6] == Catch::Approx(0.75f));
		CHECK(b[8] == 1.f);
		CHECK(b[9] == 1.f);
	}
	SECTION("stereo fadeIn longer than the buffer is refused and leaves it untouched") {
		FloatBuffer b(10, 1.f);
		b.fadeIn(8, 2); // 8 frames = 16 floats > 10
		for (auto v: b)
			CHECK(v == 1.f);
	}
	SECTION("fadeIn exactly the buffer length is allowed") {
		FloatBuffer b(10, 1.f);
		b.fadeIn(5, 2);
		CHECK(b[0] == 0.f);
		CHECK(b[8] == Catch::Approx(0.8f));
	}
	SECTION("mono fadeOut scales the last samples, last one to zero") {
		FloatBuffer b(8, 1.f);
		b.fadeOut(4, 1);
		CHECK(b[7] == 0.f);
		CHECK(b[6] == Catch::Approx(0.25f));
		CHECK(b[4] == Catch::Approx(0.75f));
		CHECK(b[3] == 1.f);
	}
	SECTION("stereo fadeOut") {
		FloatBuffer b(10, 1.f);
		b.fadeOut(2, 2);
		CHECK(b[8] == 0.f);
		CHECK(b[9] == 0.f);
		CHECK(b[6] == Catch::Approx(0.5f));
		CHECK(b[7] == Catch::Approx(0.5f));
		CHECK(b[5] == 1.f);
	}
	SECTION("fadeOut longer than the buffer is refused") {
		FloatBuffer b(10, 1.f);
		b.fadeOut(6, 2);
		for (auto v: b)
			CHECK(v == 1.f);
	}
	SECTION("smooth fade uses smoothstep (midpoint 0.5, symmetric)") {
		FloatBuffer b(4, 1.f);
		b.fadeIn(4, 1, true);
		CHECK(b[0] == 0.f);
		CHECK(b[2] == Catch::Approx(0.5f));
		CHECK(b[1] < 0.25f); // smoothstep(0.25) = 0.156
	}
}

TEST_CASE("FloatBuffer stereo/mono conversions round-trip", "[float-buffer][channels]") {
	FloatBuffer stereo = fb({1.f, 10.f, 2.f, 20.f, 3.f, 30.f});
	SECTION("splitStereo / combineStereo") {
		FloatBuffer l, r, back;
		stereo.splitStereo(l, r);
		REQUIRE(l.size() == 3);
		CHECK(l == fb({1.f, 2.f, 3.f}));
		CHECK(r == fb({10.f, 20.f, 30.f}));
		back.combineStereo(l, r);
		CHECK(back == stereo);
	}
	SECTION("setMonoFromStereo averages, setStereoFromMono duplicates") {
		FloatBuffer mono;
		mono.setMonoFromStereo(stereo);
		CHECK(mono == fb({5.5f, 11.f, 16.5f}));
		FloatBuffer dup;
		dup.setStereoFromMono(mono);
		CHECK(dup == fb({5.5f, 5.5f, 11.f, 11.f, 16.5f, 16.5f}));
	}
	SECTION("convertToMono handles 4 channels and odd leftovers") {
		FloatBuffer quad = fb({1.f, 2.f, 3.f, 4.f, 10.f, 20.f, 30.f, 40.f, 99.f});
		quad.convertToMono(4);
		CHECK(quad == fb({2.5f, 25.f}));
	}
	SECTION("getMonoAsStereo") {
		FloatBuffer mono = fb({1.f, 2.f});
		float out[4]	 = {0, 0, 0, 0};
		mono.getMonoAsStereo(out, 2);
		CHECK(out[0] == 1.f);
		CHECK(out[1] == 1.f);
		CHECK(out[2] == 2.f);
		CHECK(out[3] == 2.f);
	}
}

TEST_CASE("FloatBuffer arithmetic matches a scalar reference on odd sizes", "[float-buffer][arith]") {
	// 7 is deliberately not a multiple of any SIMD width
	const auto a = makeRamp(7, 1.f);
	const auto b = makeRamp(7, 0.5f);
	SECTION("buffer op buffer") {
		auto s = a;
		s += b;
		auto d = a;
		d -= b;
		auto m = a;
		m *= b;
		for (size_t i = 0; i < a.size(); ++i) {
			CHECK(s[i] == Catch::Approx(a[i] + b[i]));
			CHECK(d[i] == Catch::Approx(a[i] - b[i]));
			CHECK(m[i] == Catch::Approx(a[i] * b[i]));
		}
	}
	SECTION("buffer op scalar") {
		auto s = a;
		s += 2.f;
		auto d = a;
		d -= 2.f;
		auto m = a;
		m *= 2.f;
		auto q = a;
		q /= 2.f;
		for (size_t i = 0; i < a.size(); ++i) {
			CHECK(s[i] == Catch::Approx(a[i] + 2.f));
			CHECK(d[i] == Catch::Approx(a[i] - 2.f));
			CHECK(m[i] == Catch::Approx(a[i] * 2.f));
			CHECK(q[i] == Catch::Approx(a[i] / 2.f));
		}
	}
	SECTION("clamp") {
		FloatBuffer c = fb({-2.f, -0.5f, 0.f, 0.5f, 2.f});
		c.clamp(-1.f, 1.f);
		CHECK(c == fb({-1.f, -0.5f, 0.f, 0.5f, 1.f}));
	}
	SECTION("zeros in place and with resize") {
		auto z = a;
		z.zeros();
		CHECK(z.size() == 7);
		for (auto v: z)
			CHECK(v == 0.f);
		z.zeros(3);
		CHECK(z.size() == 3);
		for (auto v: z)
			CHECK(v == 0.f);
	}
}

TEST_CASE("FloatBuffer mix / append / splice", "[float-buffer][edit]") {
	SECTION("mix grows to the longer buffer and adds") {
		FloatBuffer a = fb({1.f, 2.f});
		FloatBuffer b = fb({10.f, 20.f, 30.f});
		a.mix(b);
		CHECK(a == fb({11.f, 22.f, 30.f}));
		FloatBuffer c = fb({1.f, 2.f, 3.f, 4.f});
		c.mix(fb({10.f}));
		CHECK(c == fb({11.f, 2.f, 3.f, 4.f}));
	}
	SECTION("append") {
		FloatBuffer a = fb({1.f});
		a.append(fb({2.f, 3.f}));
		float more[2] = {4.f, 5.f};
		a.append(more, 2);
		CHECK(a == fb({1.f, 2.f, 3.f, 4.f, 5.f}));
	}
	SECTION("splice end is exclusive, both overloads agree") {
		const auto r	= makeRamp(6);
		FloatBuffer out = fb({99.f});
		r.splice(1, 4, out);
		CHECK(out == fb({2.f, 3.f, 4.f}));
		CHECK(r.splice(1, 4) == out);
		CHECK(r.splice(2, 2).empty());
	}
	SECTION("get copies") {
		const auto r = makeRamp(3);
		float out[3] = {0, 0, 0};
		r.get(out);
		CHECK(out[2] == 3.f);
		float two[2] = {0, 0};
		r.get(two, 2);
		CHECK(two[1] == 2.f);
	}
}

TEST_CASE("FloatBuffer interpolation edge cases", "[float-buffer][interp]") {
	const auto r = makeRamp(4); // 1 2 3 4
	SECTION("linear inside, at the end, beyond, before, size 1") {
		CHECK(r.interpolate(1.25) == Catch::Approx(2.25f));
		CHECK(r.interpolate(3.0) == 4.f);
		CHECK(r.interpolate(3.7) == 4.f); // clamps to last sample
		CHECK(r.interpolate(10.0) == 4.f);
		CHECK(r.interpolate(-0.5) == Catch::Approx(1.f).margin(0.6f)); // clamps to the start
		FloatBuffer one = fb({7.f});
		CHECK(one.interpolate(0.3) == 7.f);
		CHECK(one.interpolate(-1.0) == 7.f);
	}
	SECTION("wrapping interpolation wraps at the end") {
		CHECK(r.interpolateWrapping(1.5) == Catch::Approx(2.5f));
		CHECK(r.interpolateWrapping(3.5) == Catch::Approx(2.5f)); // between 4 and 1
	}
	SECTION("stereo linear at and past the end returns something finite") {
		FloatBuffer st = fb({1.f, 10.f, 2.f, 20.f, 3.f, 30.f});
		float L = 0, R = 0;
		st.interpolateStereo(2.0, L, R);
		CHECK(std::isfinite(L));
		CHECK(std::isfinite(R));
		st.interpolateStereo(5.0, L, R);
		CHECK(std::isfinite(L));
		st.interpolateStereo(-1.0, L, R);
		CHECK(L == Catch::Approx(1.f));
		CHECK(R == Catch::Approx(10.f));
	}
}

TEST_CASE("FloatBuffer findFirstOnset", "[float-buffer][onset]") {
	FloatBuffer b = fb({0.f, 0.01f, -0.04f, 0.2f, 0.9f});
	CHECK(b.findFirstOnset() == 3);
	CHECK(b.findFirstOnset(0.03f) == 2);
	CHECK(!b.findFirstOnset(1.f).has_value());
	CHECK(!FloatBuffer {}.findFirstOnset().has_value());
}
