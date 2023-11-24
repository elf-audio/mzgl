//
//  PluginParameter.h
//  mzgl
//
//  Created by Marek Bereza on 23/07/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#pragma once

class PluginParameter {
public:
	std::string name;
	float		defaultValue;
	float		from;
	float		to;
	std::string unit;
	// set this to true while your UI is interacting
	// with a parameter to ignore incoming parameter
	// automation control.
	std::atomic<bool> ignoreAutomation {false};

	void beginIgnoringAutomation() { ignoreAutomation.store(true); }

	void endIgnoringAutomation() { ignoreAutomation.store(false); }

	bool isIgnoringAutomation() { return ignoreAutomation.load(); }

	enum class Type { Float, Int, Indexed };

	Type type;

	PluginParameter(std::string name,
					float		defaultValue,
					float		from = 0.f,
					float		to	 = 1.f,
					std::string unit = "")
		: name(name)
		, defaultValue(defaultValue)
		, from(from)
		, to(to)
		, type(Type::Float)
		, unit(unit) {
		assert(std::atomic_is_lock_free(&value));
		set(defaultValue);
	}

	PluginParameter(
		std::string name, int defaultValue, int from = 0, int to = 1, std::string unit = "")
		: name(name)
		, defaultValue(defaultValue)
		, from(from)
		, to(to)
		, type(Type::Int)
		, unit(unit) {
		assert(std::atomic_is_lock_free(&value));
		set(defaultValue);
	}

	std::vector<std::string> options;

	PluginParameter(std::string name, int defaultValue, const std::vector<std::string> &options)
		: name(name)
		, defaultValue(defaultValue)
		, options(options)
		, type(Type::Indexed)
		, from(0)
		, to(options.size() - 1) {
		assert(std::atomic_is_lock_free(&value));
		set(defaultValue);
	}

	float get() const noexcept { return value; }
		  operator float() const noexcept { return value; }

	void set(float v) noexcept { value.store(v); }

	PluginParameter &operator=(float value) {
		if (value != this->value) {
			//               setValueNotifyingHost (convertTo0to1 (newValue));
		}
		return *this;
	}

private:
	std::atomic<float> value;
};
