//
//  MZOpenGLView.h
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//
#pragma once

#include <functional>
#include <string>
#include <atomic>
//#include <algorithm>
#include "stringUtil.h"
#include "mzgl/util/log.h"
#include <memory>

#include "log.h"

#include "MidiMessage.h"

/**
 * This is a midi device info object, hopefully will have more in it.
 * At the moment, it's copied around a lot, so to identify it use
 * the equality operator, which uses the id field which is a temporary
 * unique counter based id for the current app run.
 *
 * Also, don't store this by reference!
 */
class MidiDevice {
public:
	enum class Direction {
		Input,
		Output,
	};

	std::string name;
	int id;
	Direction direction = Direction::Input;

	MidiDevice(std::string name = "", Direction _direction = Direction::Input)
		: name(name)
		, direction(_direction) {
		static std::atomic<int> idCounter {0};
		id = idCounter++;
	}

	virtual ~MidiDevice() = default;

	[[nodiscard]] virtual bool operator==(const MidiDevice &other) const { return id == other.id; }
	[[nodiscard]] virtual bool operator!=(const MidiDevice &other) const { return id != other.id; }

private:
};

class MidiListener {
public:
	virtual ~MidiListener() {}
	virtual void
		midiReceived(const std::shared_ptr<MidiDevice> &device, const MidiMessage &m, uint64_t timestamp) = 0;
};
