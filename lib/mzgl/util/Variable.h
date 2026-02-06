#pragma once

#include <vector>
#include <atomic>
#include <algorithm>
#include <functional>
#include <string>
#include "mzAssert.h"

template <class T>
class Variable {
public:
	class Listener {
	public:
		virtual ~Listener()			   = default;
		virtual void variableChanged() = 0;
	};

	class LambdaListener : public Listener {
	public:
		LambdaListener(Variable &_b, std::function<void(T)> _cb)
			: cb(std::move(_cb))
			, b(_b) {}

		void variableChanged() override { cb(b); }

	private:
		std::function<void(T)> cb;
		Variable &b;
	};

	Variable(const Variable &other)
		: Variable(other.variable) {}
	Variable &operator=(const Variable &other) {
		if (this != &other) {
			assign(other.variable);
		}

		return *this;
	}
	explicit Variable(const T &var) { assign(var); }

	[[maybe_unused]] const Variable<T> &operator=(const T &var) {
		if (variable == var) {
			return *this;
		}

		assign(var);
		return *this;
	}
	void setWithoutNotification(const T &var) {
		if constexpr (is_atomic<T>::value) {
			variable.store(var.load());
		} else {
			variable = var;
		}
	}

	[[nodiscard]] const T &get() const { return variable; }
	bool operator==(const T &var) const { return variable == var; }
	bool operator!=(const T &var) const { return variable != var; }
	bool operator!() const { return !variable; }

	[[nodiscard]] operator const T &() const { return variable; }

	auto getNumListeners() { return listeners.size(); }
	void addListener(Listener *listener) { listeners.push_back(listener); }
	void addListener(std::function<void(T)> variableChanged) {
		auto listener = std::make_shared<LambdaListener>(*this, variableChanged);
		ownedListeners.push_back(listener);
		listeners.push_back(listener.get());
	}
	void removeListener(Listener *listener) {
		listeners.erase(std::remove_if(std::begin(listeners),
									   std::end(listeners),
									   [listener](auto &&l) { return l == listener; }),
						std::end(listeners));
	}
	[[nodiscard]] bool isListener(Listener *listener) const {
		return std::find_if(std::begin(listeners),
							std::end(listeners),
							[listener](auto &&otherListener) { return otherListener == listener; })
			   != std::end(listeners);
	}

private:
	void assign(const T &var) {
		if constexpr (is_atomic<T>::value) {
			variable.store(var.load());
		} else {
			variable = var;
		}

		for (auto *listener: listeners) {
			listener->variableChanged();
		}
	}

	template <typename InnerT = T>
	struct is_atomic {
		static const bool value = false;
	};
	template <typename InnerT>
	struct is_atomic<std::atomic<InnerT>> {
		static const bool value = true;
	};

	T variable;

	std::vector<Listener *> listeners;
	std::vector<std::shared_ptr<Listener>> ownedListeners;
};

template <class T>
class AtomicVariable {
public:
	class Listener {
	public:
		virtual ~Listener()			   = default;
		virtual void variableChanged() = 0;
	};

	AtomicVariable(const AtomicVariable &other)
		: AtomicVariable(other.get()) {}
	AtomicVariable &operator=(const AtomicVariable &other) {
		if (this != &other) {
			assign(other.get());
		}

		return *this;
	}
	explicit AtomicVariable(const T &var) { assign(var); }

	[[maybe_unused]] const AtomicVariable<T> &operator=(const T &var) {
		if (variable == var) {
			return *this;
		}

		assign(var);
		return *this;
	}

	bool operator==(const T &var) const { return variable == var; }
	bool operator!=(const T &var) const { return variable != var; }
	bool operator!() const { return !variable; }

	[[nodiscard]] operator T() const { return variable.load(); }

	[[nodiscard]] T get() const { return variable.load(); }
	auto getNumListeners() { return listeners.size(); }
	void addListener(Listener *listener) { listeners.push_back(listener); }
	void removeListener(Listener *listener) {
		listeners.erase(std::remove_if(std::begin(listeners),
									   std::end(listeners),
									   [listener](auto &&l) { return l == listener; }),
						std::end(listeners));
	}
	[[nodiscard]] bool isListener(Listener *listener) const {
		return std::find_if(std::begin(listeners),
							std::end(listeners),
							[listener](auto &&otherListener) { return otherListener == listener; })
			   != std::end(listeners);
	}

private:
	void assign(const T &var) {
		variable.store(var);

		for (auto *listener: listeners) {
			listener->variableChanged();
		}
	}

	std::atomic<T> variable;
	std::vector<Listener *> listeners;
};

template <class T>
class VariableWatcher : private Variable<T>::Listener {
public:
	VariableWatcher(std::reference_wrapper<Variable<T>> var, const std::function<void(const T &)> &onChanged)
		: variable(var)
		, callback(std::move(onChanged)) {
		variable.get().addListener(this);
	}

	VariableWatcher(Variable<T> &var, const std::function<void(const T &)> &onChanged)
		: VariableWatcher(std::ref(var), onChanged) {}

	explicit VariableWatcher(const VariableWatcher &other)
		: variable {other.variable} {
		*this = other;
	}

	VariableWatcher &operator=(const VariableWatcher &other) {
		if (this != &other) {
			variable.get().removeListener(this);
			variable = other.variable;
			callback = other.callback;
			variable.get().addListener(this);
		}

		return *this;
	}

	~VariableWatcher() { variable.get().removeListener(this); }

	const T &getValue() const { return variable.get(); }
	void setValue(const T &newValue) { variable.get() = newValue; }

private:
	void variableChanged() override {
		if (callback != nullptr) {
			callback(variable.get());
		}
	}

	std::reference_wrapper<Variable<T>> variable;
	std::function<void(const T &)> callback;
};

template <class T>
class AtomicVariableWatcher : private AtomicVariable<T>::Listener {
public:
	AtomicVariableWatcher(std::reference_wrapper<AtomicVariable<T>> var,
						  const std::function<void(const T &)> &onChanged)
		: variable(var)
		, callback(std::move(onChanged)) {
		variable.get().addListener(this);
	}

	AtomicVariableWatcher(AtomicVariable<T> &var, const std::function<void(const T &)> &onChanged)
		: AtomicVariableWatcher(std::ref(var), onChanged) {}

	explicit AtomicVariableWatcher(const AtomicVariableWatcher &other)
		: variable {other.variable} {
		*this = other;
	}

	AtomicVariableWatcher &operator=(const AtomicVariableWatcher &other) {
		if (this != &other) {
			variable.get().removeListener(this);
			variable = other.variable;
			callback = other.callback;
			variable.get().addListener(this);
		}

		return *this;
	}

	~AtomicVariableWatcher() { variable.get().removeListener(this); }

	const T getValue() const { return variable.get(); }
	void setValue(const T &newValue) { variable.get() = newValue; }

private:
	void variableChanged() override {
		if (callback != nullptr) {
			callback(variable.get());
		}
	}

	std::reference_wrapper<AtomicVariable<T>> variable;
	std::function<void(const T &)> callback;
};

class IndexedVariable {
public:
	class Listener {
	public:
		virtual ~Listener() = default;
		virtual void indexedVariableChanged(int value) {}
		virtual void indexedVariableOptionsChanged() {}
	};

	IndexedVariable(int v = 0, const std::vector<std::string> &_options = {})
		: value(v)
		, options(_options) {}
	IndexedVariable &operator=(int v) {
		assign(v);
		return *this;
	}
	IndexedVariable &operator=(const IndexedVariable &o) {
		assign(o.value);
		return *this;
	}
	const std::vector<std::string> &getOptions() const { return options; }
	void setOptions(const std::vector<std::string> &_options) {
		options = _options;
		if (value >= static_cast<int>(options.size())) {
			value = 0; // reset to first option if current value is out of range
		}
		for (auto *listener: listeners) {
			listener->indexedVariableOptionsChanged();
		}
	}
	int getNumOptions() const { return options.size(); }
	int getValue() const { return value; }

private:
	class LambdaListener : public Listener {
	public:
		LambdaListener(IndexedVariable &_b, std::function<void(int)> _cb)
			: cb(std::move(_cb))
			, b(_b) {}

		void indexedVariableChanged(int a) override { cb(a); }

	private:
		std::function<void(int)> cb;
		IndexedVariable &b;
	};

public:
	void addListener(std::function<void(int)> variableChanged) {
		auto listener = std::make_shared<LambdaListener>(*this, std::move(variableChanged));
		ownedListeners.push_back(listener);
		listeners.push_back(listener.get());
	}
	void addListener(Listener *listener) { listeners.push_back(listener); }
	void removeListener(Listener *listener) {
		listeners.erase(std::remove_if(std::begin(listeners),
									   std::end(listeners),
									   [listener](auto &&l) { return l == listener; }),
						std::end(listeners));
	}
	void setWithoutNotification(int v) {
		if (v < 0 || v >= static_cast<int>(options.size())) {
			mzAssert(false, "Index out of range");
		}
		value = v;
	}

private:
	std::vector<std::string> options;
	std::vector<std::shared_ptr<Listener>> ownedListeners;
	int value = 0;
	void assign(int v) {
		if (v < 0 || v >= static_cast<int>(options.size())) {
			mzAssert(false, "Index out of range");
		}
		if (value != v) {
			value = v;
			notifyListeners();
		}
	}
	std::vector<Listener *> listeners;
	void notifyListeners() {
		for (auto *listener: listeners) {
			listener->indexedVariableChanged(value);
		}
	}
};

class IndexedVariableWatcher : private IndexedVariable::Listener {
public:
	IndexedVariableWatcher(IndexedVariable &var,
						   std::function<void(int)> onValueChanged = nullptr,
						   std::function<void()> onOptionsChanged  = nullptr)
		: variable(var)
		, valueCallback(std::move(onValueChanged))
		, optionsCallback(std::move(onOptionsChanged)) {
		variable.addListener(this);
	}

	~IndexedVariableWatcher() { variable.removeListener(this); }

	IndexedVariableWatcher(const IndexedVariableWatcher &)			  = delete;
	IndexedVariableWatcher &operator=(const IndexedVariableWatcher &) = delete;

private:
	void indexedVariableChanged(int value) override {
		if (valueCallback) valueCallback(value);
	}

	void indexedVariableOptionsChanged() override {
		if (optionsCallback) optionsCallback();
	}

	IndexedVariable &variable;
	std::function<void(int)> valueCallback;
	std::function<void()> optionsCallback;
};
