//
//  Timer.h
//
//  Created by Marek Bereza on 10/08/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once

#include <chrono>
#include "log.h"
struct Stopwatch {
	std::chrono::steady_clock::time_point startTime;

	/**
     * @brief Starts Stopwatch.
     */
	void start() { startTime = std::chrono::steady_clock::now(); }

	/**
     * @brief Returns the time delta from the start.
     *
     * @return time delta in milliseconds
     */
	std::uint64_t getTimeDeltaMs() {
		auto duration = std::chrono::steady_clock::now() - startTime;
		auto ms		  = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
		return ms.count();
	}

	/**
     * @brief Prints the time delta measured from the start.
     *
     * @param message Message to be printed along with the measured time delta.
     */
	void printTimeDeltaMs(const std::string &message) {
		auto ms = getTimeDeltaMs();
		Log::d() << message << " took " << ms << "ms";
	}
};
