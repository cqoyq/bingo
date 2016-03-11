/*
 * logger.h
 *
 *  Created on: 2016-2-19
 *      Author: root
 */

#ifndef LOG_LOGGER_H_
#define LOG_LOGGER_H_

#include "../xthread/spinlock.h"
using bingo::xthread::spinlock;
#include "log_data.h"

namespace bingo { namespace log {

template<typename HANDLER>
class logger {
public:
	logger(){
		cur_level_ = LOG_LEVEL_OFF;
		hdr_ = new HANDLER();
	};
	virtual ~logger(){
		delete hdr_;
	}

	void log_f(const char* tag, log_data& data){
		spinlock::scoped_lock lock(slock_);

		if(cur_level_ < LOG_LEVEL_FATAL){
			return;
		}else{
			string now;
			get_now(now);
			msg_.append(now);
			msg_.append("@");
			msg_.append("fatal");
			msg_.append("@");
			msg_.append(tag);
			msg_.append("@");
			msg_.append(data.str());

			hdr_->log_out(msg_);
		}
	}

	void log_e(const char* tag, log_data& data){
		spinlock::scoped_lock lock(slock_);

		if(cur_level_ < LOG_LEVEL_ERROR){
			return;
		}else{
			string now;
			get_now(now);
			msg_.append(now);
			msg_.append("@");
			msg_.append("error");
			msg_.append("@");
			msg_.append(tag);
			msg_.append("@");
			msg_.append(data.str());

			hdr_->log_out(msg_);
		}
	}

	void log_i(const char* tag, log_data& data){
		spinlock::scoped_lock lock(slock_);

		if(cur_level_ < LOG_LEVEL_INFO){
			return;
		}else{
			string now;
			get_now(now);
			msg_.append(now);
			msg_.append("@");
			msg_.append("info");
			msg_.append("@");
			msg_.append(tag);
			msg_.append("@");
			msg_.append(data.str());

			hdr_->log_out(msg_);
		}
	}

	void log_d(const char* tag, log_data& data){
		spinlock::scoped_lock lock(slock_);

		if(cur_level_ < LOG_LEVEL_DEBUG){
			return;
		}else{
			string now;
			get_now(now);
			msg_.append(now);
			msg_.append("@");
			msg_.append("debug");
			msg_.append("@");
			msg_.append(tag);
			msg_.append("@");
			msg_.append(data.str());

			hdr_->log_out(msg_);
		}
	}

	// Log Level, LOG_LEVEL_OFF is default.
	enum {
		LOG_LEVEL_OFF 	= 0x00,
		LOG_LEVEL_FATAL = 0x01,
		LOG_LEVEL_ERROR = 0x02,
		LOG_LEVEL_INFO 	= 0x03,
		LOG_LEVEL_DEBUG = 0x04,
		LOG_LEVEL_ALL 	= 0x05,
	};

	// Current log's level, Default is LOG_LEVEL_ALL.
	int level(){
		spinlock::scoped_lock lock(slock_);
		return cur_level_;
	}

	void level(int v){
		spinlock::scoped_lock lock(slock_);
		cur_level_ = v;
	}

private:
	void get_now(string& now){
		ptime p1 = boost::posix_time::microsec_clock::local_time();
		int year = p1.date().year();
		int month = p1.date().month();
		int day = p1.date().day();

		int hour = p1.time_of_day().hours();
		int minute = p1.time_of_day().minutes();
		int second = p1.time_of_day().seconds();

		int millsecond = p1.time_of_day().total_milliseconds() - p1.time_of_day().total_seconds()*1000;

		char ctime[20];
		memset(&ctime[0], 0x00, 20);

		sprintf(&ctime[0], "%4d-%02d-%02d %02d:%02d:%02d.%03d", year, month, day, hour, minute, second, millsecond);
		now.append(ctime);
	}
private:
	HANDLER* hdr_;

	// Log information.
	string msg_;

	spinlock slock_;

	int cur_level_;
};


} }

#endif /* LOG_LOGGER_H_ */
