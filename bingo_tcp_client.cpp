/*
 * boost_asio_tcp_client.cpp
 *
 *  Created on: 2016-1-22
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

#define BINGO_TCP_CLIENT_DEBUG
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



BOOST_AUTO_TEST_SUITE(bingo_tcp_client)

struct my_tss_data : public thread_tss_data{
	bingo::stringt t;
};

struct my_parse{
	static int retry_delay_seconds_when_connec;				// When Client connect fail, don't reconnect if the value is 0.
	static int max_retry_delay_seconds_when_connec;

	static const size_t header_size;								// Parse size of package's header.
	static int max_interval_seconds_when_send_heartjump;	// Client don't send heartjump if the value is 0.
};
const size_t my_parse::header_size = 4;
int my_parse::retry_delay_seconds_when_connec = 3;
int my_parse::max_retry_delay_seconds_when_connec = 60;
int my_parse::max_interval_seconds_when_send_heartjump = 2;

#pragma pack(1)
struct my_package{
	char data[256];
};

struct my_heartjump{
	static size_t data_size;
	static char data[11];
};
#pragma pack()

size_t 	my_heartjump::data_size = 11;
char 	my_heartjump::data[11] = {0x68, 0x23, 0x00, 0x00, 0x01, 0x00, 0x8f, 0x72, 0xd8, 0x0d, 0x16};

typedef tcp_cet_connection<
			 my_package,			// Message
			 my_parse,				// Define static header_size
			 my_heartjump,			// Type of Heartjump package
			 my_tss_data			// The thread's storage
 	 	 > my_connectiion;

class handler_mgr : public tcp_handler_cet_manager<my_connectiion>{
public:
	handler_mgr():tcp_handler_cet_manager<my_connectiion>(){}
};
typedef bingo::singleton_v0<handler_mgr> my_mgr;

typedef  tcp_client<
			my_connectiion,
			my_parse,
			my_package,
			my_tss_data
	 > my_client;


// -------------------------- 测试链接失败，重连 ---------------------- //

void connet_error(my_client::pointer ptr, int& retry_delay_seconds){
	SHOW_CURRENT_TIME("connet_error");
	cout << "hander:" << ptr.get() << ",reconnect after " << retry_delay_seconds << " seconds." << endl;
};

int connect_success_insert_handler_mgr(my_client::pointer ptr,u16& err_code){

	my_mgr::instance()->insert(ptr.get());
	cout << "handler:" << ptr.get() << ",do connect_success_insert_handler_mgr()" << endl;
	return 0;

}

void close_completed_erase_hander_mgr(my_client::pointer ptr, int& ec_value){

	u16 err_code = 0;
	if(my_mgr::instance()->erase(ptr.get(), err_code) == 0){
		cout << "handler:" << ptr.get() << ",do close_completed_erase_hander_mgr() success, ec_value:" << ec_value << endl;
	}else{
		cout << "handler:" << ptr.get() << ",do close_completed_erase_hander_mgr() fail,err_code:" << err_code << endl;
	}
}

void catch_error(my_client::pointer ptr, u16& err_code){
	cout << "handler:" << ptr.get() << ",err_code:" << err_code << ",do catch_error()" << endl;
}

void write_pk_full_completed(my_client::pointer ptr, char*& snd_p, size_t& snd_size, const boost::system::error_code& ec){

//	bingo::stringt t;

	cout << "handler:" << ptr.get() << ",do write_pk_full_completed(), snd_data:"
			<< ptr->this_tss()->t.stream_to_string(snd_p, snd_size) << ",ec:" << ec.message() << endl;
}

BOOST_AUTO_TEST_CASE(t_client1){

	my_mgr::create();

	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_client c(io_service, ipv4, port,
				 0,				// connet_success_callback
				 connet_error	// connet_error_callback
				 );

		 io_service.run();

	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	my_mgr::release();
}

// -------------------------- 测试链接成功，然后被服务器断开，自动重连 ---------------------- //

int read_pk_header_completed_all(my_client::pointer ptr,
		char*& rev_data, size_t& rev_data_size,
		size_t& remain_size,
		u16& err_code){

	char* p = rev_data;

	size_t size = (u8)p[3];
	remain_size = size + 2 + 4 + 1;

	return 0;
};

int read_pk_full_completed_all(my_client::pointer ptr, char*& rev_data, size_t& rev_data_size, u16& err_code){

//	bingo::stringt t;
	cout << "handler:" << ptr.get() << ",do s_read_pk_full_completed_return_true(), data:"
			<< ptr->this_tss()->t.stream_to_string(rev_data, rev_data_size) << endl;

	return 0;
}

BOOST_AUTO_TEST_CASE(t_client2){

	my_mgr::create();

	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_client c(io_service, ipv4, port,
				 connect_success_insert_handler_mgr,				// connet_success_callback
				 connet_error,										// connet_error_callback
				 catch_error,
				 read_pk_header_completed_all,
				 read_pk_full_completed_all,
				 0,
				 0,
				 close_completed_erase_hander_mgr					// close_complete_callback
				 );

		 io_service.run();

	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	my_mgr::release();
}

// -------------------------- 测试链接成功，发送数据包，并接受服务器来的数据包 ---------------------- //

vector<void*> handler_pointers;

struct my_tss_data_svr : public thread_tss_data{
	bingo::stringt t;
};

struct my_parse_svr{
	static const size_t header_size;
	static int max_wait_for_heartjump_seconds;
	static int max_wait_for_authentication_pass_seconds;


};
const size_t my_parse_svr::header_size = 4;
int my_parse_svr::max_wait_for_heartjump_seconds = 0;
int my_parse_svr::max_wait_for_authentication_pass_seconds = 0;


typedef tcp_svr_connection<my_package,				// Message
	 	 				   my_parse_svr,			// Define static header_size
	 	 				   my_tss_data_svr			// The thread's storage
	 	 	> my_connection_svr;

class handler_mgr_svr : public tcp_handler_svr_manager<my_connection_svr>{
public:
	handler_mgr_svr():tcp_handler_svr_manager<my_connection_svr>(){}
};
typedef bingo::singleton_v0<handler_mgr_svr> my_mgr_svr;

typedef  tcp_server<my_connection_svr,
	 				my_mgr_svr,
	 				my_parse_svr,
	 				my_package,
	 				my_tss_data_svr
	 > my_server;


int server_success_insert_handler_mgr(my_server::pointer ptr,u16& err_code){


	my_mgr_svr::instance()->insert(ptr.get());
	cout << "handler:" << ptr.get() << ",do server_success_insert_handler_mgr()" << endl;

	handler_pointers.push_back(ptr.get());

	return 0;
}

void server_close_completed_erase_hander_mgr(my_server::pointer ptr, int& ec_value){

	u16 err_code = 0;
	if(my_mgr_svr::instance()->erase(ptr.get(), err_code) == 0){
		cout << "handler:" << ptr.get() << ",do server_close_completed_erase_hander_mgr() success, ec_value:" << ec_value << endl;
	}else{
		cout << "handler:" << ptr.get() << ",do server_close_completed_erase_hander_mgr(),err_code:" << err_code << endl;
	}
}


int server_read_pk_header_completed_all(my_server::pointer ptr,
		char*& rev_data, size_t& rev_data_size,
		size_t& remain_size,
		u16& err_code){

	char* p = rev_data;

	size_t size = (u8)p[3];
	remain_size = size + 2 + 4 + 1;

	return 0;
};

int server_read_pk_full_completed_all(my_server::pointer ptr, char*& rev_data, size_t& rev_data_size, u16& err_code){

//	bingo::stringt t;
	cout << "handler:" << ptr.get() << ",do server_read_pk_full_completed_all(), data:"
			<< ptr->this_tss()->t.stream_to_string(rev_data, rev_data_size) << endl;

	return 0;
}

void server_write_pk_full_complete(my_server::pointer ptr, char*& snd_p, size_t& snd_size, const boost::system::error_code& ec){
	cout << "handler:" << ptr.get() << ",do server_write_pk_full_complete(), data:"
				<< ptr->this_tss()->t.stream_to_string(snd_p, snd_size) << endl;

}




void run_server_thread(){

	this_thread::sleep(seconds(10));

	u16 err_code = 0;
	for (int i = 0; i < 4; ++i) {

		char data[16] = {0x68, 0x01, 0x00, 0x05, 0xd9, 0x8c, 0xee, 0x47, 0x16, 0x01, 0x01, 0x8f, 0x72, 0xd8, 0x0d, 0x16};
		size_t data_size = 16;

//		data[3] = (u8)i;

		BOOST_CHECK(my_mgr_svr::instance()->send_data(handler_pointers[0], &data[0], data_size, err_code) == 0);

		this_thread::sleep(seconds(2));
	}

	BOOST_CHECK(my_mgr_svr::instance()->send_close(handler_pointers[0], err_code) == 0);

}

void run_client_thread(){

	char data[16] = {0x68, 0x33, 0x00, 0x05, 0xd9, 0x8c, 0xee, 0x47, 0x16, 0x01, 0x01, 0x8f, 0x72, 0xd8, 0x0d, 0x16};
	size_t data_size = 16;

	u16 err_code = 0;
	int i = 0;
	while(i < 4){
		this_thread::sleep(seconds(5));


		my_mgr::instance()->send_data(0u, data, data_size, err_code);

		i++;
	}

	this_thread::sleep(seconds(20));
	my_mgr::instance()->send_close(0u, err_code);
};

BOOST_AUTO_TEST_CASE(t_tcp_server3){

	my_mgr_svr::create();

	thread t(run_server_thread);

	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_server server(io_service, ipv4, port,
				 server_success_insert_handler_mgr,
				 0,
				 0,									// catch_error_callback function
				 server_read_pk_header_completed_all,		// read_pk_header_complete_callback function
				 server_read_pk_full_completed_all,		// read_pk_full_complete_callback
				 server_write_pk_full_complete,
				 0,
				 server_close_completed_erase_hander_mgr
				 );

		 io_service.run();

		 t.join();

	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	my_mgr_svr::release();

}

BOOST_AUTO_TEST_CASE(t_client3){

	my_mgr::create();


	thread t1(run_client_thread);

	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_client c(io_service, ipv4, port,
				 connect_success_insert_handler_mgr,				// connet_success_callback
				 0,													// connet_error_callback
				 0,													// catch_error_callback
				 read_pk_header_completed_all,
				 read_pk_full_completed_all,
				 0,													// write_pk_full_complete_callback
				 0,
				 close_completed_erase_hander_mgr					// close_complete_callback
				 );

		 io_service.run();

		 t1.join();

	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	my_mgr::release();

}


// -------------------------- 测试链接成功，自动发送心跳包 ---------------------- //

BOOST_AUTO_TEST_CASE(t_client4){

	my_mgr::create();

	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_client c(io_service, ipv4, port,
				 connect_success_insert_handler_mgr,				// connet_success_callback
				 connet_error,										// connet_error_callback
				 catch_error,										// catch_error_callback
				 read_pk_header_completed_all,
				 0,
				 write_pk_full_completed,							// write_pk_full_complete_callback
				 0,
				 close_completed_erase_hander_mgr					// close_complete_callback
				 );

		 io_service.run();

	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	my_mgr::release();

}

// -------------------------- 测试链接成功，首先客户端发送数据包，再开始接受服务器来的数据包 ---------------------- //

int make_send_first_package(my_client::package*& pk, u16& err_code){

	char data[16] = {0x68, 0x02, 0x00, 0x05, 0xd9, 0x8c, 0xee, 0x47, 0x16, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x16};
	size_t data_size = 16;

	pk->copy(data, data_size, err_code);

	return 0;
}

BOOST_AUTO_TEST_CASE(t_client5){

	my_mgr::create();

	try{
		 boost::asio::io_service io_service;
		 string ipv4 = "127.0.0.1";
		 u16 port = 18000;
		 my_client c(io_service, ipv4, port,
				 connect_success_insert_handler_mgr,				// connet_success_callback
				 connet_error,										// connet_error_callback
				 catch_error,										// catch_error_callback
				 read_pk_header_completed_all,
				 read_pk_full_completed_all,
				 write_pk_full_completed,							// write_pk_full_complete_callback
				 0,
				 close_completed_erase_hander_mgr,					// close_complete_callback
				 make_send_first_package							// make_send_first_package_callback
				 );

		 io_service.run();

	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	my_mgr::release();
}

BOOST_AUTO_TEST_SUITE_END()


