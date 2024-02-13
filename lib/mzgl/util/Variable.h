#pragma once

#include <vector>
#include <atomic>
#include <algorithm>
#include <functional>

template <class T>
class Variable {
public:
	class Listener {
	public:
		virtual ~Listener()			   = default;
		virtual void variableChanged() = 0;
	};

	explicit Variable(const T &var)
	//	: variable(var)
	{
		*this = var;
	}

	[[maybe_unused]] const Variable<T> &operator=(const T &var) {
		if (variable == var) return *this;

		if constexpr (is_atomic<T>::value) {
			variable.store(var.load());
		} else {
			variable = var;
		}

		for (auto *l: listeners) {
			l->variableChanged();
		}
		return *this;
	}

	bool operator==(const T &var) { return variable == var; }
	bool operator!=(const T &var) { return variable != var; }
	bool operator==(const T &var) const { return variable == var; }
	bool operator!=(const T &var) const { return variable != var; }
	bool operator!() const { return !variable; }

	[[nodiscard]] operator T &() { return variable; }

	[[nodiscard]] operator const T &() const { return variable; }
	auto getNumListeners() { return listeners.size(); }
	void addListener(Listener *listener) { listeners.push_back(listener); }
	void removeListener(Listener *listener) {
		listeners.erase(std::remove_if(std::begin(listeners),
									   std::end(listeners),
									   [listener](auto &&l) { return l == listener; }),
						std::end(listeners));
	}

	template <typename InnerT = T>
	struct is_atomic {
		static const bool value = false;
	};
	template <typename InnerT>
	struct is_atomic<std::atomic<InnerT>> {
		static const bool value = true;
	};

private:
	T variable;

	std::vector<Listener *> listeners;
};

template <class T>
class VariableWatcher : private Variable<T>::Listener {
public:
	VariableWatcher(Variable<T> &var, const std::function<void(const T &)> &onChanged)
		: variable(var)
		, callback(std::move(onChanged)) {
		variable.addListener(this);
	}

	~VariableWatcher() { variable.removeListener(this); }
	const T &getValue() { return variable; }
	void setValue(bool newValue) { variable = newValue; }

private:
	void variableChanged() override { callback(variable); }

	Variable<T> &variable;
	std::function<void(const T &)> callback;
};