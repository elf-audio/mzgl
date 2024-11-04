#include "tests.h"
#include "FixedSizeCircularBuffer.h"

TEST_CASE("FixedSizeCircularBuffer functionality", "[fixedsizecircularbuffer]") {
	GIVEN("An empty FixedSizeCircularBuffer of size 5") {
		FixedSizeCircularBuffer<int, 5> buffer;

		THEN("The buffer should be empty") {
			REQUIRE(buffer.empty());
			REQUIRE(buffer.size() == 0);
		}

		WHEN("An element is pushed into the buffer") {
			buffer.push(42);

			THEN("The buffer should not be empty") {
				REQUIRE_FALSE(buffer.empty());
				REQUIRE(buffer.size() == 1);
			}

			THEN("The element should be retrievable at index 0") {
				REQUIRE(buffer[0] == 42);
			}

			THEN("Accessing out-of-range index should throw an exception") {
				REQUIRE_THROWS_AS(buffer[1], std::out_of_range);
			}
		}

		WHEN("Multiple elements are pushed into the buffer") {
			buffer.push(1);
			buffer.push(2);
			buffer.push(3);
			buffer.push(4);
			buffer.push(5);

			THEN("The buffer should be full") {
				REQUIRE(buffer.size() == 5);
			}

			THEN("Elements should be retrievable in the correct order") {
				REQUIRE(buffer[0] == 1);
				REQUIRE(buffer[1] == 2);
				REQUIRE(buffer[2] == 3);
				REQUIRE(buffer[3] == 4);
				REQUIRE(buffer[4] == 5);
			}

			WHEN("Additional elements are pushed beyond the buffer's capacity") {
				buffer.push(6);
				buffer.push(7);

				THEN("The oldest elements should be overwritten") {
					REQUIRE(buffer.size() == 5);
					REQUIRE(buffer[0] == 3);
					REQUIRE(buffer[1] == 4);
					REQUIRE(buffer[2] == 5);
					REQUIRE(buffer[3] == 6);
					REQUIRE(buffer[4] == 7);
				}
			}
		}

		WHEN("Clearing the buffer after adding elements") {
			buffer.push(10);
			buffer.push(20);
			buffer.clear();

			THEN("The buffer should be empty") {
				REQUIRE(buffer.empty());
				REQUIRE(buffer.size() == 0);
			}

			THEN("Accessing any index should throw an exception") {
				REQUIRE_THROWS_AS(buffer[0], std::out_of_range);
			}
		}

		WHEN("Accessing elements in an empty buffer") {
			THEN("An exception should be thrown") {
				REQUIRE_THROWS_AS(buffer[0], std::out_of_range);
			}
		}
	}

	GIVEN("A FixedSizeCircularBuffer of size 3 with initial elements") {
		FixedSizeCircularBuffer<std::string, 3> buffer;
		buffer.push("alpha");
		buffer.push("beta");
		buffer.push("gamma");

		THEN("The buffer should contain the correct elements") {
			REQUIRE(buffer.size() == 3);
			REQUIRE(buffer[0] == "alpha");
			REQUIRE(buffer[1] == "beta");
			REQUIRE(buffer[2] == "gamma");
		}

		WHEN("An additional element is pushed into the full buffer") {
			buffer.push("delta");

			THEN("The oldest element should be overwritten") {
				REQUIRE(buffer.size() == 3);
				REQUIRE(buffer[0] == "beta");
				REQUIRE(buffer[1] == "gamma");
				REQUIRE(buffer[2] == "delta");
			}
		}

		WHEN("Clearing the buffer") {
			buffer.clear();

			THEN("The buffer should be empty") {
				REQUIRE(buffer.empty());
				REQUIRE(buffer.size() == 0);
			}

			THEN("Accessing elements should throw an exception") {
				REQUIRE_THROWS_AS(buffer[0], std::out_of_range);
			}
		}
	}

	GIVEN("A FixedSizeCircularBuffer of size 2") {
		FixedSizeCircularBuffer<int, 2> buffer;

		WHEN("Pushing one element") {
			buffer.push(100);

			THEN("Size should be 1") {
				REQUIRE(buffer.size() == 1);
			}

			THEN("empty() should return false") {
				REQUIRE_FALSE(buffer.empty());
			}

			THEN("Element at index 0 should be correct") {
				REQUIRE(buffer[0] == 100);
			}
		}

		WHEN("Pushing two elements") {
			buffer.push(200);
			buffer.push(300);

			THEN("Size should be 2") {
				REQUIRE(buffer.size() == 2);
			}

			THEN("Elements should be correct") {
				REQUIRE(buffer[0] == 200);
				REQUIRE(buffer[1] == 300);
			}
		}

		WHEN("Pushing three elements (overwriting occurs)") {
			buffer.push(400);
			buffer.push(500);
			buffer.push(600);

			THEN("Size should remain 2") {
				REQUIRE(buffer.size() == 2);
			}

			THEN("Oldest element should be overwritten") {
				REQUIRE(buffer[0] == 500);
				REQUIRE(buffer[1] == 600);
			}
		}
	}
}

TEST_CASE("FixedSizeCircularBuffer iterator functionality", "[FixedSizeCircularBuffer][Iterator]") {
	GIVEN("An empty FixedSizeCircularBuffer<int, 5>") {
		FixedSizeCircularBuffer<int, 5> buffer;

		WHEN("begin() and end() are obtained") {
			auto it_begin = buffer.begin();
			auto it_end	  = buffer.end();

			THEN("begin() equals end()") {
				REQUIRE(it_begin == it_end);
			}

			THEN("Iterating over the buffer yields no elements") {
				REQUIRE(it_begin == it_end);
			}
		}

		WHEN("Elements are added to the buffer") {
			buffer.push(10);
			buffer.push(20);
			buffer.push(30);

			THEN("begin() and end() define a range of size equal to buffer.size()") {
				auto it_begin = buffer.begin();
				auto it_end	  = buffer.end();
				REQUIRE(std::distance(it_begin, it_end) == buffer.size());
			}

			THEN("Iterator can be used to traverse the buffer") {
				std::vector<int> elements;
				for (auto it = buffer.begin(); it != buffer.end(); ++it) {
					elements.push_back(*it);
				}
				REQUIRE(elements.size() == 3);
				REQUIRE(elements[0] == 10);
				REQUIRE(elements[1] == 20);
				REQUIRE(elements[2] == 30);
			}

			THEN("Iterator supports std::for_each") {
				int sum = 0;
				std::for_each(buffer.begin(), buffer.end(), [&sum](int value) { sum += value; });
				REQUIRE(sum == 60);
			}

			THEN("Iterator supports std::accumulate") {
				int total = std::accumulate(buffer.begin(), buffer.end(), 0);
				REQUIRE(total == 60);
			}
		}
	}

	GIVEN("A full FixedSizeCircularBuffer<int, 5>") {
		FixedSizeCircularBuffer<int, 5> buffer;
		buffer.push(1);
		buffer.push(2);
		buffer.push(3);
		buffer.push(4);
		buffer.push(5);

		WHEN("An additional element is pushed (causing overwrite)") {
			buffer.push(6);

			THEN("The buffer contains the last 5 elements") {
				std::vector<int> elements(buffer.begin(), buffer.end());
				REQUIRE(elements.size() == 5);
				REQUIRE(elements[0] == 2);
				REQUIRE(elements[1] == 3);
				REQUIRE(elements[2] == 4);
				REQUIRE(elements[3] == 5);
				REQUIRE(elements[4] == 6);
			}

			THEN("Iterator arithmetic works correctly") {
				auto it = buffer.begin();
				REQUIRE(*it == 2);
				it += 2;
				REQUIRE(*it == 4);
				it = it + 2;
				REQUIRE(*it == 6);
				it = it - 3;
				REQUIRE(*it == 3);
			}

			THEN("Iterator comparisons work correctly") {
				auto it1 = buffer.begin();
				auto it2 = buffer.begin() + 3;
				REQUIRE(it1 != it2);
				REQUIRE(it1 < it2);
				REQUIRE(it2 > it1);
				REQUIRE(it1 <= it2);
				REQUIRE(it2 >= it1);
			}

			THEN("Difference between iterators is calculated correctly") {
				auto it_begin = buffer.begin();
				auto it_end	  = buffer.end();
				REQUIRE(it_end - it_begin == 5);
				REQUIRE(it_begin - it_end == -5);
			}
		}
	}

	GIVEN("A FixedSizeCircularBuffer<std::string, 3> with elements") {
		FixedSizeCircularBuffer<std::string, 3> buffer;
		buffer.push("alpha");
		buffer.push("beta");
		buffer.push("gamma");

		WHEN("Using const_iterator to traverse the buffer") {
			const auto &const_buffer = buffer;
			std::vector<std::string> elements(const_buffer.cbegin(), const_buffer.cend());

			THEN("All elements are correctly accessed") {
				REQUIRE(elements.size() == 3);
				REQUIRE(elements[0] == "alpha");
				REQUIRE(elements[1] == "beta");
				REQUIRE(elements[2] == "gamma");
			}
		}

		WHEN("Using reverse iteration") {
			std::vector<std::string> elements;
			for (auto it = buffer.end(); it != buffer.begin();) {
				--it;
				elements.push_back(*it);
			}

			THEN("Elements are accessed in reverse order") {
				REQUIRE(elements.size() == 3);
				REQUIRE(elements[0] == "gamma");
				REQUIRE(elements[1] == "beta");
				REQUIRE(elements[2] == "alpha");
			}
		}
	}

	GIVEN("An iterator pointing to the buffer's begin") {
		FixedSizeCircularBuffer<int, 5> buffer;
		buffer.push(100);
		buffer.push(200);

		auto it = buffer.begin();

		WHEN("Iterator is incremented") {
			++it;

			THEN("It points to the next element") {
				REQUIRE(*it == 200);
			}
		}

		WHEN("Iterator is advanced beyond end") {
			it += buffer.size();

			THEN("It equals end()") {
				REQUIRE(it == buffer.end());
			}

			THEN("Dereferencing end iterator is invalid") {
				REQUIRE_THROWS_AS(*it, std::out_of_range);
			}
		}
	}

	GIVEN("Two iterators pointing within the buffer") {
		FixedSizeCircularBuffer<int, 5> buffer;
		buffer.push(1);
		buffer.push(2);
		buffer.push(3);

		auto it1 = buffer.begin();
		auto it2 = buffer.begin() + 2;

		WHEN("Calculating the distance between iterators") {
			auto distance = it2 - it1;

			THEN("The distance is correct") {
				REQUIRE(distance == 2);
			}
		}

		WHEN("Comparing iterators for equality and ordering") {
			THEN("Iterators are compared correctly") {
				REQUIRE(it1 != it2);
				REQUIRE(it1 < it2);
				REQUIRE(it2 > it1);
				REQUIRE(it1 <= it2);
				REQUIRE(it2 >= it1);
			}
		}
	}

	GIVEN("Using STL algorithms with FixedSizeCircularBuffer") {
		FixedSizeCircularBuffer<int, 10> buffer;
		for (int i = 1; i <= 15; ++i) {
			buffer.push(i);
		}

		WHEN("Using std::find to search for an element") {
			auto it = std::find(buffer.begin(), buffer.end(), 10);

			THEN("Element is found") {
				REQUIRE(it != buffer.end());
				REQUIRE(*it == 10);
			}
		}

		WHEN("Using std::count_if to count even numbers") {
			int count = std::count_if(buffer.begin(), buffer.end(), [](int value) { return value % 2 == 0; });

			THEN("Count is correct") {
				REQUIRE(count == 5);
			}
		}

		WHEN("Using std::copy to copy elements to a vector") {
			std::vector<int> elements;
			std::copy(buffer.begin(), buffer.end(), std::back_inserter(elements));

			THEN("All elements are copied") {
				REQUIRE(elements.size() == 10);
				REQUIRE(elements[0] == 6);
				REQUIRE(elements[9] == 15);
			}
		}
	}

	GIVEN("Modifying the buffer invalidates iterators") {
		FixedSizeCircularBuffer<int, 5> buffer;
		buffer.push(1);
		buffer.push(2);
		buffer.push(3);

		auto it = buffer.begin();

		WHEN("An element is pushed to the buffer") {
			buffer.push(4);

			THEN("Iterator may be invalidated") {
				REQUIRE(*it == 1);
			}

			WHEN("Enough elements are pushed to overwrite the oldest element") {
				buffer.push(5);
				buffer.push(6);

				THEN("Iterator may now point to overwritten data") {
				}
			}
		}
	}

	GIVEN("A const_iterator and iterator pointing to the same position") {
		FixedSizeCircularBuffer<int, 5> buffer;
		buffer.push(10);
		buffer.push(20);
		buffer.push(30);

		auto it	 = buffer.begin();
		auto cit = buffer.cbegin();

		WHEN("Comparing iterator and const_iterator") {
			THEN("Their dereferenced values are equal") {
				REQUIRE(*it == *cit);
			}

			THEN("Incrementing both moves them to the next element") {
				++it;
				++cit;
				REQUIRE(*it == *cit);
			}
		}
	}

	GIVEN("Testing operator-> with iterator") {
		FixedSizeCircularBuffer<std::string, 3> buffer;
		buffer.push("hello");
		buffer.push("world");

		auto it = buffer.begin();

		WHEN("Using operator-> to access string methods") {
			THEN("Can access methods like length()") {
				REQUIRE(it->length() == 5);
			}

			++it;

			THEN("Operator-> works on the next element") {
				REQUIRE(it->substr(1) == "orld");
			}
		}
	}

	GIVEN("Testing iterator arithmetic beyond valid range") {
		FixedSizeCircularBuffer<int, 5> buffer;
		buffer.push(1);
		buffer.push(2);
		buffer.push(3);

		WHEN("Iterator is incremented beyond end") {
			auto it = buffer.end();
			++it;

			THEN("Dereferencing the iterator throws an exception") {
				REQUIRE_THROWS_AS(*it, std::out_of_range);
			}
		}

		WHEN("Iterator is decremented before begin") {
			auto it = buffer.begin();
			--it;

			THEN("Dereferencing the iterator throws an exception") {
				REQUIRE_THROWS_AS(*it, std::out_of_range);
			}
		}
	}

	GIVEN("Testing iterator subtraction") {
		FixedSizeCircularBuffer<int, 5> buffer;
		buffer.push(10);
		buffer.push(20);
		buffer.push(30);
		buffer.push(40);

		WHEN("Subtracting two iterators") {
			auto it1  = buffer.begin() + 1;
			auto it2  = buffer.begin() + 3;
			auto diff = it2 - it1;

			THEN("Difference is correct") {
				REQUIRE(diff == 2);
			}
		}
	}
}