/*
 * bingo_process_task_shm_req_and_rep.cpp
 *
 *  Created on: 2016-2-3
 *      Author: root
 */

#include <boost/test/unit_test.hpp>

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
using bingo::xprocess::task::task_message_data;

#include <boost/interprocess/sync/interprocess_mutex.hpp>
using namespace boost::interprocess;

BOOST_AUTO_TEST_SUITE(bingo_process_task_shm_req_and_rep)

struct my_tss_data : public thread_tss_data{
	bingo::stringt t;
};

struct my_parser{

	static const char* shared_memory_name;		// Shared memory name
	static const u32 message_size;					// Size of block
};

const char* my_parser::shared_memory_name = "myshm";
const u32 my_parser::message_size = 12;

typedef task_message_data<my_parser> my_message;
typedef shared_memory_req_and_rep_imp<
			my_parser,		// Parser
			my_message,		// Message
			my_tss_data		// Thread stronge.
		> my_shm;
typedef process_task_req_and_rep<
			my_shm,
			my_message,
			my_tss_data
		> my_process;

typedef bingo::singleton_v3<my_process,
		my_process::req_snd_error_callback,
		my_process::rep_rev_callback,
		my_process::rep_rev_error_callback
		> my_process_snd;
typedef bingo::singleton_v3<my_process,
		my_process::req_rev_callback,
		my_process::req_rev_error_callback,
		my_process::rep_snd_error_callback
		> my_process_rev;

// --------------------------- 测试process_task创建成功，删除成功 ------------------------- //

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
	cout << "req_rev(), data:" << tss->t.stream_to_string(data, my_parser::message_size) << endl;

	u32 value = 0;
	memcpy(&value, data + 1, 4);
	if(data[0] == 1){
		process1_num++;
		process1_total += value;
	}
	else if(data[0] == 2){
		process2_num++;
		process2_total += value;
	}
	else if(data[0] == 3){
		process3_num++;
		process3_total += value;
	}
	else if(data[0] == 4){
		process4_num++;
		process4_total += value;
	}

	process_total_num++;

	u16 err_code = 0;

	u32 d = 19771019;
	char* p = (char*)&d;
	BOOST_CHECK_EQUAL(rep_data.data.copy(p, 4, err_code), 0);

}
void req_rev_error(my_tss_data* tss, u16& err_code, interprocess_exception& ex){
	cout << "req_rev_error(), err_code:" << err_code << ",exception:" << ex.what() << endl;
}
void rep_snd_error(u16& err_code, interprocess_exception& ex){
	cout << "rep_snd_error(), err_code:" << err_code << ",exception:" << ex.what() << endl;
}



void rep_rev(my_tss_data* tss, char*& data){
	cout << "rep_rev(), data:" << tss->t.stream_to_string(data, my_parser::message_size) << endl;
	process_rep_total_num++;
}
void rep_rev_error(my_tss_data* tss, u16& err_code, interprocess_exception& ex){
	cout << "rep_rev_error(), err_code:" << err_code << ",exception:" << ex.what() << endl;
}
void req_snd_error(u16& err_code, interprocess_exception& ex){
	cout << "req_snd_error(), err_code:" << err_code << ",exception:" << ex.what() << endl;
}

BOOST_AUTO_TEST_CASE(t1_snd){
	my_process_snd::create(req_snd_error, rep_rev, rep_rev_error);

	this_thread::sleep(seconds(5));

	my_process_snd::release();
}

BOOST_AUTO_TEST_CASE(t1_rev){
	my_process_rev::create(req_rev, req_rev_error, rep_snd_error);

	this_thread::sleep(seconds(20));

	my_process_rev::release();
}

// --------------------------- 测试process_task创建成功，1个进程发送，1个进程接收数据 ------------------------- //

void run(int n){

	my_process_snd::create(req_snd_error, rep_rev, rep_rev_error);

	this_thread::sleep(seconds(2));

	u32 max = max_process_run_num, i = 0;
	while(i < max){

		this_thread::sleep(milliseconds(wait_milliseconds));

		// Send message.
		u16 err_code;
		my_message msg;
		char* p = msg.data.header();
		*p = (u8)n;
		memcpy(p + 1, &i, 4);
		msg.data.change_length(my_parser::message_size, err_code);
		BOOST_CHECK(my_process_snd::instance()->put(msg, err_code)==0);
		cout << "send success! n:" << n << ",i:" << i << endl;
		i++;
	}

	this_thread::sleep(seconds(2));
	my_process_snd::release();
}

BOOST_AUTO_TEST_CASE(t2_snd){
	int n = boost::unit_test::framework::master_test_suite().argc;

	if(n == 2){
		char* c_num = boost::unit_test::framework::master_test_suite().argv[1];

		//Launch child process
		std::string s("./test ");
		int num = lexical_cast<int>(c_num);
		if(num == 2) return;
		num++;
		s += lexical_cast<string>(num);

		s +=" --build_info=yes --run_test=bingo_process_task_shm_req_and_rep/t2_1";
		std::system(s.c_str());

		int pre = num - 1;
		run(pre);
//		sleep(30);

		cout << pre << " process_rep_total_num:" << process_rep_total_num << endl;
	}
}

BOOST_AUTO_TEST_CASE(t2_rev){
	my_process_rev::create(req_rev, req_rev_error, rep_snd_error);

	while(max_process_total_num != process_total_num){

		cout << "------------------------" << endl;
		cout << "max_process_total_num:" << max_process_total_num <<
						",process_total_num:" << process_total_num << endl;

		cout << "process1_num:" << process1_num << endl;
		cout << "process2_num:" << process2_num << endl;
		cout << "process3_num:" << process3_num << endl;
		cout << "process4_num:" << process4_num << endl;
		cout << endl;

		cout << "process1_total:" << process1_total << endl;
		cout << "process2_total:" << process2_total << endl;
		cout << "process3_total:" << process3_total << endl;
		cout << "process4_total:" << process4_total << endl;
		cout << endl;

		this_thread::sleep(seconds(30));
	}

	cout << "------------------------" << endl;
	cout << "max_process_total_num:" << max_process_total_num <<
					",process_total_num:" << process_total_num << endl;

	cout << "process1_num:" << process1_num << endl;
	cout << "process2_num:" << process2_num << endl;
	cout << "process3_num:" << process3_num << endl;
	cout << "process4_num:" << process4_num << endl;
	cout << endl;

	cout << "process1_total:" << process1_total << endl;
	cout << "process2_total:" << process2_total << endl;
	cout << "process3_total:" << process3_total << endl;
	cout << "process4_total:" << process4_total << endl;
	cout << endl;



	my_process_rev::release();

	// output:
	//	process1_num:6000
	//	process2_num:0
	//	process3_num:0
	//	process4_num:0
	//
	//	process1_total:17997000
	//	process2_total:0
	//	process3_total:0
	//	process4_total:0

}

// --------------------------- 测试process_task创建成功，成功发送和接收数据，发送进程down，重启发送进程，继续成功发送和接受数据 ------------------------- //
// --------------------------- 测试process_task创建成功，成功发送和接收数据，接受进程down，重启接受进程，继续成功发送和接受数据 ------------------------- //

BOOST_AUTO_TEST_CASE(t3_snd){

	max_process_run_num = 600*10*4;

	int n = boost::unit_test::framework::master_test_suite().argc;

	if(n == 2){
		char* c_num = boost::unit_test::framework::master_test_suite().argv[1];

		//Launch child process
		std::string s("./test ");
		int num = lexical_cast<int>(c_num);
		if(num == 2) return;
		num++;
		s += lexical_cast<string>(num);

		s +=" --build_info=yes --run_test=bingo_process_task_shm_req_and_rep/t3_1";
		std::system(s.c_str());

		int pre = num - 1;
		run(pre);
//		sleep(30);

		cout << "process_rep_total_num:" << process_rep_total_num << endl;
	}
}

BOOST_AUTO_TEST_CASE(t3_rev){
	my_process_rev::create(req_rev, req_rev_error, rep_snd_error);

	while(true){

		cout << "------------------------" << endl;
		cout << "max_process_total_num:" << max_process_total_num <<
				",process_total_num:" << process_total_num << endl;

		cout << "process1_num:" << process1_num << endl;
		cout << "process2_num:" << process2_num << endl;
		cout << "process3_num:" << process3_num << endl;
		cout << "process4_num:" << process4_num << endl;
		cout << endl;

		cout << "process1_total:" << process1_total << endl;
		cout << "process2_total:" << process2_total << endl;
		cout << "process3_total:" << process3_total << endl;
		cout << "process4_total:" << process4_total << endl;
		cout << endl;

		this_thread::sleep(seconds(30));
	}



	my_process_rev::release();
}

// --------------------------- 测试process_task创建成功，n个进程发送，1个进程接收数据 ------------------------- //
/*
 * 测试结果：n进程发送，1进程接受时，数据接受缺失
 */

BOOST_AUTO_TEST_SUITE_END()


