/*
 * bingo_process_task_ask.cpp
 *
 *  Created on: 2016-3-9
 *      Author: root
 */


#include <boost/test/unit_test.hpp>

#include <stdio.h> //gets;

#include <cstring>
#include <cstdlib>
#include <string>
#include <iostream>
using namespace std;

#include "cost_time.h"

#define BINGO_PROCESS_TASK_SHARED_MEMORY_DEBUG
#include "bingo/xprocess/all.h"
using namespace bingo;
using namespace bingo::xprocess::task;

#include "boost/thread.hpp"
using namespace boost;

BOOST_AUTO_TEST_SUITE(bingo_process_task_ask)

struct my_tss_data : public thread_tss_data{
	bingo::stringt t;
};

struct my_parser{

	static const char* shared_memory_name;		// Shared memory name
	static const u32 message_size;				// Size of block
};

const char* my_parser::shared_memory_name = "myshm";
const u32 my_parser::message_size = 1024;

typedef task_message_data<my_parser> my_message;
typedef shared_memory_ask_and_answer_imp<
			my_parser,		// Parser
			my_message,		// Message
			my_tss_data		// Thread stronge.
		> my_shm;

u32 process1_num = 0;
u32 process2_num = 0;
u32 process3_num = 0;
u32 process4_num = 0;

u32 process1_total = 0;
u32 process2_total = 0;
u32 process3_total = 0;
u32 process4_total = 0;

u32 max_process_run_num = 600*10;
u32 max_process_total_num = max_process_run_num;
u32 process_total_num = 0;
u32 process_rep_total_num = 0;
u32 wait_milliseconds = 10;

void req_rev(my_tss_data* tss, char*& data, my_message& rep_data){
	cout << "thr:" << this_thread::get_id() << ",req_rev(), data:" << tss->t.stream_to_string(data, my_parser::message_size) << endl;

//	u32 value = 0;
//	memcpy(&value, data + 1, 4);
//	if(data[0] == 1){
//		process1_num++;
//		process1_total += value;
//	}
//	else if(data[0] == 2){
//		process2_num++;
//		process2_total += value;
//	}
//	else if(data[0] == 3){
//		process3_num++;
//		process3_total += value;
//	}
//	else if(data[0] == 4){
//		process4_num++;
//		process4_total += value;
//	}

	process_total_num++;

	u16 err_code = 0;
	BOOST_CHECK_EQUAL(rep_data.data.copy(data, my_parser::message_size, err_code), 0);

}
void req_rev_error(my_tss_data* tss, u16& err_code, interprocess_exception& ex){
	cout << "req_rev_error(), err_code:" << err_code << ",exception:" << ex.what() << endl;
}
void rep_snd_error(u16& err_code, interprocess_exception& ex){
	cout << "rep_snd_error(), err_code:" << err_code << ",exception:" << ex.what() << endl;
}



void rep_rev(my_message& rep_data){
	stringt t;
	cout << "thr:" << this_thread::get_id() <<
			",rep_rev(), data:" << t.stream_to_char(rep_data.data.header(), rep_data.data.length()) << endl;
	process_rep_total_num++;
}
void rep_rev_error(my_tss_data* tss, u16& err_code, interprocess_exception& ex){
	cout << "rep_rev_error(), err_code:" << err_code << ",exception:" << ex.what() << endl;
}
void req_snd_error(u16& err_code, interprocess_exception& ex){
	cout << "req_snd_error(), err_code:" << err_code << ",exception:" << ex.what() << endl;
}


void req_snd(my_message& req_data){

	cout << "please input:";

	size_t str_size = req_data.data.size();
	char str[str_size];
	memset(str, 0x00, str_size);
	gets(str);

	cout << "in_size:" << strlen(str) << endl;
	u16 err_code = 0;
	req_data.data.copy(str, strlen(str), err_code);

	cout << "thr:" << this_thread::get_id() << ",make send data success! data:" << str << endl;
}

// --------------------------- 测试process_task创建成功，删除成功 ------------------------- //

void req_snd_with_no_data(my_message& req_data){
	// If req_data hasn't no data that it don't call send function.
}

BOOST_AUTO_TEST_CASE(t_snd1){
	my_shm my_process_snd(req_snd_error, req_snd_with_no_data, rep_rev, rep_rev_error);
	this_thread::sleep(seconds(5));
	my_process_snd.run();

}

BOOST_AUTO_TEST_CASE(t_rev1){

	my_shm my_process_rev(req_rev, req_rev_error, rep_snd_error);
	this_thread::sleep(seconds(5));
}

// --------------------------- 测试process_task创建成功，1个进程发送，1个进程接收数据 ------------------------- //

BOOST_AUTO_TEST_CASE(t_snd2){

	my_shm my_process_snd(req_snd_error, req_snd, rep_rev, rep_rev_error);
	this_thread::sleep(seconds(5));
	my_process_snd.run();

	boost::condition_variable cond;
	boost::mutex mut;

	{
		boost::unique_lock<boost::mutex> lock(mut);
		cond.wait(lock);
	}

}

BOOST_AUTO_TEST_CASE(t_rev2){

	my_shm my_process_rev(req_rev, req_rev_error, rep_snd_error);

	while(true){
		this_thread::sleep(seconds(30));
	}
}

BOOST_AUTO_TEST_SUITE_END()
