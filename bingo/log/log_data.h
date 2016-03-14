/*
 * log_data.h
 *
 *  Created on: 2016-2-19
 *      Author: root
 */

#ifndef LOG_LOG_DATA_H_
#define LOG_LOG_DATA_H_

namespace bingo { namespace log {

class log_data {
public:
	log_data(){}
	virtual ~log_data(){}

	const char* str(){
		return str_.c_str();
	}

	template<typename T>
	log_data& operator+(const T& obj){
		str_.append(t_.convert(obj));
		return (*this);
	}

	log_data& operator+(const char* obj){
		str_.append(t_.convert(obj));
		return (*this);
	}

	log_data& operator+(string obj){
		str_.append(obj.c_str());
		return (*this);
	}

private:
	string str_;
	bingo::stringt t_;
};

} }

#endif /* LOG_LOG_DATA_H_ */
