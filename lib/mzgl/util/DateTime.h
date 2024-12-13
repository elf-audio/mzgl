/**     ___           ___           ___                         ___           ___     
 *     /__/\         /  /\         /  /\         _____         /  /\         /__/|    
 *    |  |::\       /  /::\       /  /::|       /  /::\       /  /::\       |  |:|    
 *    |  |:|:\     /  /:/\:\     /  /:/:|      /  /:/\:\     /  /:/\:\      |  |:|    
 *  __|__|:|\:\   /  /:/~/::\   /  /:/|:|__   /  /:/~/::\   /  /:/  \:\   __|__|:|    
 * /__/::::| \:\ /__/:/ /:/\:\ /__/:/ |:| /\ /__/:/ /:/\:| /__/:/ \__\:\ /__/::::\____
 * \  \:\~~\__\/ \  \:\/:/__\/ \__\/  |:|/:/ \  \:\/:/~/:/ \  \:\ /  /:/    ~\~~\::::/
 *  \  \:\        \  \::/          |  |:/:/   \  \::/ /:/   \  \:\  /:/      |~~|:|~~ 
 *   \  \:\        \  \:\          |  |::/     \  \:\/:/     \  \:\/:/       |  |:|   
 *    \  \:\        \  \:\         |  |:/       \  \::/       \  \::/        |  |:|   
 *     \__\/         \__\/         |__|/         \__\/         \__\/         |__|/   
 *
 *  Description: 
 *				 
 *  DateTime.h, created by Marek Bereza on 05/12/2011.
 */

#pragma once
#include <string>

class DateTime {
public:
	int year;
	int month;
	int day;
	int hour;
	int min;
	int sec;
	DateTime(int year, int month, int day, int hour = 0, int min = 0, int sec = 0);

	enum DayOfWeek { Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday };
	// creates a date with todays date and now's time.
	DateTime();
	static DateTime now();
	std::string toSql() const;
	void fromSql(std::string sql);
	int getDayOfWeek() const;
	void addDays(int numDays);
	void setFromTimestamp(long timestamp);
	long timestamp() const;
	static bool isLeapYear(int year);
	int daysSinceEpoch() const;
	static std::string monthName(int monthIndex); // 1 is January
	static std::string dayName(int dayIndex); // 0 is monday
	static int daysInMonth(int month, bool leapYear); // 1 indexed
	std::string timestampString() const;

	bool operator<(const DateTime &other) const { return timestamp() < other.timestamp(); }
};
