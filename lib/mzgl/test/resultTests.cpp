#include "tests.h"

#include "Result.h"

SCENARIO("A default-constructed Result is empty and successful", "[result]") {
	GIVEN("a default Result") {
		Result r;
		THEN("it has no issues") {
			REQUIRE(r.issues.empty());
		}
		THEN("success() is true") {
			REQUIRE(r.success());
		}
		THEN("getIssueList() is an empty string") {
			REQUIRE(r.getIssueList() == "");
		}
	}
}

SCENARIO("Constructing a Result with a single issue marks it as failed", "[result]") {
	GIVEN("a Result built from one issue string") {
		Result r {"file not found"};
		THEN("success() is false") {
			REQUIRE_FALSE(r.success());
		}
		THEN("issues contains exactly that string") {
			REQUIRE(r.issues.size() == 1);
			REQUIRE(r.issues[0] == "file not found");
		}
		THEN("getIssueList() prefixes a single dash-space bullet") {
			REQUIRE(r.getIssueList() == "- file not found");
		}
	}
}

SCENARIO("addIssue appends to the issue list in order", "[result]") {
	GIVEN("an empty Result") {
		Result r;
		WHEN("three issues are added in order") {
			r.addIssue("a");
			r.addIssue("b");
			r.addIssue("c");
			THEN("success() is false") {
				REQUIRE_FALSE(r.success());
			}
			THEN("issues preserves insertion order") {
				REQUIRE(r.issues.size() == 3);
				REQUIRE(r.issues[0] == "a");
				REQUIRE(r.issues[1] == "b");
				REQUIRE(r.issues[2] == "c");
			}
			THEN("getIssueList() joins them with newline-dash-space and a leading dash-space") {
				REQUIRE(r.getIssueList() == "- a\n- b\n- c");
			}
		}
	}
}

SCENARIO("operator+= merges another Result's issues onto the end of this one", "[result]") {
	GIVEN("two Results with disjoint issues") {
		Result a;
		a.addIssue("a1");
		a.addIssue("a2");
		Result b;
		b.addIssue("b1");

		WHEN("a += b") {
			a += b;
			THEN("a now contains a's issues followed by b's, in order") {
				REQUIRE(a.issues.size() == 3);
				REQUIRE(a.issues[0] == "a1");
				REQUIRE(a.issues[1] == "a2");
				REQUIRE(a.issues[2] == "b1");
			}
			THEN("b is unchanged") {
				REQUIRE(b.issues.size() == 1);
				REQUIRE(b.issues[0] == "b1");
			}
		}
	}

	GIVEN("a successful Result and an empty other Result") {
		Result a;
		Result b;
		WHEN("a += b") {
			a += b;
			THEN("a is still successful") {
				REQUIRE(a.success());
				REQUIRE(a.issues.empty());
			}
		}
	}

	GIVEN("a failing Result and an empty other Result") {
		Result a {"boom"};
		Result b;
		WHEN("a += b") {
			a += b;
			THEN("a is unchanged") {
				REQUIRE_FALSE(a.success());
				REQUIRE(a.issues.size() == 1);
				REQUIRE(a.issues[0] == "boom");
			}
		}
	}

	GIVEN("an empty Result and a failing other Result") {
		Result a;
		Result b {"boom"};
		WHEN("a += b") {
			a += b;
			THEN("a takes on b's issues") {
				REQUIRE_FALSE(a.success());
				REQUIRE(a.issues.size() == 1);
				REQUIRE(a.issues[0] == "boom");
			}
		}
	}
}

SCENARIO("operator+= returns a reference suitable for chaining", "[result]") {
	GIVEN("three Results") {
		Result a;
		Result b {"b"};
		Result c {"c"};
		WHEN("merged with a chained +=") {
			(a += b) += c;
			THEN("a contains both b's and c's issues in order") {
				REQUIRE(a.issues.size() == 2);
				REQUIRE(a.issues[0] == "b");
				REQUIRE(a.issues[1] == "c");
			}
		}
	}
}
