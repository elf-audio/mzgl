#include "tests.h"
#include "DateTime.h"
#include <ctime>
#include "log.h"

TEST_CASE("datetime tests", "[datetime]") {
	auto now		   = DateTime::now();
	std::time_t t	   = std::time(nullptr); // Get the current time
	std::tm *localTime = std::localtime(&t); // Convert to local time
	int year		   = 1900 + localTime->tm_year; // Extract the year
	REQUIRE(year == now.year);
	DateTime inThePast(2023, 1, 1);
	DateTime inTheFuture(3000, 12, 31);
	REQUIRE(inThePast < now);
	REQUIRE(now < inTheFuture);
	for (int y = 1900; y < 3000; y++) {
		bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
		REQUIRE(DateTime::isLeapYear(y) == leap);
	}
}