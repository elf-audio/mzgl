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

namespace Log {
	std::ofstream logStream;
	bool isLoggingToFile = false;
};

void Log::Logger::startSavingToFile(std::string path) {
	
	logFile = path;
	
	logStream.open(logFile.c_str());
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
#ifdef DEBUG
	listeners.push_back(listener);
#endif
}
void Log::Logger::removeListener(LogListener *listener) {
	for(int i =0 ; i < listeners.size(); i++) {
        if(listeners[i]==listener) {
            listeners.erase(listeners.begin() + i);
            return;
        }
	}
}

std::vector<LogListener*> Log::Logger::listeners;



Log::Logger::~Logger() {
	if(level>=userLogLevel) {
		msg << std::endl;
#ifdef __ANDROID__
		if(level==4) {
			__android_log_print(ANDROID_LOG_ERROR, "mzgl", "%s", msg.str().c_str());
		} else if(level==3) {
			__android_log_print(ANDROID_LOG_WARN, "mzgl", "%s", msg.str().c_str());
		} else {
			__android_log_print(ANDROID_LOG_INFO, "mzgl", "%s", msg.str().c_str());
		}
#else

		
		fprintf(stderr, "[%s] %s", levelName.c_str(), msg.str().c_str());
		fflush(stderr);
#endif
		if(isLoggingToFile) {
			logStream << std::string("[") << levelName << "] " << msg.str();
		}
		
#ifdef DEBUG
		std::string m = "[" + levelName + "] " + msg.str();
		for(auto *l : listeners) {
			l->stringLogged(m);
		}
#endif
	}
}
