#include "tests.h"

#include "filesystem.h"
#include "util.h"

#include <vector>

TEST_CASE("trashOrDeleteTest", "[mzgl][util]") {
	auto path = fs::path {tempDir()} / "temp.txt";
	writeStringToFile(path.string(), "test");

	REQUIRE(fs::exists(path));
	deleteOrTrash(path.string());
	REQUIRE_FALSE(fs::exists(path));
	REQUIRE_NOTHROW(deleteOrTrash(path.string()));
}

TEST_CASE("concatenate and append functions", "[concatenate][append]") {
	SECTION("Using std::vector<int>") {
		GIVEN("Two non-empty vectors") {
			std::vector<int> vec1 = {1, 2, 3};
			std::vector<int> vec2 = {4, 5, 6};

			WHEN("Using concatenate to create a new vector") {
				auto result = concatenate(vec1, vec2);

				THEN("The new vector contains elements from both vectors in the correct order") {
					REQUIRE(result == std::vector<int> {1, 2, 3, 4, 5, 6});
				}

				THEN("The original vectors remain unchanged") {
					REQUIRE(vec1 == std::vector<int> {1, 2, 3});
					REQUIRE(vec2 == std::vector<int> {4, 5, 6});
				}
			}

			WHEN("Using append to modify the source vector") {
				append(vec1, vec2);

				THEN("The source vector contains elements from both vectors in the correct order") {
					REQUIRE(vec1 == std::vector<int> {1, 2, 3, 4, 5, 6});
				}

				THEN("The append vector remains unchanged") {
					REQUIRE(vec2 == std::vector<int> {4, 5, 6});
				}
			}
		}

		GIVEN("An empty source vector and a non-empty append vector") {
			std::vector<int> vec1 = {};
			std::vector<int> vec2 = {4, 5, 6};

			WHEN("Using concatenate to create a new vector") {
				auto result = concatenate(vec1, vec2);

				THEN("The new vector contains only elements from the append vector") {
					REQUIRE(result == vec2);
				}
			}

			WHEN("Using append to modify the source vector") {
				append(vec1, vec2);

				THEN("The source vector contains only elements from the append vector") {
					REQUIRE(vec1 == vec2);
				}
			}
		}

		GIVEN("A non-empty source vector and an empty append vector") {
			std::vector<int> vec1 = {1, 2, 3};
			std::vector<int> vec2 = {};

			WHEN("Using concatenate to create a new vector") {
				auto result = concatenate(vec1, vec2);

				THEN("The new vector contains only elements from the source vector") {
					REQUIRE(result == vec1);
				}
			}

			WHEN("Using append to modify the source vector") {
				append(vec1, vec2);

				THEN("The source vector remains unchanged") {
					REQUIRE(vec1 == std::vector<int> {1, 2, 3});
				}
			}
		}

		GIVEN("Two empty vectors") {
			std::vector<int> vec1 = {};
			std::vector<int> vec2 = {};

			WHEN("Using concatenate to create a new vector") {
				auto result = concatenate(vec1, vec2);

				THEN("The new vector is empty") {
					REQUIRE(result.empty());
				}
			}

			WHEN("Using append to modify the source vector") {
				append(vec1, vec2);

				THEN("The source vector remains empty") {
					REQUIRE(vec1.empty());
				}
			}
		}
	}

	SECTION("Using std::vector<std::string>") {
		GIVEN("Two non-empty vectors") {
			std::vector<std::string> vec1 = {"Hello", "World"};
			std::vector<std::string> vec2 = {"Foo", "Bar"};

			WHEN("Using concatenate to create a new vector") {
				auto result = concatenate(vec1, vec2);

				THEN("The new vector contains elements from both vectors in the correct order") {
					REQUIRE(result == std::vector<std::string> {"Hello", "World", "Foo", "Bar"});
				}

				THEN("The original vectors remain unchanged") {
					REQUIRE(vec1 == std::vector<std::string> {"Hello", "World"});
					REQUIRE(vec2 == std::vector<std::string> {"Foo", "Bar"});
				}
			}

			WHEN("Using append to modify the source vector") {
				append(vec1, vec2);

				THEN("The source vector contains elements from both vectors in the correct order") {
					REQUIRE(vec1 == std::vector<std::string> {"Hello", "World", "Foo", "Bar"});
				}

				THEN("The append vector remains unchanged") {
					REQUIRE(vec2 == std::vector<std::string> {"Foo", "Bar"});
				}
			}
		}

		GIVEN("An empty source vector and a non-empty append vector") {
			std::vector<std::string> vec1 = {};
			std::vector<std::string> vec2 = {"Foo", "Bar"};

			WHEN("Using concatenate to create a new vector") {
				auto result = concatenate(vec1, vec2);

				THEN("The new vector contains only elements from the append vector") {
					REQUIRE(result == vec2);
				}
			}

			WHEN("Using append to modify the source vector") {
				append(vec1, vec2);

				THEN("The source vector contains only elements from the append vector") {
					REQUIRE(vec1 == vec2);
				}
			}
		}

		GIVEN("A non-empty source vector and an empty append vector") {
			std::vector<std::string> vec1 = {"Hello", "World"};
			std::vector<std::string> vec2 = {};

			WHEN("Using concatenate to create a new vector") {
				auto result = concatenate(vec1, vec2);

				THEN("The new vector contains only elements from the source vector") {
					REQUIRE(result == vec1);
				}
			}

			WHEN("Using append to modify the source vector") {
				append(vec1, vec2);

				THEN("The source vector remains unchanged") {
					REQUIRE(vec1 == std::vector<std::string> {"Hello", "World"});
				}
			}
		}

		GIVEN("Two empty vectors") {
			std::vector<std::string> vec1 = {};
			std::vector<std::string> vec2 = {};

			WHEN("Using concatenate to create a new vector") {
				auto result = concatenate(vec1, vec2);

				THEN("The new vector is empty") {
					REQUIRE(result.empty());
				}
			}

			WHEN("Using append to modify the source vector") {
				append(vec1, vec2);

				THEN("The source vector remains empty") {
					REQUIRE(vec1.empty());
				}
			}
		}
	}
}