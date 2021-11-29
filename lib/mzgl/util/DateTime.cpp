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

//#ifndef _WIN32
DateTime::DateTime(int year, int month, int day, int hour, int min, int sec) {
	this->year = year;
	this->month = month;
	this->day = day;
	this->hour = hour;
	this->min = min;
	this->sec = sec;
}

DateTime::DateTime() {
	time_t t = time(NULL);
	tm *timePtr = localtime(&t);
	
	this->year = timePtr->tm_year;
	this->month = timePtr->tm_mon+1;
	this->day = timePtr->tm_mday;
	this->hour = timePtr->tm_hour;
	this->min = timePtr->tm_min;
	this->sec = timePtr->tm_sec;
}
string DateTime::timestampString() {
	
	return toYear(this->year) + "-" + pad2(month) + "-" + pad2(day) + "--" + pad2(hour) + "." + pad2(min) + "." + pad2(sec);
}
void DateTime::fromSql(string sql) {
	this->year  = stoi(sql.substr(0, 4));
	this->month = stoi(sql.substr(5, 2));
	this->day 	= stoi(sql.substr(8, 2));
	this->hour 	= stoi(sql.substr(11,2));
	this->min 	= stoi(sql.substr(14,2));
	this->sec 	= stoi(sql.substr(17,2));
}
string DateTime::pad2(int i) {
	if(i>9) return to_string(i);
	else return "0" + to_string(i);
}
string DateTime::toSql() {
	string d = toYear(this->year);

	
	
	d += "-";
	d += pad2(month);
	d += "-";
	d += pad2(day);
	d += " ";
	d += pad2(hour);
	d += ":";
	d += pad2(min);
	d += ":";
	d += pad2(sec);
	return d;
}


int DateTime::getDayOfWeek() {
	// http://en.wikipedia.org/wiki/Zeller's_congruence
	int K = year % 100;
	int q = day;
	int m = month;
	int J = year / 100;
	int h = q + ((13*m+1)/5) + K + (K/4) + (J/4) + 5*J;
	h = h % 7;
	return h;
}
string DateTime::dayName(int dayIndex) {
	switch(dayIndex) {
		case 0: return "Mon";	break;
		case 1: return "Tue";	break;
		case 2: return "Wed";	break;
		case 3: return "Thu";	break;
		case 4: return "Fri";	break;
		case 5: return "Sat";	break;
		case 6: return "Sun";	break;
		default:return "error!"; break;
	}
}
string DateTime::monthName(int monthIndex) {
	switch(monthIndex) {
		case 1:	return "Jan";		break;
		case 2:	return "Feb";		break;
		case 3:	return "Mar";		break;
		case 4:	return "Apr";		break;
		case 5:	return "May";		break;
		case 6:	return "Jun";		break;
		case 7:	return "Jul";		break;
		case 8:	return "Aug";		break;
		case 9:	return "Sep";		break;
		case 10:return "Oct";		break;
		case 11:return "Nov";		break;
		case 12:return "Dec";		break;
		default:return "Error";		break;

	}
}

bool DateTime::isLeapYear() {
	int a = year;
	return ((a % 4 == 0 && a % 100 != 0) || a % 400 == 0);
}
int DateTime::daysInMonth(int month, bool leapYear) { // 1 indexed
	switch(month) {
		case 1: return 31;
		case 2: return leapYear?29:28;
		case 3: return 31;
		case 4: return 30;
		case 5: return 31;
		case 6: return 30;
		case 7: return 31;
		case 8: return 31;
		case 9: return 30;
		case 10: return 31;
		case 11: return 30;
		case 12: return 31;
		default: return 0;
	}
}


long DateTime::timestamp() const {
	
	struct tm ptm;
	memset(&ptm, 0, sizeof(struct tm));
	ptm.tm_sec = sec;
	ptm.tm_min = min;
	ptm.tm_hour = hour;
	ptm.tm_mday = day;
	ptm.tm_mon = month - 1;
	ptm.tm_year = year - 1900;
	
	return mktime(&ptm);
}

void DateTime::addDays(int days) {
	long inc = days * 24 * 60 * 60;
	setFromTimestamp(timestamp() + inc);
}

void DateTime::setFromTimestamp(long timestamp) {

	time_t time = timestamp;
	struct tm *tinfo = gmtime(&time);
	year = tinfo->tm_year + 1900;
	month = tinfo->tm_mon + 1;
	day = tinfo->tm_mday;
	hour = tinfo->tm_hour;
	min = tinfo->tm_min;
	sec = tinfo->tm_sec;
}

int DateTime::daysSinceEpoch() {
	DateTime dt(year, month, day);
	return dt.timestamp() / (24 * 60 * 60);
}
string DateTime::toYear(int yr) {
	yr -= 100;
	if(yr>1000) {
		return to_string(yr);
	} else if(yr>100) {
		return "2" + to_string(yr);
	} else if(yr>10) {
		return "20" + to_string(yr);
	} else if(yr>1) {
		return "200" + to_string(yr);
	} else {
		return "2000";
	}
}
//#endif
