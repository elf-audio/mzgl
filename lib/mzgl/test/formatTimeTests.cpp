#include "tests.h"
#include "util.h"
#include <regex>

namespace {
	// 2026-11-14 09:46:38 UTC, and a winter and a summer noon (where clocks change, those differ)
	constexpr std::time_t november = 1794649598;
	constexpr std::time_t january  = 1768478400; // 2026-01-15 12:00:00 UTC
	constexpr std::time_t july	   = 1784116800; // 2026-07-15 12:00:00 UTC
} // namespace

TEST_CASE("a timestamp is UTC and says so", "[util][time]") {
	REQUIRE(timestampString(november) == "2026-11-14 09:46:38 UTC");
	REQUIRE(timestampString(0) == "1970-01-01 00:00:00 UTC");
	REQUIRE(timestampString(july) == "2026-07-15 12:00:00 UTC");
	REQUIRE(std::regex_match(timestampString(), std::regex(R"(\d{4}-\d\d-\d\d \d\d:\d\d:\d\d UTC)")));
}

TEST_CASE("a timestamp reads back as the time it was written from", "[util][time]") {
	for (auto time: {november, january, july, std::time(nullptr)}) {
		REQUIRE(timeFromTimestampString(timestampString(time)) == time);
	}
	REQUIRE(timeFromTimestampString("2026-11-14 09:46:38 UTC") == november);
	REQUIRE(timeFromTimestampString("not a time") == 0);
	REQUIRE(timeFromTimestampString("") == 0);
}

TEST_CASE("an old timestamp with no zone on it is read as local time", "[util][time]") {
	// what used to be written: the same text, minus " UTC", in whatever zone the
	// machine was in. Build one for this machine and check it lands on the right
	// instant - in summer too, which used to come back an hour late.
	for (auto time: {january, july}) {
		std::tm local {};
#ifdef _WIN32
		localtime_s(&local, &time);
#else
		localtime_r(&time, &local);
#endif
		char text[32];
		std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", &local);
		REQUIRE(timeFromTimestampString(text) == time);
	}
}

TEST_CASE("the filename timestamp has nothing a file system would object to", "[util][time]") {
	const auto stamp = filenameTimestampString();
	REQUIRE(std::regex_match(stamp, std::regex(R"(\d{4}-\d\d-\d\d--\d\d\.\d\d\.\d\d)")));
	REQUIRE(stamp.find(':') == std::string::npos);
	REQUIRE(stamp.find(' ') == std::string::npos);
}

TEST_CASE("the friendly time is day, month name, year, hours and minutes", "[util][time]") {
	// local time, so only the shape is fixed
	REQUIRE(std::regex_match(friendlyTimeString(november), std::regex(R"(\d\d [A-Z][a-z]{2} \d{4} \d\d:\d\d)")));
}
