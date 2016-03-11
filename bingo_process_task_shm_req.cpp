/*
 * bingo_process_task_shm_req.cpp
 *
 *  Created on: 2016-2-1
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

BOOST_AUTO_TEST_SUITE(bingo_process_task_shm_req)



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
typedef shared_memory_req_imp<
			my_parser,		// Parser
			my_message,		// Message
			my_tss_data		// Thread stronge.
		> my_shm;
typedef process_task_req<
			my_shm,
			my_message,
			my_tss_data
		> my_process;

typedef bingo::singleton_v1<my_process, my_process::req_snd_error_callback> my_process_snd;
typedef bingo::singleton_v2<my_process, my_process::req_rev_callback, my_process::req_rev_error_callback> my_process_rev;

// --------------------------- 测试process_task创建成功，删除成功 ------------------------- //

void snd_error(u16& err_code, interprocess_exception& ex){
	cout << "snd_error(), err_code:" << err_code << ",exception:" << ex.what() << endl;
	SHOW_CURRENT_TIME("snd_error");
}

void rev(my_tss_data* tss, char*& data){
	cout << "rev(), data:" << tss->t.stream_to_string(data, my_parser::message_size) << endl;
}

void rev_error(my_tss_data* tss, u16& err_code, interprocess_exception& ex){
	cout << "rev_error(), err_code:" << err_code << ",exception:" << ex.what() << endl;
}


BOOST_AUTO_TEST_CASE(t1_snd){
	my_process_snd::create(snd_error);


	this_thread::sleep(seconds(5));

	my_process_snd::release();
}

BOOST_AUTO_TEST_CASE(t1_rev){
	my_process_rev::create(rev, rev_error);

	this_thread::sleep(seconds(10));

	my_process_rev::release();
}

// --------------------------- 测试process_task创建成功，4个进程发送，1个进程接收数据 ------------------------- //
/*
 * 最好使用1发1收模式，n发1收会出现丢包情况
 */

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
u32 wait_milliseconds = 10;

void rev2(my_tss_data* tss, char*& data){
	cout << "rev(), data:" << tss->t.stream_to_string(data, my_parser::message_size) << endl;

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

}

void run(int n){

	my_process_snd::create(snd_error);

	this_thread::sleep(seconds(2));

	u32 snd_process_total_num = 0;

	u32 max = max_process_run_num, i = 0;
	while(i < max){

		this_thread::sleep(milliseconds(wait_milliseconds));

//		 cout << "beging send:" << i << endl;

		// Send message.
		u16 err_code;
		my_message msg;
		char* p = msg.data.header();
		*p = (u8)n;
		memcpy(p + 1, &i, 4);
		msg.data.change_length(my_parser::message_size, err_code);
		int result = my_process_snd::instance()->put(msg, err_code);
		if(result == 0){
//			cout << "send success! n:" << n << ",i:" << i << endl;
			snd_process_total_num++;
		}else
			cout << "send fail! :n:" << n << ",i:" << i << ",err:" << err_code << endl;
		i++;
	}
	cout << "send success! n:" << n;
	cout << ",snd_process_total_num:" << snd_process_total_num << endl;

	this_thread::sleep(seconds(2));
	my_process_snd::release();
}

void call_proc(int proc_index){

	pid_t pid;
	while(true){
		pid=fork();              					// 父进程返回的pid是大于零的，而创建的子进程返回的pid是等于0的，这个机制正好用来区分父进程和子进程
		if(pid==0)//子进程
		{
			proc_index++;
			cout << "child process start!" << proc_index << endl;
//			 num++;
//			 printf("  %d  \n",num);                //如果没有 \n ，num是输不出来的，为什么？因为printf()会把num放在缓冲区，暂时不输出，
//													//而下一行execlp()系统调用会把子进程的内存清空
//													//装进‘ls’程序，自然的，缓冲区中的num也就没有了。

			if(proc_index == 2) {
				cout << "child proces return!" << proc_index << endl;
				return;
			}
////			cout << "start child process:" << s << ",parament:" << s1 << endl;
//			cout << "start child process:" << s << endl;
////			cout << execl(s.c_str(), lexical_cast<string>(num).c_str(),NULL) << endl;          	//将ls程序装进子进程
//			cout << execlp(s.c_str(), "./test", lexical_cast<string>(num).c_str(), NULL) << endl;
//			cout << errno << endl;

			continue;

//			cout << "child proces end!" << proc_index << endl;
//			 printf("after execlp:  %d  \n",num);	//这一行程序始终得不到执行，为什么？因为上一行的execlp()已经将内存清空，
//													//所以内存中压根就没有这一行代码。
		}
		else if(pid>0)     //父进程
		{


			cout << "return parent process id:" << pid << ",proc_index:" << proc_index << endl;
//			 wait(NULL);   //等待子进程结束
//			 printf("  %d  ",num);
//			 printf("\nchild eomplete");

			run(proc_index);

			wait();    //等待子进程结束

			return;

		}else if(pid == -1){

			cout << "child process create fail!" << endl;
		}
	}
}

BOOST_AUTO_TEST_CASE(t2_snd){
	int n = boost::unit_test::framework::master_test_suite().argc;

	cout << "current proceee's argc:" << n << endl;
	if(n == 2){
		char* c_num = boost::unit_test::framework::master_test_suite().argv[1];
		cout << "current proceee's argv[0]:" << boost::unit_test::framework::master_test_suite().argv[0] << endl;
		cout << "current proceee's argv[1]:" << c_num << endl;

		//Launch child process
		std::string s("./test ");
		int num = lexical_cast<int>(c_num);
//		num++;

		s += lexical_cast<string>(num);
		s += " --build_info=yes --run_test=bingo_process_task_shm_req/t2_snd";

//		std::string s1 = lexical_cast<string>(num);
//		s1 +=" --build_info=yes --run_test=bingo_process_task_shm_req/t2_snd";
//		std::system(s.c_str());

		call_proc(num);

//		sleep(30);

	}
}

BOOST_AUTO_TEST_CASE(t2_rev){

	my_process_rev::create(rev2, rev_error);

	while(max_process_total_num != process_total_num){
		cout << "--------------------------------" << endl;
		cout << "process_total_num:" << process_total_num << endl;

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

	cout << "--------------------------------" << endl;
	cout << "process_total_num:" << process_total_num << endl;

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
	//	process2_num:6000
	//	process3_num:6000
	//	process4_num:6000
	//
	//	process1_total:17997000
	//	process2_total:17997000
	//	process3_total:17997000
	//	process4_total:17997000

}

// --------------------------- 测试process_task创建成功，成功发送和接收数据，发送进程down，重启发送进程，继续成功发送和接受数据 ------------------------- //

BOOST_AUTO_TEST_CASE(t3_snd){

	int n = boost::unit_test::framework::master_test_suite().argc;

	if(n == 2){
		char* c_num = boost::unit_test::framework::master_test_suite().argv[1];


		//Launch child process
		int num = lexical_cast<int>(c_num);

		call_proc(num);

//		std::string s("./test ");
//		int num = lexical_cast<int>(c_num);
//		if(num == 2) return;
//		num++;
//		s += lexical_cast<string>(num);
//
//		s +=" --build_info=yes --run_test=bingo_process_task_shm_req/t3_1";
//
//		int pre = num - 1;
//
//		SHOW_CURRENT_TIME("process " << c_num << " start")
//		run(pre);
//		SHOW_CURRENT_TIME("process " << c_num << " end")
//
//		sleep(10);
//
//		std::system(s.c_str());
	}
}

BOOST_AUTO_TEST_CASE(t3_rev){

	max_process_run_num = 600*10;
	max_process_total_num = max_process_run_num*2;
	process_total_num = 0;

	my_process_rev::create(rev2, rev_error);

	while(true){

		cout << "max_process_total_num:" << max_process_total_num << ",process_total_num:" << process_total_num << endl;

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


// --------------------------- 测试process_task创建成功，成功发送和接收数据，接受进程down，重启接受进程，继续成功发送和接受数据 ------------------------- //

BOOST_AUTO_TEST_CASE(t4_snd){

	int n = boost::unit_test::framework::master_test_suite().argc;

	if(n == 2){
		char* c_num = boost::unit_test::framework::master_test_suite().argv[1];



		//Launch child process
		int num = lexical_cast<int>(c_num);

		call_proc(num);


	}
}

BOOST_AUTO_TEST_CASE(t4_rev){

	max_process_run_num = 600*10;
	max_process_total_num = max_process_run_num*2;
	process_total_num = 0;

	my_process_rev::create(rev2, rev_error);

	while(max_process_total_num != process_total_num){

		cout << "max_process_total_num:" << max_process_total_num << ",process_total_num:" << process_total_num << endl;

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

BOOST_AUTO_TEST_SUITE_END()


