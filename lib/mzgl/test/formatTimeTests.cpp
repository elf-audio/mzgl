#include "tests.h"
#include "stringUtil.h"
#include "util.h"
#include <chrono>
#include <regex>

TEST_CASE("formatTime writes a time out with a strftime format", "[util][time]") {
	// 2026-11-14 09:46:38 UTC
	const std::time_t t = 1794649598;
	REQUIRE(formatTime(t, "%Y-%m-%d %H:%M:%S", TimeZone::UTC) == "2026-11-14 09:46:38");
	REQUIRE(formatTime(t, "%d %b %Y", TimeZone::UTC) == "14 Nov 2026");
	REQUIRE(formatTime(0, "%Y-%m-%d %H:%M:%S", TimeZone::UTC) == "1970-01-01 00:00:00");

	// local time is whatever the machine says, but always the same shape
	REQUIRE(std::regex_match(formatTime(t, "%Y-%m-%d %H:%M:%S"), std::regex(R"(\d{4}-\d\d-\d\d \d\d:\d\d:\d\d)")));
}

TEST_CASE("the ready-made timestamps have the shapes their users rely on", "[util][time]") {
	REQUIRE(std::regex_match(utcTimestampString(), std::regex(R"(\d{4}-\d\d-\d\d \d\d:\d\d:\d\d UTC)")));
	REQUIRE(std::regex_match(timestampString(), std::regex(R"(\d{4}-\d\d-\d\d \d\d:\d\d:\d\d)")));

	// nothing a file system would object to
	const auto forFilename = filenameTimestampString();
	REQUIRE(std::regex_match(forFilename, std::regex(R"(\d{4}-\d\d-\d\d--\d\d\.\d\d\.\d\d)")));
	REQUIRE(forFilename.find(':') == std::string::npos);
	REQUIRE(forFilename.find(' ') == std::string::npos);
}

TEST_CASE("a time point survives being written and read back", "[util][time]") {
	// to the second, which is all the text form holds
	const auto now = std::chrono::system_clock::from_time_t(std::time(nullptr));
	REQUIRE(string_to_time_point(time_point_to_string(now)) == now);

	// in winter and in summer: where clocks change, a summer time used to come back an hour late
	const auto january = std::chrono::system_clock::from_time_t(1768478400); // 2026-01-15 12:00 UTC
	const auto july	   = std::chrono::system_clock::from_time_t(1784116800); // 2026-07-15 12:00 UTC
	REQUIRE(string_to_time_point(time_point_to_string(january)) == january);
	REQUIRE(string_to_time_point(time_point_to_string(july)) == july);
}
