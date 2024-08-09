//
//  log.cpp
//  mzgl
//
//  Created by Marek Bereza on 20/09/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#include "log.h"
#include <fstream>
#include <vector>
std::string Log::Logger::logFile;
#ifdef __APPLE__
#	include <os/log.h>
#endif
namespace Log {
	std::ofstream logStream;
	bool isLoggingToFile = false;
}; // namespace Log

void Log::Logger::startSavingToFile(std::string path, bool append) {
	logFile = path;
	if (append) {
		logStream.open(logFile.c_str(), std::ios_base::app);
	} else {
		logStream.open(logFile.c_str());
	}
	isLoggingToFile = true;
}

std::string Log::Logger::stopSavingToFile() {
	logStream.close();
	isLoggingToFile = false;
	return logFile;
}

bool Log::Logger::isSavingToFile() {
	return isLoggingToFile;
}

void Log::Logger::addListener(LogListener *listener) {
	listeners.push_back(listener);
}

void Log::Logger::removeListener(LogListener *listener) {
	for (int i = 0; i < listeners.size(); i++) {
		if (listeners[i] == listener) {
			listeners.erase(listeners.begin() + i);
			return;
		}
	}
}

std::vector<LogListener *> Log::Logger::listeners;

Log::Logger::~Logger() {
	if (level >= userLogLevel) {
		msg << std::endl;
#ifdef __ANDROID__
		if (level == 4) {
			__android_log_print(ANDROID_LOG_ERROR, "mzgl", "%s", msg.str().c_str());
		} else if (level == 3) {
			__android_log_print(ANDROID_LOG_WARN, "mzgl", "%s", msg.str().c_str());
		} else {
			__android_log_print(ANDROID_LOG_INFO, "mzgl", "%s", msg.str().c_str());
		}
#else

#	ifdef __APPLE__
		os_log_type_t osLevel = OS_LOG_TYPE_DEFAULT;
		switch (level) {
			case 1: break;
			case 4:
			case 2: osLevel = OS_LOG_TYPE_DEBUG; break;
			case 3: osLevel = OS_LOG_TYPE_INFO; break;
			case 5: osLevel = OS_LOG_TYPE_ERROR; break;
		}

		os_log_with_type(OS_LOG_DEFAULT, osLevel, "[%{public}s] %{public}s", levelName.c_str(), msg.str().c_str());
#	else
		if (level >= 5) {
			fprintf(stderr, "[%s] %s", levelName.c_str(), msg.str().c_str());
			fflush(stderr);
		} else {
			printf("[%s] %s", levelName.c_str(), msg.str().c_str());
			fflush(stdout);
		}

#	endif
#endif
		if (isLoggingToFile) {
			logStream << std::string("[") << levelName << "] " << msg.str();
		}

		if (listeners.size() > 0) {
			std::string m = "[" + levelName + "] " + msg.str();
			for (auto *l: listeners) {
				l->stringLogged(m);
			}
		}
	}
}

#ifdef __APPLE__
#	include <Foundation/Foundation.h>

void iosLog(std::string msg) {
	NSLog(@"[MZGLEffectAU ?]%s\n", msg.c_str());
}
#else
void iosLog(std::string msg) {
	Log::d() << msg;
}
#endif
