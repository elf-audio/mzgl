#include "tests.h"
#include <mzgl/util/Variable.h>
#include <array>

class VariableListener : public Variable<float>::Listener {
public:
	void variableChanged() override { gotNotification = true; }
	bool gotNotification {false};
};

SCENARIO("Variables can have a listener", "[Variable]") {
	GIVEN("A primitive variable and a listener") {
		Variable<float> var {0.5f};
		VariableListener listener;
		REQUIRE(var.getNumListeners() == 0);
		WHEN("We add a listener") {
			var.addListener(&listener);
			THEN("This should be reflected in the count") {
				REQUIRE(var.getNumListeners() == 1);
				REQUIRE(var.isListener(&listener));
			}
		}
	}
}

SCENARIO("Variable listeners are notified", "[Variable]") {
	GIVEN("A primitive variable and a listener") {
		Variable<float> var {0.5f};
		VariableListener listener;
		var.addListener(&listener);
		WHEN("The variable is changed") {
			var = 1.f;
			THEN("The listener should be notified") {
				REQUIRE(listener.gotNotification);
			}
		}
	}
}

SCENARIO("Variables can have multiple listeners", "[Variable]") {
	GIVEN("A primitive variable and a multiple listener") {
		Variable<float> var {0.5f};
		std::array<VariableListener, 5> listeners;
		REQUIRE(var.getNumListeners() == 0);
		WHEN("We add the listeners") {
			for (auto &listener: listeners) {
				var.addListener(&listener);
			}
			THEN("This should be reflected in the count") {
				REQUIRE(var.getNumListeners() == listeners.size());
				for (auto &listener: listeners) {
					REQUIRE(var.isListener(&listener));
				}
			}
		}
	}
}

SCENARIO("Multiple listeners are notified", "[Variable]") {
	GIVEN("A primitive variable and a listener") {
		Variable<float> var {0.5f};
		std::array<VariableListener, 5> listeners;
		for (auto &listener: listeners) {
			var.addListener(&listener);
		}
		WHEN("The variable is changed") {
			var = 1.f;
			THEN("The listener should be notified") {
				for (auto &listener: listeners) {
					REQUIRE(listener.gotNotification);
				}
			}
		}
	}
}

SCENARIO("Listeners can be removed from variables", "[Variable]") {
	GIVEN("A primitive variable and a listener") {
		Variable<float> var {0.5f};
		VariableListener listener;
		REQUIRE(var.getNumListeners() == 0);
		WHEN("We add a listener") {
			var.addListener(&listener);
			THEN("This should be reflected in the count") {
				REQUIRE(var.getNumListeners() == 1);
				REQUIRE(var.isListener(&listener));
				WHEN("We remove the listener") {
					var.removeListener(&listener);
					THEN("This should be reflected in the count") {
						REQUIRE(var.getNumListeners() == 0);
						REQUIRE_FALSE(var.isListener(&listener));
					}
				}
			}
		}
	}
}

SCENARIO("Multiple Listeners can be removed from variables", "[Variable]") {
	GIVEN("A primitive variable and a listener") {
		Variable<float> var {0.5f};
		REQUIRE(var.getNumListeners() == 0);
		std::array<VariableListener, 5> listeners;

		WHEN("We add a listener") {
			for (auto &listener: listeners) {
				var.addListener(&listener);
			}
			THEN("This should be reflected in the count") {
				REQUIRE(var.getNumListeners() == listeners.size());
				WHEN("We remove the listener") {
					for (auto &listener: listeners) {
						var.removeListener(&listener);
					}
					THEN("This should be reflected in the count") {
						REQUIRE(var.getNumListeners() == 0);
						for (auto &listener: listeners) {
							REQUIRE_FALSE(var.isListener(&listener));
						}
					}
				}
			}
		}
	}
}

SCENARIO("Removing a non existent listener doesnt cause an issue", "[Variable]") {
	GIVEN("A primitive variable and a listener") {
		Variable<float> var {0.5f};
		VariableListener listener;
		REQUIRE(var.getNumListeners() == 0);
		WHEN("We remove that listener") {
			REQUIRE_NOTHROW(var.removeListener(&listener));
			REQUIRE_NOTHROW(var.isListener(&listener));
			REQUIRE_NOTHROW(var.isListener(nullptr));
			REQUIRE(var.getNumListeners() == 0);
		}
	}
}

SCENARIO("Variables can be copied", "[Variable]") {
	GIVEN("A primitive variable and a listener") {
		Variable<float> var {0.5f};
		VariableListener listener;
		var.addListener(&listener);

		WHEN("We assign the variable") {
			Variable<float> secondVar {1.f};
			secondVar = var;
			THEN("The first and second variables should be the same") {
				REQUIRE(var == 0.5f);
				REQUIRE(secondVar == 0.5f);
				REQUIRE(var.getNumListeners() == 1);
				REQUIRE(secondVar.getNumListeners() == 1);
			}
		}
	}
}

SCENARIO("Variables can be copy constructed", "[Variable]") {
	GIVEN("A primitive variable and a listener") {
		Variable<float> var {0.5f};
		VariableListener listener;
		var.addListener(&listener);

		WHEN("We assign the variable") {
			Variable<float> secondVar {var};
			THEN("The first and second variables should be the same") {
				REQUIRE(var == 0.5f);
				REQUIRE(secondVar == 0.5f);
				REQUIRE(var.getNumListeners() == 1);
				REQUIRE(secondVar.getNumListeners() == 1);
			}
		}
	}
}

SCENARIO("Variable listeners are copied", "[Variable]") {
	GIVEN("A primitive variable and a listener") {
		Variable<float> var1 {0.5f};
		Variable<float> var2 {1.f};
		VariableListener listener1;
		VariableListener listener2;
		var1.addListener(&listener1);
		var2.addListener(&listener2);

		WHEN("We assign the variable") {
			var2 = var1;
			THEN("The first and second variables should be the same") {
				REQUIRE(var1 == 0.5f);
				REQUIRE(var2 == 0.5f);
				REQUIRE(var1.getNumListeners() == 1);
				REQUIRE(var2.getNumListeners() == 1);
			}
		}
	}
}

SCENARIO("Variables can be watched by multiple watchers", "[Variable]") {
	GIVEN("a primitive variable and multiple watchers") {
		Variable<float> var {0.5f};
		VariableWatcher<float> watcher1 {var, [](float) {}};
		VariableWatcher<float> watcher2 {var, [](float) {}};
		REQUIRE(var.getNumListeners() == 2);
	}
}

SCENARIO("Atomic variables can be watched", "[Variable]") {
	GIVEN("an atomic variable and multiple watchers") {
		Variable<std::atomic<float>> var {0.5f};
		VariableWatcher<std::atomic<float>> watcher1 {var, [](float) {}};
		VariableWatcher<std::atomic<float>> watcher2 {var, [](float) {}};
		REQUIRE(var.getNumListeners() == 2);
	}
}

SCENARIO("Callbacks of watchers are called on change", "[Variable]") {
	GIVEN("a primitive variable and multiple watchers") {
		Variable<float> var {0.5f};

		auto called1 = false;
		auto called2 = false;
		VariableWatcher<float> watcher1 {var, [&](float) { called1 = true; }};
		VariableWatcher<float> watcher2 {var, [&](float) { called2 = true; }};
		REQUIRE(var.getNumListeners() == 2);
		WHEN("Variable is changed") {
			var = 1.f;
			THEN("The watchers should be notified") {
				REQUIRE(called1);
				REQUIRE(called2);
			}
		}
	}
}

SCENARIO("when you copy assign a watcher it should be removed from the original variable", "[Variable]") {
	GIVEN("Some variables and watchers") {
		Variable<bool> variable1 {false};
		Variable<bool> variable2 {false};
		int watcher1Count = 0;
		int watcher2Count = 0;
		VariableWatcher<bool> watcher1 {variable1, [&](bool) { ++watcher1Count; }};
		VariableWatcher<bool> watcher2 {variable2, [&](bool) { ++watcher2Count; }};

		REQUIRE(watcher1Count == 0);
		REQUIRE(watcher2Count == 0);
		REQUIRE(variable1.getNumListeners() == 1);
		REQUIRE(variable2.getNumListeners() == 1);

		WHEN("the first watcher is copied to the second variable") {
			watcher1 = watcher2;
			THEN("The listeners are updated properly") {
				REQUIRE(watcher1Count == 0);
				REQUIRE(watcher2Count == 0);
				REQUIRE(variable1.getNumListeners() == 0);
				REQUIRE(variable2.getNumListeners() == 2);
				AND_WHEN("Variable 1 is changed") {
					variable1 = true;

					THEN("no watchers are notified") {
						REQUIRE(watcher1Count == 0);
						REQUIRE(watcher2Count == 0);
					}
				}
				AND_WHEN("the second variable is changed") {
					variable2 = true;
					THEN("both watcher notifications are the same") {
						REQUIRE(watcher1Count == 0);
						REQUIRE(watcher2Count == 2);
					}
				}
			}
		}
	}
}