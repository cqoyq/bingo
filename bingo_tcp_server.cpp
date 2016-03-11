/*
 * boost_asio_tcp_server.cpp
 *
 *  Created on: 2016-1-28
 *      Author: root
 */

#include <boost/test/unit_test.hpp>

#include <iostream>
using namespace std;

#include "cost_time.h"

#include <boost/foreach.hpp>
#define foreach_         BOOST_FOREACH
#define foreach_r_       BOOST_REVERSE_FOREACH

#include <boost/range/algorithm.hpp>
#include <boost/make_shared.hpp>


//#define BOOST_ASIO_ENABLE_HANDLER_TRACKING			// Open handler tracking


#define BINGO_TCP_SERVER_DEBUG
#define BINGO_TCP_HANDLER_MANAGER_DEBUG
#define BINGO_TCP_ATOMIC_VERSION
#include "bingo/internet/tcp/all.h"
using namespace bingo;
using namespace bingo::internet::tcp;

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::asio::ip;



BOOST_AUTO_TEST_SUITE(bingo_tcp_server)

#pragma pack(1)
struct my_package{
	char data[256];
};
#pragma pack()

// ------------------------------------------------ tcp server ------------------------------------ //

struct my_tss_data : public thread_tss_data{
	bingo::stringt t;
};

struct my_parse{
	static const size_t header_size;										// Parse size of package's header.b
	static int max_wait_for_heartjump_seconds;						// If the value is 0, then server don't check heartjump.
	static int max_wait_for_authentication_pass_seconds;			// If the value is 0, then server don't check authentication pass.


};
const size_t my_parse::header_size = 4;
int my_parse::max_wait_for_heartjump_seconds = 0;
int my_parse::max_wait_for_authentication_pass_seconds = 0;




typedef tcp_svr_connection<my_package,			// Message
	 	 				   my_parse,			// Define static header_size
	 	 				   my_tss_data			// The thread's storage
	 	 	> my_connection;

class handler_mgr : public tcp_handler_svr_manager<my_connection>{
public:
	handler_mgr():tcp_handler_svr_manager<my_connection>(){}
};
typedef bingo::singleton_v0<handler_mgr> my_mgr;

typedef  tcp_server<my_connection,
					my_mgr,
					my_parse,
					my_tss_data
			> my_server;

vector<void*> handler_pointers;



// ------------------- 测试客户端链接成功，服务器accept_success返回-1，服务器主动断开链接 ------------------------------- //

int accept_success_return_false(my_server::pointer ptr, u16& err_code){
	cout << "handler:" << ptr.get() << ",do accept_success_return_false()" << endl;
	err_code = error_tcp_unknown;
	return -1;
}

int accept_success_return_true(my_server::pointer ptr, u16& err_code){
	cout << "handler:" << ptr.get() << ",do accept_success_return_true()" << endl;
	return 0;
}

void catch_error1(my_server::pointer ptr, u16& err_code){
	cout << "handler:" << ptr.get() << ",err_code:" << err_code << ",do catch_error()" << endl;
}


BOOST_AUTO_TEST_CASE(t_tcp_server1){

	my_mgr::create();	 			// Create tcp_handler_manager.

	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_server server(io_service, ipv4, port,
				 accept_success_return_false,			// accept_success_callback
				 0,
				 catch_error1							// catch_error_callback
				 );

		 io_service.run();

	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	my_mgr::release();

	// output:
	//	thr:7fe8a583a720,call start_accept()
	//	thr:7fe8a583a720,call accept_handler()
	//	handler:0x25de3e0,do accept_success_return_false()
	//	handler:0x25de3e0,err_code:271,do catch_error()			// error_tcp_unknown
	//	thr:7fe8a583a720,call start_accept()
	//	hander:0x25de3e0,destory!


}

void run_client1(){
	try
	{
		cout << "client start." << endl;

		io_service ios;
		ip::tcp::socket sock(ios);						//创建socket对象

		ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 18000);	//创建连接端点


		sock.connect(ep);								//socket连接到端点

		cout << "client msg : remote_endpoint : " << sock.remote_endpoint().address() << endl;	//输出远程连接端点信息

		char data[16] = {0x68, 0x33, 0x00, 0x05, 0xd9, 0x8c, 0xee, 0x47, 0x16, 0x01, 0x01, 0x8f, 0x72, 0xd8, 0x0d, 0x16};

		sock.write_some(buffer(&data[0], 16)); //发送数据

		while(true){
			vector<char> str(256, 0);						//定义一个vector缓冲区
			size_t received_size = sock.read_some(buffer(str));					//使用buffer()包装缓冲区数据

			bingo::stringt tool;
			cout << "receviced data:" << tool.stream_to_string(&str[0], received_size) << endl;

	//		this_thread::sleep(seconds(10));
		}

		sock.close();

	}
	catch(std::exception& e)							//捕获错误
	{
		cout << "exception:" << e.what() << endl;
	}
};

BOOST_AUTO_TEST_CASE(t_client1){

	run_client1();
}

// ------------------- 测试客户端链接成功，客户端断开链接 ----------------------------------- //

int read_pk_header_completed_return_true(my_server::pointer ptr,
		char*& rev_data,
		size_t& rev_data_size,
		size_t& remain_size,
		u16& err_code){

	cout << "handler:" << ptr.get() << ",do read_pk_header_completed_return_true()" << endl;

	remain_size = 5 + 2 + 4 + 1;

	return 0;
};

void catch_error2(my_server::pointer ptr, u16& err_code){
	cout << "handler:" << ptr.get() << ",err_code:" << err_code << ",do catch_error()" << endl;
	BOOST_CHECK(err_code == error_tcp_server_close_socket_because_client);
}

BOOST_AUTO_TEST_CASE(t_tcp_server2){


	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_server server(io_service, ipv4, port,
				 0, 0,
				 catch_error2,								// catch_error_callback function
				 read_pk_header_completed_return_true		// read_pk_header_complete_callback function
				 );

		 io_service.run();

	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}


	// output:
	//	thr:7f96301ba720,call start_accept()
	//	thr:7f96301ba720,call accept_handler()
	//	thr:7f96301ba720,call start_accept()
	//	thr:7f96301ba720,call read_handler()
	//	handler:0x233b3e0,do read_pk_header_completed_return_true()
	//	thr:7f96301ba720,call read_handler()
	//	thr:7f96301ba720,call read_handler()
	//	handler:0x233b3e0,err_code:265,do catch_error()				// error_tcp_server_close_socket_because_client
	//	hander:0x233b3e0,destory!

}

void run_client2(){
	try
	{
		cout << "client start." << endl;

		io_service ios;
		ip::tcp::socket sock(ios);						//创建socket对象

		ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 18000);	//创建连接端点


		sock.connect(ep);								//socket连接到端点

		cout << "client msg : remote_endpoint : " << sock.remote_endpoint().address() << endl;	//输出远程连接端点信息

		char data[16] = {0x68, 0x33, 0x00, 0x05, 0xd9, 0x8c, 0xee, 0x47, 0x16, 0x01, 0x01, 0x8f, 0x72, 0xd8, 0x0d, 0x16};

		sock.write_some(buffer(&data[0], 16)); //发送数据

		sock.close();

	}
	catch(std::exception& e)							//捕获错误
	{
		cout << "exception:" << e.what() << endl;
	}
};

BOOST_AUTO_TEST_CASE(t_client2){

	run_client2();
}

// ------------------- 测试客户端链接成功，服务器15秒后主动断开链接 ------------------------------------ //

int accept_success_insert_handler_mgr(my_server::pointer ptr,u16& err_code){

	my_mgr::instance()->insert(ptr.get());
	cout << "handler:" << ptr.get() << ",do accept_success_check_handler_mgr()" << endl;
	handler_pointers.push_back(ptr.get());
	return 0;
}

void close_completed_erase_hander_mgr(my_server::pointer ptr, int& ec_value){

	u16 err_code = 0;
	if(my_mgr::instance()->erase(ptr.get(), err_code) == 0){
		cout << "handler:" << ptr.get() << ",do close_completed_erase_hander_mgr(),success" << endl;
	}else{
		cout << "handler:" << ptr.get() << ",do close_completed_erase_hander_mgr(),err_code:" << err_code << endl;
	}
}

void run_thread3(){

	this_thread::sleep(seconds(15));

	if(handler_pointers.size() > 0){
		u16 err_code = 0;
		BOOST_CHECK(my_mgr::instance()->send_close(handler_pointers[0], err_code) == 0);
	}
};

BOOST_AUTO_TEST_CASE(t_tcp_server3){

	my_mgr::create();	 			// Create 4 element in tcp_handler_manager.

	boost::thread t(run_thread3);

	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_server server(io_service, ipv4, port,
				 accept_success_insert_handler_mgr,				// accept_success_callback
				 0,
				 catch_error1,									// catch_error_callback
				 read_pk_header_completed_return_true,			// read_pk_header_complete_callback
				 0,0,0,
				 close_completed_erase_hander_mgr				// close_complete_callback
				 );

		 io_service.run();

		 t.join();

	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	my_mgr::release();

	// output:
	//	thr:7fd5df4ad720,call start_accept()
	//	thr:7fd5df4ad720,call accept_handler()
	//	handler:0xcc9690,do accept_success_check_handler_mgr()
	//	thr:7fd5df4ad720,call start_accept()
	//	thr:7fd5df4ad720,call read_handler()
	//	handler:0xcc9690,do read_pk_header_completed_return_true()
	//	thr:7fd5df4ad720,call read_handler()
	//
	//	handler:0xcc9690,err_code:268,do catch_error()				// error_tcp_server_close_socket_because_package
	//	thr:7fd5df4ad720,call read_handler()
	//	handler:0xcc9690,do close_completed_erase_hander_mgr(),success
	//	hander:0xcc9690,destory!


}

BOOST_AUTO_TEST_CASE(t_client3){

	run_client1();
}

// ------------------- 测试客户端链接成功，发送报文，服务器验资报文头错误，主动断开链接 ---------------------------------- //

int read_pk_header_completed_return_false(my_server::pointer ptr,
		char*& rev_data,
		size_t& rev_data_size,
		size_t& remain_size,
		u16& err_code){

//	bingo::stringt t;
	cout << "handler:" << ptr.get() << ",do read_pk_header_completed_return_false(), header data:" <<
			ptr->this_tss()->t.stream_to_string(rev_data, rev_data_size) << endl;
	err_code = error_tcp_package_header_is_wrong;

	return -1;
};

void catch_error4(my_server::pointer ptr, u16& err_code){
	cout << "handler:" << ptr.get() << ",err_code:" << err_code << ",do catch_error()" << endl;
}

BOOST_AUTO_TEST_CASE(t_tcp_server4){

	my_mgr::create();	 			// Create tcp_handler_manager.

	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_server server(io_service, ipv4, port,
				 accept_success_insert_handler_mgr,				// accept_success_callback
				 0,
				 catch_error4,									// catch_error_callback
				 read_pk_header_completed_return_false,			// read_pk_header_complete_callback
				 0,0,0,
				 close_completed_erase_hander_mgr				// close_complete_callback function
				 );

		 io_service.run();


	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	my_mgr::release();

	// output:
	//	thr:7f45b7ef7720,call start_accept()
	//	thr:7f45b7ef7720,call accept_handler()
	//	handler:0x247c690,do accept_success_check_handler_mgr()
	//	thr:7f45b7ef7720,call start_accept()
	//	thr:7f45b7ef7720,call read_handler()
	//	handler:0x247c690,do read_pk_header_completed_return_false(), header data:68 33 00 05
	//	handler:0x247c690,err_code:261,do catch_error()						// error_tcp_package_header_is_wrong
	//	handler:0x247c690,do close_completed_erase_hander_mgr(),success
	//	hander:0x247c690,destory!

}

BOOST_AUTO_TEST_CASE(t_client4){

	run_client1();
}

// ------------------- 测试客户端链接成功，发送报文，服务器接受完整报文，验资错误，主动断开链接 -------------------------- //

int read_pk_full_completed_return_false(
		my_server::pointer ptr,
		char*& rev_data,
		size_t& rev_data_size,
		u16& err_code){

	cout << "handler:" << ptr.get() << ",do read_pk_full_completed_return_false()" << endl;
	err_code = error_tcp_package_body_is_wrong;
	return -1;
}

void catch_error5(my_server::pointer ptr, u16& err_code){
	cout << "handler:" << ptr.get() << ",err_code:" << err_code << ",do catch_error()" << endl;
	BOOST_CHECK(err_code == 0x0106);
}

BOOST_AUTO_TEST_CASE(t_tcp_server5){

	my_mgr::create();	 			// Create tcp_handler_manager.

	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_server server(io_service, ipv4, port,
				 accept_success_insert_handler_mgr,				// accept_success_callback
				 0,
				 catch_error5,									// catch_error_callback
				 read_pk_header_completed_return_true,			// read_pk_header_complete_callback
				 read_pk_full_completed_return_false,			// read_pk_full_complete_callback
				 0, 0,
				 close_completed_erase_hander_mgr				// close_complete_callback function
				 );

		 io_service.run();


	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	my_mgr::release();

	// output:
	//	thr:7fb5ba63a720,call start_accept()
	//	thr:7fb5ba63a720,call accept_handler()
	//	handler:0x1080690,do accept_success_check_handler_mgr()
	//	thr:7fb5ba63a720,call start_accept()
	//	thr:7fb5ba63a720,call read_handler()
	//	handler:0x1080690,do read_pk_header_completed_return_true()
	//	thr:7fb5ba63a720,call read_handler()
	//	handler:0x1080690,do read_pk_full_completed_return_false()
	//	handler:0x1080690,err_code:262,do catch_error()						// error_tcp_package_body_is_wrong
	//	handler:0x1080690,do close_completed_erase_hander_mgr(),success
	//	hander:0x1080690,destory!

}

BOOST_AUTO_TEST_CASE(t_client5){

	run_client1();
}

// ------------------- 测试4个独立客户端链接成功，服务器向各客户端分别发送正确报文10次后，主动断开 ------------------------------- //

void write_pk_full_completed(
		my_server::pointer ptr,
		char*& snd_p,
		size_t& snd_size,
		const boost::system::error_code& ec){

//	bingo::stringt t;

	cout << "handler:" << ptr.get() << ",do write_pk_full_completed(), snd_data:"
			<< ptr->this_tss()->t.stream_to_string(snd_p, snd_size) << ",ec:" << ec.message() << endl;
}

void catch_error6(my_server::pointer ptr, u16& err_code){
	cout << "handler:" << ptr.get() << ",err_code:" << err_code << ",do catch_error()" << endl;
}

void run_thread6(int& n){

	this_thread::sleep(seconds(10));

	if(handler_pointers.size() > 0){

		u32 idx = n -1;
		u16 err_code = 0;


		for (int i = 0; i < 10; ++i) {

			char data[16] = {0x68, 0x01, 0x00, 0x00, 0xd9, 0x8c, 0xee, 0x47, 0x16, 0x01, 0x01, 0x8f, 0x72, 0xd8, 0x0d, 0x16};
			size_t data_size = 16;

			data[1] = (u8)n;
			data[3] = (u8)i;

			bingo::stringt t;
			cout << "thread data:" << t.stream_to_string(&data[0], data_size) << endl;

			BOOST_CHECK(my_mgr::instance()->send_data(handler_pointers[idx], &data[0], data_size, err_code) == 0);
		}

		BOOST_CHECK(my_mgr::instance()->send_close(handler_pointers[idx], err_code) == 0);
	}
}

BOOST_AUTO_TEST_CASE(t_tcp_server6){

	my_mgr::create();	 			// Create tcp_handler_manager.

	boost::thread t1(run_thread6, 1);
	boost::thread t2(run_thread6, 2);
	boost::thread t3(run_thread6, 3);
	boost::thread t4(run_thread6, 4);

	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_server server(io_service, ipv4, port,
				 accept_success_insert_handler_mgr,				// accept_success_callback
				 0,
				 catch_error6,									// catch_error_callback
				 read_pk_header_completed_return_true,			// read_pk_header_complete_callback
				 0,
				 write_pk_full_completed,						// write_pk_full_complete_callback
				 0,
				 close_completed_erase_hander_mgr				// close_complete_callback
				 );

		 io_service.run();

		 t1.join();
		 t2.join();
		 t3.join();
		 t4.join();

	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	my_mgr::release();

	// output:

}

void run_client6(int& n){

	try
	{
		cout << "n:" << n << ",client start." << endl;

		io_service ios;
		ip::tcp::socket sock(ios);						//创建socket对象

		ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 18000);	//创建连接端点


		sock.connect(ep);								//socket连接到端点

		cout << "n:" << n << ",client msg : remote_endpoint : " << sock.remote_endpoint().address() << endl;	//输出远程连接端点信息

		char data[16] = {0x68, 0x33, 0x00, 0x05, 0xd9, 0x8c, 0xee, 0x47, 0x16, 0x01, 0x01, 0x8f, 0x72, 0xd8, 0x0d, 0x16};

		sock.write_some(buffer(&data[0], 16)); //发送数据

		while(true){
			vector<char> str(256, 0);						//定义一个vector缓冲区
			size_t received_size = sock.read_some(buffer(str));					//使用buffer()包装缓冲区数据

			bingo::stringt tool;
			cout << "n:" << n << ",receviced data:" << tool.stream_to_string(&str[0], received_size) << endl;

	//		this_thread::sleep(seconds(10));
		}

		sock.close();

	}
	catch(std::exception& e)							//捕获错误
	{
		cout << "n:" << n << ",exception:" << e.what() << endl;
	}

}

BOOST_AUTO_TEST_CASE(t_client6){

	boost::thread t1(run_client6, 1);
	boost::thread t2(run_client6, 2);
	boost::thread t3(run_client6, 3);
	boost::thread t4(run_client6, 4);

	 t1.join();
	 t2.join();
	 t3.join();
	 t4.join();
}

// ------------------- 测试5个独立客户端链接服务器，服务器最大只支持3个客户端链接，多的主动断开 ----------------------- //

int accept_success_insert_max_three_handler(my_server::pointer ptr,u16& err_code){

	if(my_mgr::instance()->size() == 3){

		cout << "handler:" << ptr.get() << ",do accept_success_check_handler_mgr() fail" << endl;
		err_code = error_tcp_handler_mgr_sequence_is_full;
		return -1;
	}else{
		my_mgr::instance()->insert(ptr.get());
		cout << "handler:" << ptr.get() << ",do accept_success_check_handler_mgr() success" << endl;
		handler_pointers.push_back(ptr.get());
		return 0;
	}
}

BOOST_AUTO_TEST_CASE(t_tcp_server7){

	my_mgr::create();	 			// Create tcp_handler_manager.

	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_server server(io_service, ipv4, port,
				 accept_success_insert_max_three_handler,		// accept_success_callback
				 0,
				 catch_error6,									// catch_error_callback
				 read_pk_header_completed_return_true,			// read_pk_header_complete_callback
				 0,
				 0,
				 0,
				 close_completed_erase_hander_mgr				// close_complete_callback
				 );

		 io_service.run();

	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	my_mgr::release();

	// output:
	//	handler:0xc893e0,do accept_success_check_handler_mgr() success			// one client success
	//	handler:0xc893e0,do read_pk_header_completed_return_true()
	//	handler:0xc89a10,do accept_success_check_handler_mgr() success			// two client success
	//	handler:0xc89d10,do accept_success_check_handler_mgr() success			// three client success
	//	handler:0xc89d10,do read_pk_header_completed_return_true()
	//	handler:0xc89a10,do read_pk_header_completed_return_true()
	//	handler:0xc8a180,do accept_success_check_handler_mgr() fail				// four client fail
	//	handler:0xc8a180,err_code:257,do catch_error()							// error_tcp_handler_mgr_sequence_is_full
	//	hander:0xc8a180,destory!
	//	handler:0xc8a3c0,do accept_success_check_handler_mgr() fail				// fifth client fail
	//	handler:0xc8a3c0,err_code:257,do catch_error()							// error_tcp_handler_mgr_sequence_is_full
	//	hander:0xc8a3c0,destory!

}

BOOST_AUTO_TEST_CASE(t_client7){

	boost::thread t1(run_client6, 1);
	boost::thread t2(run_client6, 2);
	boost::thread t3(run_client6, 3);
	boost::thread t4(run_client6, 4);
	boost::thread t5(run_client6, 5);

	 t1.join();
	 t2.join();
	 t3.join();
	 t4.join();
	 t5.join();
}

// --------- 测试4个独立客户端链接成功，服务器向各客户端间隔20ms分别发送正确报文n次,客户端接受正常 ---------- //

int snd_amount_1 = 0;
int snd_amount_2 = 0;
int snd_amount_3 = 0;
int snd_amount_4 = 0;

void write_pk_full_completed8(
		my_server::pointer ptr,
		char*& snd_p,
		size_t& snd_size,
		const boost::system::error_code& ec){

//	bingo::stringt t;

	if(snd_p[1] == 1){
		snd_amount_1++;
		cout << "handler:" << ptr.get() << ",amount1:" << snd_amount_1 << endl;
	}else if(snd_p[1] == 2){
		snd_amount_2++;
		cout << "handler:" << ptr.get() << ",amount2:" << snd_amount_2 << endl;
	}else if(snd_p[1] == 3){
		snd_amount_3++;
		cout << "handler:" << ptr.get() << ",amount3:" << snd_amount_3 << endl;
	}else if(snd_p[1] == 4){
		snd_amount_4++;
		cout << "handler:" << ptr.get() << ",amount4:" << snd_amount_4 << endl;
	}

//	cout << "handler:" << ptr.get() << ",do write_pk_full_completed(), snd_data:"
//			<< ptr->this_tss()->t.stream_to_string(snd_p, snd_size) << ",ec:" << ec.message() << endl;
}

void run_thread8(int& n){

	this_thread::sleep(seconds(10));

	if(handler_pointers.size() > 0){

		u32 idx = n -1;
		u16 err_code = 0;

		char data[16] = {0x68, 0x01, 0x00, 0x00, 0xd9, 0x8c, 0xee, 0x47, 0x16, 0x01, 0x01, 0x8f, 0x72, 0xd8, 0x0d, 0x16};
		size_t data_size = 16;


//		while(true){
		for (int i = 0; i < 1000; ++i) {

			data[1] = (u8)n;
//			data[3] = (u8)i;

			BOOST_CHECK(my_mgr::instance()->send_data(handler_pointers[idx], &data[0], data_size, err_code) == 0);

			this_thread::sleep(milliseconds(20));
		}

		BOOST_CHECK(my_mgr::instance()->send_close(handler_pointers[idx], err_code) == 0);
	}
}

BOOST_AUTO_TEST_CASE(t_tcp_server8){

	my_mgr::create();	 			// Create tcp_handler_manager.

	boost::thread t1(run_thread8, 1);
	boost::thread t2(run_thread8, 2);
	boost::thread t3(run_thread8, 3);
	boost::thread t4(run_thread8, 4);

	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_server server(io_service, ipv4, port,
				 accept_success_insert_handler_mgr,				// accept_success_callback
				 0,
				 catch_error6,									// catch_error_callback
				 read_pk_header_completed_return_true,			// read_pk_header_complete_callback
				 0,
				 write_pk_full_completed8,						// write_pk_full_complete_callback
				 0,
				 close_completed_erase_hander_mgr				// close_complete_callback
				 );

		 io_service.run();

		 t1.join();
		 t2.join();
		 t3.join();
		 t4.join();

	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	my_mgr::release();

	// output:

}

BOOST_AUTO_TEST_CASE(t_client8){

	boost::thread t1(run_client6, 1);
	boost::thread t2(run_client6, 2);
	boost::thread t3(run_client6, 3);
	boost::thread t4(run_client6, 4);


	 t1.join();
	 t2.join();
	 t3.join();
	 t4.join();

}

// --------- 测试客户端链接成功，服务器验证socket是否合法，不合法主动断开链接 ---------- //

int read_pk_header_completed_all(my_server::pointer ptr,
		char*& rev_data,
		size_t& rev_data_size,
		size_t& remain_size,
		u16& err_code){

	cout << "handler:" << ptr.get() << ",do read_pk_header_completed_return_true()" << endl;

//	char* p = rev_mgr.header();

	size_t size = (u8)rev_data[3];
	remain_size = size + 2 + 4 + 1;

	return 0;
};

int read_pk_full_completed_all(
		my_server::pointer ptr,
		char*& rev_data,
		size_t& rev_data_size,
		u16& err_code){

//	char* p = rev_mgr.header();

	if(rev_data[1] == 0x23){
		// heartjump
		ptr->set_heartjump_datetime();
		cout << "handler:" << ptr.get() << ",do set_heartjump_datetime()" << endl;
	}

	return 0;
}

int active_send_in_ioservice_authentication_pass(my_server::pointer ptr, char*& snd_p, size_t& snd_size, u16& err_code){

	if(snd_p[1] == 0x01){
		// authencation is pass
		ptr->set_authentication_pass();
		cout << "handler:" << ptr.get() << ",do set_authentication_pass()" << endl;
	}

	return 0;
}

void catch_error_all(my_server::pointer ptr, u16& err_code){
	cout << "handler:" << ptr.get() << ",err_code:" << err_code << ",do catch_error()" << endl;
}

void run_thread9(int& n){

	if(n == 1){
//		for (int i = 0; i < 3; ++i) {
//
//			this_thread::sleep(seconds(15));
//
//			my_mgr::instance()->check_authentication_pass();
//			cout << "start: check_authentication_pass" << endl;
//		}
	}else{

		this_thread::sleep(seconds(10));

		char data[5] = {0x68, 0x01, 0x00, 0x00, 0x16};
		size_t data_size = 5;

		u16 err_code = 0;
		BOOST_CHECK(my_mgr::instance()->send_data(handler_pointers[1], &data[0], data_size, err_code) == 0);
	}

}

BOOST_AUTO_TEST_CASE(t_tcp_server9){

	my_parse::max_wait_for_authentication_pass_seconds = 15;

	my_mgr::create();	 			// Create tcp_handler_manager.


//	boost::thread t1(run_thread9, 1);		// Valid authencation is pass
	boost::thread t2(run_thread9, 2);		// Set second socket that authencation is pass.

	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_server server(io_service, ipv4, port,
				 accept_success_insert_handler_mgr,				// accept_success_callback
				 0,
				 catch_error_all,								// catch_error_callback
				 read_pk_header_completed_all,					// read_pk_header_complete_callback
				 read_pk_full_completed_all,					// read_pk_full_complete_callback
				 0,
				 active_send_in_ioservice_authentication_pass,	// active_send_in_ioservice_callback
				 close_completed_erase_hander_mgr				// close_complete_callback
				 );

		 io_service.run();

//		 t1.join();
		 t2.join();


	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	my_mgr::release();

	// output:
	//	handler:0x102a690,do accept_success_check_handler_mgr()
	//	handler:0x102a690,do read_pk_header_completed_return_true()
	//	handler:0x102ab00,do accept_success_check_handler_mgr()
	//	handler:0x102ab00,do read_pk_header_completed_return_true()
	//
	//
	//	handler:0x102ab00,do set_authentication_pass()
	//	start: check_authentication_pass, time:2016-03-04T22:50:36
	//	handler:0x102a690,err_code:267,do catch_error()						// error_tcp_server_close_socket_because_authrication_pass
	//	handler:0x102a690,do close_completed_erase_hander_mgr(),success
	//	hander:0x102a690,destory!
	//	start: check_authentication_pass, time:2016-03-04T22:50:51
	//	start: check_authentication_pass, time:2016-03-04T22:51:06
}

BOOST_AUTO_TEST_CASE(t_client9){

	boost::thread t1(run_client6, 1);
	boost::thread t2(run_client6, 2);

	 t1.join();
	 t2.join();

}

// --------- 测试客户端链接成功，服务器3秒心跳超时，主动断开链接 ---------- //

void run_thread10(int& n){

	if(n == 1){
//		for (int i = 0; i < 3; ++i) {
//
//			this_thread::sleep(seconds(15));
//
//			my_mgr::instance()->check_heartjump();
//			cout << "start: check_heartjump" << endl;
//		}
	}else{

		this_thread::sleep(seconds(10));

		char data[5] = {0x68, 0x01, 0x00, 0x00, 0x16};
		size_t data_size = 5;

		u16 err_code = 0;
		BOOST_CHECK(my_mgr::instance()->send_data(handler_pointers[0], &data[0], data_size, err_code) == 0);
		BOOST_CHECK(my_mgr::instance()->send_data(handler_pointers[1], &data[0], data_size, err_code) == 0);
	}

}

void run_client10(int& n){

	if(n == 2)
		this_thread::sleep(seconds(2));
	if(n == 3)
		this_thread::sleep(seconds(4));

	try
	{
		cout << "n:" << n << ",client start." << endl;

		io_service ios;
		ip::tcp::socket sock(ios);						//创建socket对象

		ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 18000);	//创建连接端点


		sock.connect(ep);								//socket连接到端点

		cout << "n:" << n << ",client msg : remote_endpoint : " << sock.remote_endpoint().address() << endl;	//输出远程连接端点信息

		char data[16] = {0x68, 0x33, 0x00, 0x05, 0xd9, 0x8c, 0xee, 0x47, 0x16, 0x01, 0x01, 0x8f, 0x72, 0xd8, 0x0d, 0x16};

		sock.write_some(buffer(&data[0], 16)); //发送数据

		if(n == 1){

			vector<char> str(256, 0);						//定义一个vector缓冲区
			size_t received_size = sock.read_some(buffer(str));					//使用buffer()包装缓冲区数据

			bingo::stringt tool;
			cout << "n:" << n << ",receviced data:" << tool.stream_to_string(&str[0], received_size) << endl;

			while(true){
				char data2[11] = {0x68, 0x23, 0x00, 0x00, 0x01, 0x00, 0x8f, 0x72, 0xd8, 0x0d, 0x16};

				sock.write_some(buffer(&data2[0], 11)); //发送数据

				this_thread::sleep(seconds(4));
			}

		}else{
			while(true){
				vector<char> str(256, 0);						//定义一个vector缓冲区
				size_t received_size = sock.read_some(buffer(str));					//使用buffer()包装缓冲区数据

				bingo::stringt tool;
				cout << "n:" << n << ",receviced data:" << tool.stream_to_string(&str[0], received_size) << endl;

		//		this_thread::sleep(seconds(10));
			}
		}

		sock.close();

	}
	catch(std::exception& e)							//捕获错误
	{
		cout << "n:" << n << ",exception:" << e.what() << endl;
	}

}

BOOST_AUTO_TEST_CASE(t_tcp_server10){

	my_parse::max_wait_for_heartjump_seconds = 15;

	my_mgr::create();	 			// Create tcp_handler_manager.

	boost::thread t1(run_thread10, 2);		// Set all socket which authencation is pass.
//	boost::thread t2(run_thread10, 1);		// Start to check heartjump.

	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_server server(io_service, ipv4, port,
				 accept_success_insert_handler_mgr,				// accept_success_callback
				 0,
				 catch_error_all,								// catch_error_callback
				 read_pk_header_completed_all,					// read_pk_header_complete_callback
				 read_pk_full_completed_all,					// read_pk_full_complete_callback
				 0,
				 active_send_in_ioservice_authentication_pass,	// active_send_in_ioservice_callback
				 close_completed_erase_hander_mgr				// close_complete_callback
				 );

		 io_service.run();

		 t1.join();
//		 t2.join();


	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	my_mgr::release();

	// output:
	//	handler:0xece690,do accept_success_check_handler_mgr()
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xeceb00,do accept_success_check_handler_mgr()
	//	handler:0xeceb00,do read_pk_header_completed_return_true()
	//	handler:0xecef70,do accept_success_check_handler_mgr()
	//	handler:0xecef70,do read_pk_header_completed_return_true()

	//	handler:0xece690,do set_authentication_pass()						// first socket authentication pass
	//	handler:0xeceb00,do set_authentication_pass()						// second socket authentication pass

	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	start: check_heartjump, time:2016-03-04T22:55:39
	//	handler:0xecef70,err_code:267,do catch_error()
	//	handler:0xecef70,do close_completed_erase_hander_mgr(),success
	//	hander:0xecef70,destory!											// close thirty socket

	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	start: check_heartjump, time:2016-03-04T22:55:54
	//	handler:0xeceb00,err_code:266,do catch_error()
	//	handler:0xeceb00,do close_completed_erase_hander_mgr(),success
	//	hander:0xeceb00,destory!											// close second socket

	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	start: check_heartjump, time:2016-03-04T22:56:09
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	start: check_heartjump, time:2016-03-04T22:56:24
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()
	//	start: check_heartjump, time:2016-03-04T22:56:39
	//	handler:0xece690,do read_pk_header_completed_return_true()
	//	handler:0xece690,do set_heartjump_datetime()



}

BOOST_AUTO_TEST_CASE(t_client10){

	boost::thread t1(run_client10, 1);
	boost::thread t2(run_client10, 2);
	boost::thread t3(run_client10, 3);

	 t1.join();
	 t2.join();
	 t3.join();

}

BOOST_AUTO_TEST_SUITE_END()


