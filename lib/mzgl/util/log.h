//
//  log.h
//
//  Created by Marek Bereza on 26/03/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#pragma once

/*
 
 
 
 log::v("verbose logging");
 log::n("");
 log::e("channel") << "hello test";
 
 or
 
 log::w("channel", "this is a %f thing\n", 1.2345);

 */
#include <string>
#include <sstream>
#include <vector>
#include <list>

#ifdef __ANDROID__
#include <android/log.h>
#endif
class LogListener {
public:
	virtual void stringLogged(const std::string & m) = 0;
};


namespace Log {
	
	class Logger {
	public:
		static constexpr int userLogLevel = 1;
		Logger() {
			
		}
		template <class T>
		Logger& operator<<(const T& value){
			msg << value;
			return *this;
		}
		
		virtual ~Logger();
		
		// you can tell the logger to log to a file as well as
		// stdout, to a specific path
		static void startSavingToFile(std::string path);
		
		// if you want to get that file and stop the saving, call this
		static std::string stopSavingToFile();
		static bool isSavingToFile();
		
		static void addListener(LogListener *listener);
		static void removeListener(LogListener *listener);
		
		static std::vector<LogListener*> listeners;
	protected:
		std::string levelName;
		int level = 0;
	private:
		std::stringstream msg;
		static std::string logFile;
	
	};


	class v: public Logger {
	public:
		virtual ~v() {
			level = 1;
			levelName = "verbose";
		}
	};

	class d: public Logger {
	public:
		virtual ~d() {
			level = 2;
			levelName = "debug";
		}
	};

	class i: public Logger {
	public:
		virtual ~i() {
			level = 3;
			levelName = "info";
		}
	};
	class w: public Logger {
	public:
		virtual ~w() {
			level = 4;
			levelName = "warning";
		}
	};
	class e: public Logger {
	public:
		virtual ~e() {
			level = 5;
			levelName = "error";
		}
	};
};

// this is for debugging when running as a plugin on iOS
// because it comes up in the Console app on mac - its
// the only way to do logging in that setup. On other platforms
// it just logs to Log::d()
void iosLog(std::string msg);

// handy class for something that you can tap into for logs
class LogCapturer : public LogListener {
public:
	LogCapturer() {
		Log::Logger::addListener(this);
	}
	~LogCapturer() {
		Log::Logger::removeListener(this);
	}
	virtual void stringLogged(const std::string & m) {
		lines.push_back(m);
	}
	
	std::list<std::string> lines;
};
