//
//  Timer.h
//  mzgl
//
//  Helpful debugging and performance thing.
//
//  Created by Marek Bereza on 10/08/2022.
//  Copyright © 2022 Marek Bereza. All rights reserved.
//

#pragma once
#include <chrono>
struct Timer {
	std::chrono::system_clock::time_point startTime;
	
	void start() {
		startTime = std::chrono::system_clock::now();
	}

	std::uint64_t stop() {
		auto duration = std::chrono::system_clock::now() - startTime;
		auto us = std::chrono::microseconds(duration).count();
		return us / 1000;
	}
	
	void stopAndPrint(const std::string &msg) {
		auto ms = stop();
		printf("%s took %d ms\n", msg.c_str(), ms);
	}
};
