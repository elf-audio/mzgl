/**
 *  DateTime.cpp
 *
 *  Created by Marek Bereza on 05/12/2011.
 */

#include "DateTime.h"

#include <time.h>
#include <stdlib.h>
#include <cstring>
using namespace std;
#include "stringUtil.h"

DateTime::DateTime(int year, int month, int day, int hour, int min, int sec) {
	this->year	= year;
	this->month = month;
	this->day	= day;
	this->hour	= hour;
	this->min	= min;
	this->sec	= sec;
}

DateTime DateTime::now() {
	return DateTime();
}
DateTime::DateTime() {
	time_t t	= time(nullptr);
	tm *timePtr = localtime(&t);

	year  = timePtr->tm_year + 1900;
	month = timePtr->tm_mon + 1;
	day	  = timePtr->tm_mday;
	hour  = timePtr->tm_hour;
	min	  = timePtr->tm_min;
	sec	  = timePtr->tm_sec;
}
string DateTime::timestampString() const {
	return zeroPad(year, 4) + "-" + zeroPad2(month) + "-" + zeroPad2(day) + "--" + zeroPad2(hour) + "."
		   + zeroPad2(min) + "." + zeroPad2(sec);
}
void DateTime::fromSql(string sql) {
	year  = stoi(sql.substr(0, 4));
	month = stoi(sql.substr(5, 2));
	day	  = stoi(sql.substr(8, 2));
	hour  = stoi(sql.substr(11, 2));
	min	  = stoi(sql.substr(14, 2));
	sec	  = stoi(sql.substr(17, 2));
}

string DateTime::toSql() const {
	return zeroPad(this->year, 4) + "-" + zeroPad2(month) + "-" + zeroPad2(day) + " " + zeroPad2(hour) + ":"
		   + zeroPad2(min) + ":" + zeroPad2(sec);
}

int DateTime::getDayOfWeek() const {
	// http://en.wikipedia.org/wiki/Zeller's_congruence
	int K = year % 100;
	int q = day;
	int m = month;
	int J = year / 100;
	int h = q + ((13 * m + 1) / 5) + K + (K / 4) + (J / 4) + 5 * J;
	h	  = h % 7;
	return h;
}
string DateTime::dayName(int dayIndex) {
	switch (dayIndex) {
		case 0: return "Mon";
		case 1: return "Tue";
		case 2: return "Wed";
		case 3: return "Thu";
		case 4: return "Fri";
		case 5: return "Sat";
		case 6: return "Sun";
		default: return "error!";
	}
}
string DateTime::monthName(int monthIndex) {
	switch (monthIndex) {
		case 1: return "Jan";
		case 2: return "Feb";
		case 3: return "Mar";
		case 4: return "Apr";
		case 5: return "May";
		case 6: return "Jun";
		case 7: return "Jul";
		case 8: return "Aug";
		case 9: return "Sep";
		case 10: return "Oct";
		case 11: return "Nov";
		case 12: return "Dec";
		default: return "Error";
	}
}

bool DateTime::isLeapYear(int year) {
	// A year is a leap year if it is divisible by 4
	// but not divisible by 100, except when it is divisible by 400.
	return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int DateTime::daysInMonth(int month, bool leapYear) { // 1 indexed
	switch (month) {
		case 2: return leapYear ? 29 : 28;
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12: return 31;

		case 4:
		case 6:
		case 9:
		case 11: return 30;

		default: return 0;
	}
}

long DateTime::timestamp() const {
	struct tm ptm;
	memset(&ptm, 0, sizeof(struct tm));
	ptm.tm_sec	= sec;
	ptm.tm_min	= min;
	ptm.tm_hour = hour;
	ptm.tm_mday = day;
	ptm.tm_mon	= month - 1;
	ptm.tm_year = year - 1900;

	return mktime(&ptm);
}

void DateTime::addDays(int days) {
	long inc = days * 24 * 60 * 60;
	setFromTimestamp(timestamp() + inc);
}

void DateTime::setFromTimestamp(long timestamp) {
	time_t time		 = timestamp;
	struct tm *tinfo = gmtime(&time);
	year			 = tinfo->tm_year + 1900;
	month			 = tinfo->tm_mon + 1;
	day				 = tinfo->tm_mday;
	hour			 = tinfo->tm_hour;
	min				 = tinfo->tm_min;
	sec				 = tinfo->tm_sec;
}

int DateTime::daysSinceEpoch() const {
	DateTime dt(year, month, day);
	return static_cast<int>(dt.timestamp() / (24 * 60 * 60));
}
