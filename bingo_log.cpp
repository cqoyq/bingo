/*
 * bingo_log.cpp
 *
 *  Created on: 2016-2-19
 *      Author: root
 */

#include <boost/test/unit_test.hpp>

#include <iostream>
using namespace std;

#include "cost_time.h"

#include "bingo/log/all.h"
using namespace bingo;

#include <boost/thread.hpp>
using namespace boost;

BOOST_AUTO_TEST_SUITE(bingo_log)

BOOST_AUTO_TEST_CASE(t_log_data){

	string s1("this is a test");
	char c1 = 0xE1;
	u8 c = 0xE1;
	u16 n = 123;
	u32 n1 = 4567;
	u64 n2 = 9090;
	void* p = &s1;

	log::log_data data;
	data + "abcd"
	     + "," + string("1234 89")
	     + "," + s1

	     + "," + 0xEE	// 238
	     + "," + c		// 225
	     + "," + c1		// 225

	     + "," + n		// 123
	     + "," + n1		// 123
	     + "," + n2		// 123

	     + "," + this_thread::get_id()
		 + "," + p;

	cout << data.str() << endl;

	// output:
	//	abcd,1234 89,this is a test,238,225,225,123,4567,9090,5e43100,0x7feffe250


}

struct file_info{
	static const char* directory;
	static const char* filename;
};
const char* file_info::directory = ".";
const char* file_info::filename = "log";

BOOST_AUTO_TEST_CASE(t_logger){

	typedef log::logger<log::file_handler<file_info> > mylog;

	mylog log;
	log.level(mylog::LOG_LEVEL_FATAL);

	log.log_f("bad", log::log_data() + "abcd" + "," + "1234");
}

BOOST_AUTO_TEST_SUITE_END()


