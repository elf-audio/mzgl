#include "Base64.h"
#include "tests.h"
#include <stdexcept>
#include <string>
#include <vector>

SCENARIO("Decoding Base64 strings to binary data", "[base64]") {
	GIVEN("A valid Base64 encoded string representing 'Hello world'") {
		std::string base64Data = "SGVsbG8gd29ybGQ=";

		WHEN("The string is decoded") {
			std::vector<char> decoded = Base64::decode(base64Data);

			THEN("The output should match the original binary data") {
				std::string expected = "Hello world";
				REQUIRE(decoded == std::vector<char>(expected.begin(), expected.end()));
			}
		}
	}

	GIVEN("A valid Base64 encoded string with no padding") {
		std::string base64Data = "U29mdHdhcmUgRW5naW5lZXJpbmc=";

		WHEN("The string is decoded") {
			std::vector<char> decoded = Base64::decode(base64Data);

			THEN("The output should match the original binary data") {
				std::string expected = "Software Engineering";
				REQUIRE(decoded == std::vector<char>(expected.begin(), expected.end()));
			}
		}
	}

	GIVEN("A valid Base64 encoded string with one padding character") {
		std::string base64Data = "U29m";

		WHEN("The string is decoded") {
			std::vector<char> decoded = Base64::decode(base64Data);

			THEN("The output should match the original binary data") {
				std::string expected = "Sof";
				REQUIRE(decoded == std::vector<char>(expected.begin(), expected.end()));
			}
		}
	}

	GIVEN("A valid Base64 encoded string with two padding characters") {
		std::string base64Data = "U3Q=";

		WHEN("The string is decoded") {
			std::vector<char> decoded = Base64::decode(base64Data);

			THEN("The output should match the original binary data") {
				std::string expected = "St";
				REQUIRE(decoded == std::vector<char>(expected.begin(), expected.end()));
			}
		}
	}

	GIVEN("An empty Base64 string") {
		std::string base64Data = "";

		WHEN("The string is decoded") {
			std::vector<char> decoded = Base64::decode(base64Data);

			THEN("The output should be an empty vector") {
				REQUIRE(decoded.empty());
			}
		}
	}

	GIVEN("An invalid Base64 string with non-Base64 characters") {
		std::string base64Data = "Invalid@@Base64";

		WHEN("The string is decoded") {
			THEN("An exception should be thrown") {
				REQUIRE_THROWS_AS(Base64::decode(base64Data), std::invalid_argument);
			}
		}
	}

	GIVEN("An invalid Base64 string with incorrect padding") {
		std::string base64Data = "SGVsbG===";

		WHEN("The string is decoded") {
			THEN("An exception should be thrown") {
				REQUIRE_THROWS_AS(Base64::decode(base64Data), std::invalid_argument);
			}
		}
	}

	GIVEN("A Base64 string representing a binary file") {
		std::string base64Data = "/9j/4AAQSkZJRgABAQEAYABgAAD/2wCEAAEBAQEBAQEBAQEBAQEBAQ==";

		WHEN("The string is decoded") {
			std::vector<char> decoded = Base64::decode(base64Data);

			THEN("The output should match the original binary content") {
				REQUIRE(decoded.size() > 0);
				REQUIRE(decoded[0] == static_cast<char>(0xFF));
			}
		}
	}
}

TEST_CASE("Base64::encode function") {
	GIVEN("An empty input vector") {
		std::vector<unsigned char> input = {};

		WHEN("The Base64::encode function is called") {
			std::string result = Base64::encode(input);

			THEN("The result should be an empty string") {
				REQUIRE(result == "");
			}
		}
	}

	GIVEN("A single byte input vector") {
		std::vector<unsigned char> input = {'A'};

		WHEN("The Base64::encode function is called") {
			std::string result = Base64::encode(input);

			THEN("The result should be the base64 representation with padding") {
				REQUIRE(result == "QQ==");
			}
		}
	}

	GIVEN("A three-byte input vector forming a full group") {
		std::vector<unsigned char> input = {'M', 'a', 'n'};

		WHEN("The Base64::encode function is called") {
			std::string result = Base64::encode(input);

			THEN("The result should be the base64 representation without padding") {
				REQUIRE(result == "TWFu");
			}
		}
	}

	GIVEN("An input vector with two bytes (requires padding)") {
		std::vector<unsigned char> input = {'A', 'B'};

		WHEN("The base64Encode function is called") {
			std::string result = Base64::encode(input);

			THEN("The result should include padding (one '=')") {
				REQUIRE(result == "QUI=");
			}
		}
	}

	GIVEN("An input vector with one byte (requires padding)") {
		std::vector<unsigned char> input = {'X'}; // ASCII for "X"

		WHEN("The Base64::encode function is called") {
			std::string result = Base64::encode(input);

			THEN("The result should include padding (two '=')") {
				REQUIRE(result == "WA==");
			}
		}
	}
}