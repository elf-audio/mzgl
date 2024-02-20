//
// Created by Marek Bereza on 12/02/2024.
//
#include "tests.h"
#include "Variable.h"

template <class T>
class VariableTest {
public:
	Variable<T> watchedVariable {false};
	bool requiredHeadphonesValue = false;
	int callbackCount			 = 0;

	void runTest() {
		REQUIRE(watchedVariable.getNumListeners() == 0);
		WHEN("a watcher is added") {
			VariableWatcher<T> t {watchedVariable, [this](bool newValue) {
									  //headphoneToggle.setValue(newValue);
									  REQUIRE(newValue == requiredHeadphonesValue);
									  callbackCount++;
								  }};
			THEN("the number of listeners should increase") {
				REQUIRE(watchedVariable.getNumListeners() == 1);
			}
			AND_WHEN("another watcher is added") {
				VariableWatcher<T> t2 {watchedVariable, [this](bool newValue) {
										   //headphoneToggle.setValue(newValue);
										   REQUIRE(newValue == requiredHeadphonesValue);
										   callbackCount++;
									   }};
				THEN("the number of listeners should increase") {
					REQUIRE(watchedVariable.getNumListeners() == 2);
				}
			}
			AND_WHEN("the second watcher falls out of scope") {
				THEN("the number of listeners should decrease") {
					REQUIRE(watchedVariable.getNumListeners() == 1);
				}
			}

			AND_WHEN("the variable is changed") {
				requiredHeadphonesValue = true;
				watchedVariable			= true;
				THEN("the callback should be called") {
					REQUIRE(callbackCount == 1);
					AND_THEN("the value should be changed") {
						REQUIRE(watchedVariable == true);
					}
				}
				AND_WHEN("the variable is changed again") {
					requiredHeadphonesValue = false;
					watchedVariable			= false;
					THEN("the callback should be called again") {
						REQUIRE(callbackCount == 2);
						AND_THEN("the value should be changed") {
							REQUIRE(watchedVariable == false);
						}
					}
				}
			}
		}
	}
};

SCENARIO("variables can be watched by multiple watchers", "[Variable]") {
	GIVEN("a primitive variable") {
		VariableTest<bool> t2;
		t2.runTest();
	}
	GIVEN("an atomic variable") {
		VariableTest<std::atomic<bool>> t;
		t.runTest();
	}
}

SCENARIO("when you copy assign a watcher it should be removed from the original variable", "[Variable]") {
	Variable<bool> a {false};
	Variable<bool> b {false};
	bool aCalled = false;
	bool bCalled = false;
	VariableWatcher<bool> w1 {a, [&aCalled](bool) { aCalled = true; }};
	VariableWatcher<bool> w2 {b, [&bCalled](bool) { bCalled = true; }};

	REQUIRE(aCalled == false);
	REQUIRE(bCalled == false);
	REQUIRE(a.getNumListeners() == 1);
	REQUIRE(b.getNumListeners() == 1);

	WHEN("the first watcher is copied to the second variable and first variable is changed") {
		w1 = w2;
		REQUIRE(aCalled == false);
		REQUIRE(bCalled == false);
		REQUIRE(a.getNumListeners() == 0);
		REQUIRE(b.getNumListeners() == 2);
		//
		//		a  = true;
		//
		//		THEN("no watchers are notified") {
		//			REQUIRE(aCalled == false);
		//			REQUIRE(bCalled == false);
		//		}
		//
		//		AND_WHEN("the second variable is changed") {
		//			b = true;
		//			THEN("both watchers are notified") {
		//				REQUIRE(aCalled == true);
		//				REQUIRE(bCalled == true);
		//			}
		//		}
	}
}