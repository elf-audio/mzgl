#include "tests.h"
#include "Listenable.h"

class MyListener {
public:
	std::function<void()> evt;
	void onEvent() { evt(); }
};

class MyClass : public Listenable<MyListener> {
public:
	void fireEvent() {
		for (auto l: listeners) {
			l->onEvent();
		}
	}
};
SCENARIO("listeners can be added and removed correctly", "[listener]") {
	GIVEN("a class that is listenable") {
		MyClass c;
		MyListener l;
		REQUIRE(c.getNumListeners() == 0);
		WHEN("a listener is added") {
			c.addListener(&l);
			THEN("the listener is in the list") {
				REQUIRE(c.getNumListeners() == 1);
			}
			AND_WHEN("the listener is removed") {
				c.removeListener(&l);
				THEN("the listener is not in the list") {
					REQUIRE(c.getNumListeners() == 0);
				}
			}
			AND_WHEN("the listener is added multiple times") {
				c.addListener(&l);
				c.addListener(&l);
				THEN("the listener is only in the list once") {
					REQUIRE(c.getNumListeners() == 1);
				}
			}
			AND_WHEN("the listener is removed twice") {
				c.removeListener(&l);
				c.removeListener(&l);
				THEN("the listener is not in the list") {
					REQUIRE(c.getNumListeners() == 0);
				}
			}
			AND_WHEN("an event is fired") {
				int eventFireCount = 0;
				l.evt			   = [&eventFireCount] { eventFireCount++; };
				c.fireEvent();
				THEN("the listener is notified") {
					REQUIRE(eventFireCount == 1);
				}
				c.fireEvent();
				THEN("listener is notified twice") {
					REQUIRE(eventFireCount == 2);
				}
			}
		}

		WHEN("a scoped listener is added") {
			{
				ScopedListener<MyListener> sl(c, &l);
				THEN("the listener is in the list") {
					REQUIRE(c.getNumListeners() == 1);
				}
			}

			THEN("the listener is not in the list") {
				REQUIRE(c.getNumListeners() == 0);
			}
		}
		WHEN("2 scoped listeners are added") {
			MyListener l1, l2;
			ScopedListener<MyListener> sl1(c, &l1);
			ScopedListener<MyListener> sl2(c, &l2);
			THEN("there are 2 listeners total") {
				REQUIRE(c.getNumListeners() == 2);
			}

			AND_WHEN("an event is fired") {
				int fireCount = 0;
				l1.evt		  = [&fireCount] { fireCount++; };
				l2.evt		  = l1.evt;
				c.fireEvent();
				THEN("both listeners are notified") {
					REQUIRE(fireCount == 2);
				}
			}
		}
	}
}