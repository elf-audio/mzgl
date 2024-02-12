#pragma once

#include <vector>
#include <algorithm>

template <class T>
class AtomicVariable {
public:
	class Listener {
	public:
		virtual ~Listener()			   = default;
		virtual void variableChanged() = 0;
	};

	explicit AtomicVariable(const T &var)
	// : variable(Var)
	{
		variable.store(var);
	}

	[[nodiscard]] AtomicVariable<T> &operator=(const T &var) {
		if (variable == var) return *this;

		variable = var;
		for (auto *l: listeners) {
			l->variableChanged();
		}
		return *this;
	}

	bool operator==(const T &var) { return variable == var; }
	bool operator!=(const T &var) { return variable != var; }
	bool operator==(const T &var) const { return variable == var; }
	bool operator!=(const T &var) const { return variable != var; }

	[[nodiscard]] operator T() { return variable; }
	[[nodiscard]] operator T() const { return variable; }
	auto getNumListeners() { return listeners.size(); }
	void addListener(Listener *listener) { listeners.push_back(listener); }
	void removeListener(Listener *listener) {
		listeners.erase(std::remove_if(std::begin(listeners),
									   std::end(listeners),
									   [listener](auto &&l) { return l == listener; }),
						std::end(listeners));
	}

	void updateNoNotify(T newValue) { variable.store(newValue); }

private:
	std::atomic<T> variable;
	std::vector<Listener *> listeners;
};

template <class T>
class AtomicVariableWatcher : private AtomicVariable<T>::Listener {
public:
	AtomicVariableWatcher(AtomicVariable<T> &var, const std::function<void(T)> &onChanged)
		: variable(var)
		, callback(std::move(onChanged)) {
		variable.addListener(this);
	}

	~AtomicVariableWatcher() { variable.removeListener(this); }
	void updateVariableNoNotify(T newValue) { variable.updateNoNotify(newValue); }

private:
	void variableChanged() override { callback(variable); }

	AtomicVariable<T> &variable;
	std::function<void(T)> callback;
};
