#include "tests.h"
#include "stringUtil.h"

struct StartsWithTestCase {
	std::string stringToSearch;
	std::string prefix;
	CaseSensitivity caseSensitivity;
	bool expected;
};

struct EndsWithTestCase {
	std::string stringToSearch;
	std::string suffix;
	CaseSensitivity caseSensitivity;
	bool expected;
};

struct CombinedTestCase {
	std::string stringToSearch;
	std::string prefix;
	std::string suffix;
	CaseSensitivity caseSensitivity;
	bool expectedStartsWith;
	bool expectedEndsWith;
};

SCENARIO("Checking if a string starts with a given prefix using startsWith function", "[stringutil][startsWith]") {
	GIVEN("A set of test cases for startsWith function") {
		auto tc = GENERATE(
			StartsWithTestCase {"Hello, World!", "Hello", CaseSensitivity::caseSensitive, true},
			StartsWithTestCase {"Hello, World!", "hello", CaseSensitivity::caseSensitive, false},
			StartsWithTestCase {"Hello, World!", "hello", CaseSensitivity::caseInSensitive, true},
			StartsWithTestCase {"Hello, World!", "World", CaseSensitivity::caseSensitive, false},
			StartsWithTestCase {"Hello, World!", "World", CaseSensitivity::caseInSensitive, false},
			StartsWithTestCase {"Hello, World!", "", CaseSensitivity::caseSensitive, true},
			StartsWithTestCase {"Hello, World!", "", CaseSensitivity::caseInSensitive, true},
			StartsWithTestCase {"Hello, World!", "Hello, World! Extra", CaseSensitivity::caseSensitive, false},
			StartsWithTestCase {"Hello, World!", "Hello, World! Extra", CaseSensitivity::caseInSensitive, false},
			StartsWithTestCase {"", "", CaseSensitivity::caseSensitive, true},
			StartsWithTestCase {"", "", CaseSensitivity::caseInSensitive, true},
			StartsWithTestCase {"", "Hello", CaseSensitivity::caseSensitive, false},
			StartsWithTestCase {"", "Hello", CaseSensitivity::caseInSensitive, false},
			StartsWithTestCase {"Hello, World!", "Hellx", CaseSensitivity::caseSensitive, false},
			StartsWithTestCase {"Hello, World!", "Hellx", CaseSensitivity::caseInSensitive, false},
			StartsWithTestCase {"Hello, World!", "HELLO", CaseSensitivity::caseInSensitive, true},
			StartsWithTestCase {"Hello, World!", "HELLO", CaseSensitivity::caseSensitive, false});

		WHEN("Testing startsWith with various prefixes and case sensitivities") {
			THEN("startsWith should return the expected result") {
				REQUIRE(startsWith(tc.stringToSearch, tc.prefix, tc.caseSensitivity) == tc.expected);
			}
		}
	}
}

SCENARIO("Checking if a string ends with a given suffix using endsWith function", "[stringutil][endsWith]") {
	GIVEN("A set of test cases for endsWith function") {
		auto tc = GENERATE(
			EndsWithTestCase {"Hello, World!", "World!", CaseSensitivity::caseSensitive, true},
			EndsWithTestCase {"Hello, World!", "world!", CaseSensitivity::caseSensitive, false},
			EndsWithTestCase {"Hello, World!", "world!", CaseSensitivity::caseInSensitive, true},
			EndsWithTestCase {"Hello, World!", "Hello", CaseSensitivity::caseSensitive, false},
			EndsWithTestCase {"Hello, World!", "Hello", CaseSensitivity::caseInSensitive, false},
			EndsWithTestCase {"Hello, World!", "", CaseSensitivity::caseSensitive, true},
			EndsWithTestCase {"Hello, World!", "", CaseSensitivity::caseInSensitive, true},
			EndsWithTestCase {"Hello, World!", "Hello, World! Extra", CaseSensitivity::caseSensitive, false},
			EndsWithTestCase {"Hello, World!", "Hello, World! Extra", CaseSensitivity::caseInSensitive, false},
			EndsWithTestCase {"", "", CaseSensitivity::caseSensitive, true},
			EndsWithTestCase {"", "", CaseSensitivity::caseInSensitive, true},
			EndsWithTestCase {"", "World!", CaseSensitivity::caseSensitive, false},
			EndsWithTestCase {"", "World!", CaseSensitivity::caseInSensitive, false},
			EndsWithTestCase {"Hello, World!", "rlx!", CaseSensitivity::caseSensitive, false},
			EndsWithTestCase {"Hello, World!", "rlx!", CaseSensitivity::caseInSensitive, false},
			EndsWithTestCase {"Hello, World!", "WORLD!", CaseSensitivity::caseInSensitive, true},
			EndsWithTestCase {"Hello, World!", "WORLD!", CaseSensitivity::caseSensitive, false},
			EndsWithTestCase {"Hello, World!", "World!", CaseSensitivity::caseInSensitive, true},
			EndsWithTestCase {
				"C++ is a powerful programming language.", "language.", CaseSensitivity::caseSensitive, true},
			EndsWithTestCase {
				"C++ is a powerful programming language.", "Language.", CaseSensitivity::caseSensitive, false},
			EndsWithTestCase {
				"C++ is a powerful programming language.", "language.", CaseSensitivity::caseInSensitive, true},
			EndsWithTestCase {
				"C++ is a powerful programming language.", "LANGUAGE.", CaseSensitivity::caseInSensitive, true},
			EndsWithTestCase {"CaseSensitive", "CaseSensitive", CaseSensitivity::caseSensitive, true},
			EndsWithTestCase {"CaseSensitive", "casesensitive", CaseSensitivity::caseSensitive, false},
			EndsWithTestCase {"CaseSensitive", "casesensitive", CaseSensitivity::caseInSensitive, true});

		WHEN("Testing endsWith with various suffixes and case sensitivities") {
			THEN("endsWith should return the expected result") {
				REQUIRE(endsWith(tc.stringToSearch, tc.suffix, tc.caseSensitivity) == tc.expected);
			}
		}
	}
}

SCENARIO("Combined checks for startsWith and endsWith functions with case sensitivity", "[stringutil][combined]") {
	GIVEN("A set of combined test cases for startsWith and endsWith functions") {
		auto tc = GENERATE(
			CombinedTestCase {"The quick brown fox jumps over the lazy dog.",
							  "The quick",
							  "lazy dog.",
							  CaseSensitivity::caseSensitive,
							  true,
							  true},
			CombinedTestCase {"The quick brown fox jumps over the lazy dog.",
							  "the quick",
							  "LAZY DOG.",
							  CaseSensitivity::caseInSensitive,
							  true,
							  true},
			CombinedTestCase {"The quick brown fox jumps over the lazy dog.",
							  "quick",
							  "lazy dog",
							  CaseSensitivity::caseSensitive,
							  false,
							  false},
			CombinedTestCase {"The quick brown fox jumps over the lazy dog.",
							  "Quick",
							  "Lazy Dog",
							  CaseSensitivity::caseInSensitive,
							  false,
							  false},
			CombinedTestCase {"C++ is a powerful programming language.",
							  "C++",
							  "language.",
							  CaseSensitivity::caseSensitive,
							  true,
							  true},
			CombinedTestCase {"C++ is a powerful programming language.",
							  "c++",
							  "language.",
							  CaseSensitivity::caseInSensitive,
							  true,
							  true},
			CombinedTestCase {"C++ is a powerful programming language.",
							  "c++",
							  "Language.",
							  CaseSensitivity::caseSensitive,
							  false,
							  false},
			CombinedTestCase {"C++ is a powerful programming language.",
							  "C++",
							  "Language.",
							  CaseSensitivity::caseInSensitive,
							  true,
							  true},
			CombinedTestCase {"", "", "", CaseSensitivity::caseSensitive, true, true},
			CombinedTestCase {"", "Hello", "World", CaseSensitivity::caseSensitive, false, false},
			CombinedTestCase {
				"CaseSensitive", "CaseSensitive", "CaseSensitive", CaseSensitivity::caseSensitive, true, true},
			CombinedTestCase {
				"CaseSensitive", "casesensitive", "casesensitive", CaseSensitivity::caseInSensitive, true, true},
			CombinedTestCase {
				"CaseSensitive", "casesensitive", "CaseSensitive", CaseSensitivity::caseInSensitive, true, true},
			CombinedTestCase {"Hello, World!", "Hello", "World!", CaseSensitivity::caseSensitive, true, true},
			CombinedTestCase {"Hello, World!", "hello", "world!", CaseSensitivity::caseInSensitive, true, true},
			CombinedTestCase {"Hello, World!", "hello", "world!", CaseSensitivity::caseSensitive, false, false},
			CombinedTestCase {"Hello, World!", "Hello", "world!", CaseSensitivity::caseInSensitive, true, true},
			CombinedTestCase {"Hello, World!", "Hellx", "rlx!", CaseSensitivity::caseSensitive, false, false},
			CombinedTestCase {"Hello, World!", "Hellx", "rlx!", CaseSensitivity::caseInSensitive, false, false});

		WHEN("Testing both startsWith and endsWith with various prefixes, suffixes, and case sensitivities") {
			THEN("startsWith and endsWith should return the expected results") {
				REQUIRE(startsWith(tc.stringToSearch, tc.prefix, tc.caseSensitivity) == tc.expectedStartsWith);
				REQUIRE(endsWith(tc.stringToSearch, tc.suffix, tc.caseSensitivity) == tc.expectedEndsWith);
			}
		}
	}
}

TEST_CASE("toDecimalPlacesIfNeeded", "[stringUtil]") {
	// 100.0494 -> 100.05, 100.0001 -> 100, 100.000 ->100
	REQUIRE(toDecimalPlacesIfNeeded(100.0494, 2) == "100.05");
	REQUIRE(toDecimalPlacesIfNeeded(100.0001, 2) == "100");
	REQUIRE(toDecimalPlacesIfNeeded(100.000, 2) == "100");
}