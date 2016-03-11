/*
 * bingo_thread_task.cpp
 *
 *  Created on: 2016-1-25
 *      Author: root
 */


#include <boost/test/unit_test.hpp>

#include <iostream>
using namespace std;

#include "cost_time.h"

#define BINGO_THREAD_TASK_MUTEX_IMP_DEBUG
//#define BINGO_THREAD_TASK_ATOMIC_IMP_DEBUG
#include "bingo/xthread/all.h"
using namespace bingo;
using namespace bingo::xthread::task;
using bingo::xthread::spinlock;

BOOST_AUTO_TEST_SUITE(bingo_thread_task)

struct my_tss_data_one_to_one : public thread_tss_data{
	bingo::stringt t;
};

struct parser_one_to_one{

	// task_message_data use
	static const int message_size;

	// atomic_imp_one_to_one use

};
const int parser_one_to_one::message_size = 256;


typedef task_message_data<parser_one_to_one> my_data_message;
typedef task_exit_data<parser_one_to_one>    my_exit_message;

typedef thread_task<
		mutex_imp_one_to_one<my_data_message,
							 my_exit_message,
							 my_tss_data_one_to_one>,
//		atomic_imp_one_to_one<my_data_message,
//							  my_exit_message,
//							  my_tss_data_one_to_one>,
		my_data_message,
		my_tss_data_one_to_one
	> my_task_type_thread;
typedef bingo::singleton_v1<my_task_type_thread, my_task_type_thread::thr_top_callback> my_task;



void top(my_tss_data_one_to_one* tss, my_data_message*& data){
}

// --------------------------- 测试thread_task创建成功 ------------------------- //

BOOST_AUTO_TEST_CASE(t1){

	my_task::create(
			top					// thread_task queue top callback
			);

	this_thread::sleep(seconds(30));

	my_task::release();

	// output:
	//	wait for thread exit,  ~mutex_imp_one_to_one()
	//	received task_data_type_exit_thread
	//	is_thread_exit_ is true in is_empty()
	//	thread exit,  ~mutex_imp_one_to_one()

}

// --------------------------- 测试thread_task分配器队列空，分配时返回错误信息 ------------------------- //

BOOST_AUTO_TEST_CASE(t2){

//	my_allocation::create(2u);
//
//	my_data_message* out = 0;
//	u16 err_code = 0;
//	BOOST_CHECK_EQUAL(my_allocation::instance()->top(out, err_code), 0);
//	cout << out << endl;
//	BOOST_CHECK_EQUAL(my_allocation::instance()->top(out, err_code), 0);
//	cout << out << endl;
//	BOOST_CHECK_EQUAL(my_allocation::instance()->top(out, err_code), -1);				// allocation queue has empty.
//	BOOST_CHECK(err_code == error_thread_task_allocation_queue_is_empty);
//
//	my_allocation::release();
}

// --------------------------- 测试thread_task消息队列满，入队时返回错误信息 ------------------------- //

void top3(my_tss_data_one_to_one* tss, my_data_message*& msg){

	SHOW_CURRENT_TIME("top data:" << tss->t.stream_to_string(&msg->data.this_object()->message[0], 4))

	this_thread::sleep(seconds(10));
}

void run_input3(){

	for (int i = 0; i < 5; ++i) {

		this_thread::sleep(seconds(2));

		my_data_message* msg = new my_data_message();
		u16 err_code = 0;

		msg->data.this_object()->message[0] = (u8)i;

		if(my_task::instance()->put(msg, err_code) == -1){			// Input T into queue

			bingo::stringt t;
			SHOW_CURRENT_TIME("put error, err_code:" << err_code << ",data:" << t.stream_to_string(&msg->data.this_object()->message[0], 4))
			BOOST_CHECK(err_code == error_thread_task_queue_is_full);

			delete msg;

		}else{

			bingo::stringt t;
			SHOW_CURRENT_TIME("put success, data:" << t.stream_to_string(&msg->data.this_object()->message[0], 4))
		}
	}
}

BOOST_AUTO_TEST_CASE(t3){

//	parser_one_to_one::queue_size = 2;		// size of thread_task queue is 2

	my_task::create(
			top3							// thread_task queue top callback
			);

	thread t1(run_input3);
	t1.join();

	this_thread::sleep(seconds(40));

	my_task::release();

	// output:
	//	put success, data:00 00 00 00  time:2016-01-26T14:10:35
	//	top data:00 00 00 00  time:2016-01-26T14:10:35
	//	put success, data:01 00 00 00  time:2016-01-26T14:10:37
	//	put success, data:02 00 00 00  time:2016-01-26T14:10:39
	//	put error, err_code:272,data:03 00 00 00  time:2016-01-26T14:10:41			// Return error_thread_task_queue_is_full when queue is full.
	//	put error, err_code:272,data:04 00 00 00  time:2016-01-26T14:10:43
	//	top data:01 00 00 00  time:2016-01-26T14:10:45
	//	top data:02 00 00 00  time:2016-01-26T14:10:55
}

// --------------------------- 测试thread_task线程退出，入队时返回错误信息 ------------------------- //

void top4(my_tss_data_one_to_one* tss, my_data_message*& msg){

	SHOW_CURRENT_TIME("top data:" << tss->t.stream_to_string(&msg->data.this_object()->message[0], 4))

}

void run_input4(){

	for (int i = 0; i < 5; ++i) {

		this_thread::sleep(seconds(2));

		my_data_message* msg = new my_data_message();
		u16 err_code = 0;


		if(i == 1)
			msg->type = task_data_type_exit_thread;

		msg->data.this_object()->message[0] = (u8)i;

		if(my_task::instance()->put(msg, err_code) == -1){			// Input T into queue

			bingo::stringt t;
			SHOW_CURRENT_TIME("put error, err_code:" << err_code << ",data:" << t.stream_to_string(&msg->data.this_object()->message[0], 4))
			BOOST_CHECK(err_code == error_thread_task_svc_thread_exit);

			delete msg;

		}else{

			bingo::stringt t;
			SHOW_CURRENT_TIME("put success, data:" << t.stream_to_string(&msg->data.this_object()->message[0], 4))
		}
	}
}

BOOST_AUTO_TEST_CASE(t4){

//	parser_one_to_one::queue_size = 2;	// size of thread_task queue is 2

	my_task::create(
			top4							// thread_task queue top callback
			);

	thread t1(run_input4);
	t1.join();

	this_thread::sleep(seconds(10));

	my_task::release();

	// output:
	//	put success, data:00 00 00 00  time:2016-01-26T14:23:30
	//	top data:00 00 00 00  time:2016-01-26T14:23:30
	//	put success, data:01 00 00 00  time:2016-01-26T14:23:32
	//	received task_data_type_exit_thread
	//	is_thread_exit_ is true in is_empty()
	//	put error, err_code:273,data:02 00 00 00  time:2016-01-26T14:23:34		// Return error_thread_task_svc_thread_exit when queue thread has exit.
	//	put error, err_code:273,data:03 00 00 00  time:2016-01-26T14:23:36
	//	put error, err_code:273,data:04 00 00 00  time:2016-01-26T14:23:38
}

// --------------------------- 测试thread_task多线程入队、出对传输正确 ------------------------- //

void top5(my_tss_data_one_to_one* tss, my_data_message*& msg){

	SHOW_CURRENT_TIME("top data:" << tss->t.stream_to_string(&msg->data.this_object()->message[0], 4))
}

void run_input5(int& n){

	for (int i = 0; i < 5; ++i) {

		my_data_message* msg = new my_data_message();
		u16 err_code = 0;

		msg->data.this_object()->message[0] = (u8)n;
		msg->data.this_object()->message[1] = (u8)i;

		if(my_task::instance()->put(msg, err_code) == -1){			// Input T into queue

			bingo::stringt t;
			SHOW_CURRENT_TIME("put error, err_code:" << err_code << ",data:" << t.stream_to_string(&msg->data.this_object()->message[0], 4))
			BOOST_CHECK(err_code == error_thread_task_svc_thread_exit);

			delete msg;

		}else{

			bingo::stringt t;
			SHOW_CURRENT_TIME("put success, data:" << t.stream_to_string(&msg->data.this_object()->message[0], 4))
		}
	}
}

BOOST_AUTO_TEST_CASE(t5){

//	parser_one_to_one::queue_size = 30;	// size of thread_task queue is 2

	my_task::create(
			top5							// thread_task queue top callback
			);

	thread t1(run_input5, 1);
	thread t2(run_input5, 2);
	thread t3(run_input5, 3);
	thread t4(run_input5, 4);
	t1.join();
	t2.join();
	t3.join();
	t4.join();

	this_thread::sleep(seconds(2));

	my_task::release();

	// output:
	//	put success, data:04 00 00 00  time:2016-01-26T14:30:04
	//	put success, data:04 01 00 00  time:2016-01-26T14:30:04
	//	put success, data:04 02 00 00  time:2016-01-26T14:30:04
	//	put success, data:04 03 00 00  time:2016-01-26T14:30:04
	//	put success, data:04 04 00 00  time:2016-01-26T14:30:04
	//	put success, data:02 00 00 00  time:2016-01-26T14:30:04
	//	put success, data:02 01 00 00  time:2016-01-26T14:30:04
	//	put success, data:02 02 00 00  time:2016-01-26T14:30:04
	//	put success, data:02 03 00 00  time:2016-01-26T14:30:04
	//	put success, data:02 04 00 00  time:2016-01-26T14:30:04
	//	top data:04 00 00 00  time:2016-01-26T14:30:04
	//	top data:01 00 00 00  time:2016-01-26T14:30:04
	//	put success, data:01 00 00 00  time:2016-01-26T14:30:04
	//	put success, data:01 01 00 00  time:2016-01-26T14:30:04
	//	put success, data:01 02 00 00  time:2016-01-26T14:30:04
	//	put success, data:01 03 00 00  time:2016-01-26T14:30:04
	//	put success, data:01 04 00 00  time:2016-01-26T14:30:04
	//	top data:04 01 00 00  time:2016-01-26T14:30:04
	//	top data:04 02 00 00  time:2016-01-26T14:30:04
	//	top data:04 03 00 00  time:2016-01-26T14:30:04
	//	top data:04 04 00 00  time:2016-01-26T14:30:04
	//	top data:02 00 00 00  time:2016-01-26T14:30:04
	//	top data:02 01 00 00  time:2016-01-26T14:30:04
	//	top data:02 02 00 00  time:2016-01-26T14:30:04
	//	top data:02 03 00 00  time:2016-01-26T14:30:04
	//	top data:02 04 00 00  time:2016-01-26T14:30:04
	//	top data:03 00 00 00  time:2016-01-26T14:30:04
	//	top data:01 01 00 00  time:2016-01-26T14:30:04
	//	top data:01 02 00 00  time:2016-01-26T14:30:04
	//	top data:01 03 00 00  time:2016-01-26T14:30:04
	//	top data:01 04 00 00  time:2016-01-26T14:30:04
	//	put success, data:03 00 00 00  time:2016-01-26T14:30:04
	//	put success, data:03 01 00 00  time:2016-01-26T14:30:04
	//	put success, data:03 02 00 00  time:2016-01-26T14:30:04
	//	put success, data:03 03 00 00  time:2016-01-26T14:30:04
	//	put success, data:03 04 00 00  time:2016-01-26T14:30:04
	//	top data:03 01 00 00  time:2016-01-26T14:30:04
	//	top data:03 02 00 00  time:2016-01-26T14:30:04
	//	top data:03 03 00 00  time:2016-01-26T14:30:04
	//	top data:03 04 00 00  time:2016-01-26T14:30:04
}

// --------------------------- 测试thread_task性能，入队和出队花费时间 ------------------------- //

u32 thr1_num = 0;
u32 thr1_num_add = 0;
u32 thr2_num = 0;
u32 thr2_num_add = 0;
u32 thr3_num = 0;
u32 thr3_num_add = 0;
u32 thr4_num = 0;
u32 thr4_num_add = 0;
u32 thr_total = 0;

void top6(my_tss_data_one_to_one* tss, my_data_message*& msg){

	if(msg->data.this_object()->message[0] == 0x01){
		thr1_num++;
	}else if(msg->data.this_object()->message[0] == 0x02){
		thr2_num++;
	}else if(msg->data.this_object()->message[0] == 0x03){
		thr3_num++;
	}else if(msg->data.this_object()->message[0] == 0x04){
		thr4_num++;
	}

	thr_total++;
	if(thr_total == 600 * 300 * 4)
		SHOW_CURRENT_TIME("end")

//	bingo::stringt t;
//		SHOW_CURRENT_TIME("top data:" << t.stream_to_string(&data->message[0], 4))

//	my_task::instance()->alloc_push(data);			// Push T into allocation
}

void run_input6(int& n){

	u32 max = 600 * 300;
	for (u32 i = 0; i < max; ++i) {

		this_thread::sleep(milliseconds(10));

		u16 err_code = 0;
		my_data_message* msg = new my_data_message();
		memset(&msg->data.this_object()->message[0], 0x00, parser_one_to_one::message_size);

		msg->data.this_object()->message[0] = (u8)n;
		memcpy(&msg->data.this_object()->message[0] + 1, &i, 2);

		if(my_task::instance()->put(msg, err_code) == -1){			// Input T into queue

			bingo::stringt t;
			SHOW_CURRENT_TIME("put error, err_code:" << err_code << ",data:" << t.stream_to_string(&msg->data.this_object()->message[0], 4))
			BOOST_CHECK(err_code == error_thread_task_svc_thread_exit);

			delete msg;

//			my_task::instance()->alloc_push(data);					// Push T into allocation
		}else{

//			bingo::stringt t;
//			SHOW_CURRENT_TIME("put success, data:" << t.stream_to_string(&data->message[0], 4))
		}
	}
}

BOOST_AUTO_TEST_CASE(t6){

//	parser_one_to_one::queue_size = 30;	// size of thread_task queue is 2


	my_task::create(

			top6							// thread_task queue top callback
			);

	SHOW_CURRENT_TIME("start")
	thread t1(run_input6, 1);
	thread t2(run_input6, 2);
	thread t3(run_input6, 3);
	thread t4(run_input6, 4);
	t1.join();
	t2.join();
	t3.join();
	t4.join();

	this_thread::sleep(seconds(2));

	cout << "thr1_num:" << thr1_num << endl;
	cout << "thr2_num:" << thr2_num << endl;
	cout << "thr3_num:" << thr3_num << endl;
	cout << "thr4_num:" << thr4_num << endl;


	my_task::release();

	// output:
	// 	mutex_imp_one_to_one
	//	start time:2016-01-26T17:57:44
	//	end time:2016-01-26T17:57:58

	//  atomic_imp_one_to_one
	//	start time:2016-01-26T17:58:51
	//	end time:2016-01-26T17:59:03

}

// --------------------------- 测试thread_task多线程出队 ------------------------- //

struct my_tss_data_one_to_many : public thread_tss_data{
	bingo::stringt t;
	int flag;
};

struct parser_one_to_many{

	// mutex_imp_one_to_many and atomic_imp_one_to_many use
	static const int thread_n;
};
const int parser_one_to_many::thread_n = 3;

typedef thread_task<
		mutex_imp_one_to_many<my_data_message,
							  my_exit_message,
							  parser_one_to_many,
							  my_tss_data_one_to_many>,
//		atomic_imp_one_to_many<my_data_message,
//							  my_exit_message,
//							  parser_one_to_many,
//							  my_tss_data_one_to_many>,
		my_data_message,
		my_tss_data_one_to_many
	> my_task_type_nthread;
typedef bingo::singleton_v1<my_task_type_nthread, my_task_type_nthread::thr_top_callback> my_task_nthread;

std::map<string, u32> thr_map;
spinlock mu;
void insert_thr_map(my_data_message*& msg, string& id){
	spinlock::scoped_lock lock(mu);

	std::map<string, u32>::iterator iter = thr_map.find(id);
	if(iter == thr_map.end()){
		thr_map.insert(make_pair(id, 1u));
	}else{
		((*iter).second)++;
	}

	u16 value = 0;
	memcpy(&value, &msg->data.this_object()->message[0] + 1, 2);
	if(msg->data.this_object()->message[0] == 0x01){
		thr1_num++;
		thr1_num_add += value;
	}else if(msg->data.this_object()->message[0] == 0x02){
		thr2_num++;
		thr2_num_add += value;
	}else if(msg->data.this_object()->message[0] == 0x03){
		thr3_num++;
		thr3_num_add += value;
	}else if(msg->data.this_object()->message[0] == 0x04){
		thr4_num++;
		thr4_num_add += value;
	}

	thr_total++;

//	bingo::stringt t;
//	SHOW_CURRENT_TIME("top data:" << t.stream_to_string(&data->message[0], 4))
}

void top21(my_tss_data_one_to_many* tss, my_data_message*& msg){

	std::stringstream stream;
	stream << this_thread::get_id();
	string id = stream.str();

	insert_thr_map(msg, id);

	SHOW_CURRENT_TIME("top data:" << tss->t.stream_to_string(&msg->data.this_object()->message[0], 4) << ",tss:" << tss)

//	my_task::instance()->alloc_push(data);			// Push T into allocation
}

void run_input21(int& n){

//	u32 max = 600 * 300;
	u32 max = 600;
	for (u32 i = 0; i < max; ++i) {

		this_thread::sleep(milliseconds(10));

//		my_data_message* data = 0;
//		u16 err_code = 0;
//		BOOST_CHECK_EQUAL(my_task::instance()->alloc_top(data, err_code), 0);	// Top T from allocation

		u16 err_code = 0;
		my_data_message* msg = new my_data_message();
		memset(&msg->data.this_object()->message[0], 0x00, parser_one_to_one::message_size);

		msg->data.this_object()->message[0] = (u8)n;
		memcpy(&msg->data.this_object()->message[0] + 1, &i, 2);

		if(my_task_nthread::instance()->put(msg, err_code) == -1){			// Input T into queue

			bingo::stringt t;
			SHOW_CURRENT_TIME("put error, err_code:" << err_code << ",data:" << t.stream_to_string(&msg->data.this_object()->message[0], 4))
			BOOST_CHECK(err_code == error_thread_task_svc_thread_exit);

			delete msg;

//			my_task::instance()->alloc_push(data);					// Push T into allocation
		}else{

//			bingo::stringt t;
//			SHOW_CURRENT_TIME("put success, data:" << t.stream_to_string(&data->message[0], 4))
		}
	}
}

BOOST_AUTO_TEST_CASE(t21){

	my_task_nthread::create(
			top21							// thread_task queue top callback
			);

	this_thread::sleep(seconds(2));

	thread t1(run_input21, 1);
	thread t2(run_input21, 2);
	thread t3(run_input21, 3);
	thread t4(run_input21, 4);
	t1.join();
	t2.join();
	t3.join();
	t4.join();

	this_thread::sleep(seconds(2));

	typedef std::map<string, u32>::value_type value_type;
	foreach_(value_type& n, thr_map){
		cout << "thr_key:" << n.first << ",counter:" << n.second << endl;
	}
	cout << endl;

	cout << "thr_total:" << (u32)thr_total << endl;
	cout << endl;

	cout << "thr1_num:" << thr1_num << endl;
	cout << "thr2_num:" << thr2_num << endl;
	cout << "thr3_num:" << thr3_num << endl;
	cout << "thr4_num:" << thr4_num << endl;
	cout << endl;

	cout << "thr1_num_add:" << thr1_num_add << endl;
	cout << "thr2_num_add:" << thr2_num_add << endl;
	cout << "thr3_num_add:" << thr3_num_add << endl;
	cout << "thr4_num_add:" << thr4_num_add << endl;

	my_task_nthread::release();

	// output:
	//	thr:7911700,create success!
	//	thr:6f10700,create success!
	//	thr:650f700,create success!
	//
	//	thr_key:650f700,counter:769
	//	thr_key:6f10700,counter:771
	//	thr_key:7911700,counter:860
	//
	//	thr_total:2400
	//
	//	thr1_num:600
	//	thr2_num:600
	//	thr3_num:600
	//	thr4_num:600
	//
	//	thr1_num_add:179700
	//	thr2_num_add:179700
	//	thr3_num_add:179700
	//	thr4_num_add:179700

}

BOOST_AUTO_TEST_SUITE_END()

