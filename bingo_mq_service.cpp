/*
 * bingo_mq_server.cpp
 *
 *  Created on: 2016-3-6
 *      Author: root
 */

#include <boost/test/unit_test.hpp>

#include <iostream>
using namespace std;

#include "cost_time.h"

#define BINGO_TCP_ATOMIC_VERSION
#include "bingo/mq/server_all.h"
#include "bingo/mq/client_all.h"
using namespace bingo;
using namespace bingo::mq;

BOOST_AUTO_TEST_SUITE(bingo_mq_server)

void run_server(){

	string err_what;
	string ip = "127.0.0.1";
	u16 port = 18000;

	mq_server svr;
	if(svr.run(ip, port, err_what) == -1){
		cout << "create server fail, err:" << err_what << endl;
	}
}

vector<int> amount_of_client_receive_message(5);

void client_receive_complete(string& channel_name, char*& rev_data, size_t& rev_data_size){

	stringt t;
	cout << "client name:" << channel_name << ",receive data:" << t.stream_to_string(rev_data, rev_data_size) << endl;

	int offset = MAX_SIZE_OF_MQ_PACKAGE_HEADER + MAX_SIZE_OF_MQ_PACKAGE_NAME_SECTION;

	if(channel_name.compare("rev1")==0 && rev_data[offset+1] == 0x0A){
		amount_of_client_receive_message[0]++;
		cout << "amount_of_client_receive_message[0]:" << amount_of_client_receive_message[0] << endl;
	}
	if(channel_name.compare("rev2")==0 && rev_data[offset+1] == 0x0A){
		amount_of_client_receive_message[1]++;
		cout << "amount_of_client_receive_message[1]:" << amount_of_client_receive_message[1] << endl;
	}
	if(channel_name.compare("rev3")==0 && rev_data[offset+1] == 0x0B){
		amount_of_client_receive_message[2]++;
		cout << "amount_of_client_receive_message[2]:" << amount_of_client_receive_message[2] << endl;
	}


}

void run_client(string& name){

	string err_what;
	string ip = "127.0.0.1";
	u16 port = 18000;

	mq_client cet;
	if(cet.run(ip, port, client_receive_complete, name, err_what) == -1){
		cout << "create client fail, name:" << name << ",err:" << err_what << endl;
	}
}

BOOST_AUTO_TEST_CASE(t_server){
	run_server();
}

// ----------------- 测试sever身份验证没有通过，自动断开客户端 ----------------------- //

struct my_tss_data : public thread_tss_data{
	bingo::stringt t;
};

struct my_parse{
	static int retry_delay_seconds_when_connec;				// When Client connect fail, don't reconnect if the value is 0.
	static int max_retry_delay_seconds_when_connec;

	static const size_t header_size;						// Parse size of package's header.
	static int max_interval_seconds_when_send_heartjump;	// Client don't send heartjump if the value is 0.
};
const size_t my_parse::header_size = 4;
int my_parse::retry_delay_seconds_when_connec = 3;
int my_parse::max_retry_delay_seconds_when_connec = 60;
int my_parse::max_interval_seconds_when_send_heartjump = 0;

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

void connet_error(my_client::pointer ptr, int& retry_delay_seconds){
	SHOW_CURRENT_TIME("connet_error");
	cout << "hander:" << ptr.get() << ",reconnect after " << retry_delay_seconds << " seconds." << endl;
};

BOOST_AUTO_TEST_CASE(t1){

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

	// Server-output:
	//	insert handler success, hdr:0x1dcb390, time:2016-03-06T23:36:09
	//	catch error, hdr:0x1dcb390,err_code:267								// error_tcp_server_close_socket_because_authrication_pass
	//	erase handler success, hdr:0x1dcb390, time:2016-03-06T23:36:23

	//	insert handler success, hdr:0x1dcbac0, time:2016-03-06T23:36:23
	//	catch error, hdr:0x1dcbac0,err_code:267
	//	erase handler success, hdr:0x1dcbac0, time:2016-03-06T23:36:38

	//	insert handler success, hdr:0x1dcb390, time:2016-03-06T23:36:38
	//	catch error, hdr:0x1dcb390,err_code:267
	//	erase handler success, hdr:0x1dcb390, time:2016-03-06T23:36:53
}

// ----------------- 测试sever身份验证通过，但心跳验证失败，自动断开客户端 ----------------------- //

BOOST_AUTO_TEST_CASE(t2){

	mq_client_parse::max_interval_seconds_when_send_heartjump = 0;

	string name("cet1");
	run_client(name);

	// Server-output:
	//	insert handler success, hdr:0x1dcba00, time:2016-03-07T00:02:34
	//	receive register handler's name success, name:cet1,hdr:0x1dcba00
	//	catch error, hdr:0x1dcba00,err_code:266								// error_tcp_server_close_socket_because_heartjump
	//	erase handler success, hdr:0x1dcba00, time:2016-03-07T00:02:38

	//	insert handler success, hdr:0x1dcbf40, time:2016-03-07T00:02:38
	//	receive register handler's name success, name:cet1,hdr:0x1dcbf40
	//	catch error, hdr:0x1dcbf40,err_code:266
	//	erase handler success, hdr:0x1dcbf40, time:2016-03-07T00:02:48

	//	insert handler success, hdr:0x1dcba00, time:2016-03-07T00:02:48
	//	receive register handler's name success, name:cet1,hdr:0x1dcba00
	//	catch error, hdr:0x1dcba00,err_code:266
	//	erase handler success, hdr:0x1dcba00, time:2016-03-07T00:02:58
}

// ----------------- 测试sever分发消息，无接受方，抛弃消息 ----------------------- //

int max_send_amount = 5;

void client_send_data(string& dest_name, string& sour_name){
	this_thread::sleep(seconds(5));

	char data[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
	if(dest_name.compare("rev[12]") ==0)
		data[1] = 0x0A;
	if(dest_name.compare("rev3") ==0)
		data[1] = 0x0B;

	int i=0;
	while(i< max_send_amount){
		u16 err_code = 0;
		mem_guard<mq_package> pk;

		if(make_mq_data_pk(pk, dest_name, sour_name, &data[0], 5, err_code) == 0){

			mq_queue_client_data_message* message = new mq_queue_client_data_message();
			if(message->data.copy(pk.header(), pk.length(), err_code) == -1){
				cout << "mq_queue_client_data_message create fail!,err_code:" << err_code << endl;
				delete message;
			}else{

				// send data
				if(mq_queue_client_task::instance()->put(message, err_code) == -1){
					cout << "put() fail!,err_code:" << err_code << endl;
				}
			}

		}else{
			cout << "make_mq_data_pk fail!,err_code:" << err_code << endl;
		}

//		this_thread::sleep(seconds(2));
		this_thread::sleep(milliseconds(10));
		i++;
	}
}

BOOST_AUTO_TEST_CASE(t3){

	string dest_name1 = "bcd";
	string sour_name = "abc";
	thread t(client_send_data, ref(dest_name1), ref(sour_name));

	string name("cet1");
	run_client(name);

	t.join();

	// Server-output:
	//	insert handler success, hdr:0x14a97c0, time:2016-03-07T11:19:50
	//	receive register handler's name success, name:cet1,hdr:0x14a97c0
	//	receive data success, name:bac,data:01 02 03 04 05				// not dispatch
	//	receive heartjump success, hdr:0x14a97c0
	//	receive data success, name:bac,data:01 02 03 04 05
	//	receive data success, name:bac,data:01 02 03 04 05
	//	receive data success, name:bac,data:01 02 03 04 05
	//	receive heartjump success, hdr:0x14a97c0
	//	receive data success, name:bac,data:01 02 03 04 05

}

// ----------------- 测试sever分发消息单播 ----------------------- //

BOOST_AUTO_TEST_CASE(t4_rev1){

	string name("rev1");
	run_client(name);

	// Server-output:
//	insert handler success, hdr:0x1124e10, time:2016-03-07T12:12:05
//	receive register handler's name success, name:rev1,hdr:0x1124e10			// register 'rev1'
//	insert handler success, hdr:0x1125540, time:2016-03-07T12:12:08
//	receive register handler's name success, name:rev2,hdr:0x1125540			// register 'rev2'
//	receive heartjump success, hdr:0x1124e10
//	receive heartjump success, hdr:0x1125540
//	receive heartjump success, hdr:0x1124e10
//	receive heartjump success, hdr:0x1125540
//	receive heartjump success, hdr:0x1124e10
//	receive heartjump success, hdr:0x1125540
//	insert handler success, hdr:0x1125be0, time:2016-03-07T12:12:23
//	receive register handler's name success, name:snd,hdr:0x1125be0				// register 'snd'
//	receive heartjump success, hdr:0x1124e10
//	receive data success, name:rev1,data:01 02 03 04 05
//	dispatch data success, data:02 45 00 72 65 76 31 00 00 00 00...00 00 01 02 03 04 05 ,hdr:0x1124e10	// dispatch to 'rev1'
//	receive heartjump success, hdr:0x1125540
//	receive heartjump success, hdr:0x1125be0
//	receive data success, name:rev1,data:01 02 03 04 05
//	dispatch data success, data:02 45 00 72 65 76 31 00 00 00 00... 00 00 01 02 03 04 05 ,hdr:0x1124e10 // dispatch to 'rev1'
//	receive heartjump success, hdr:0x1124e10
//	receive data success, name:rev1,data:01 02 03 04 05
//	dispatch data success, data:02 45 00 72 65 76 31 00 00 00 00... 00 00 01 02 03 04 05 ,hdr:0x1124e10	// dispatch to 'rev1'
//	receive data success, name:rev1,data:01 02 03 04 05
//	dispatch data success, data:02 45 00 72 65 76 31 00 00 00 00... 00 00 01 02 03 04 05 ,hdr:0x1124e10	// dispatch to 'rev1'
//	receive heartjump success, hdr:0x1125540
//	receive heartjump success, hdr:0x1125be0
//	receive data success, name:rev1,data:01 02 03 04 05
//	dispatch data success, data:02 45 00 72 65 76 31 00 00 00 00... 00 00 01 02 03 04 05 ,hdr:0x1124e10	// dispatch to 'rev1'
}

BOOST_AUTO_TEST_CASE(t4_rev2){

	string name("rev2");
	run_client(name);
}

BOOST_AUTO_TEST_CASE(t4_snd){

	string dest_name = "rev1";					// Send data to 'rev1'
	string sour_name = "snd";
	thread t(client_send_data, ref(dest_name), ref(sour_name));

	string name1 = "snd";
	run_client(name1);

	t.join();
}

// ----------------- 测试sever分发消息组播 ----------------------- //

BOOST_AUTO_TEST_CASE(t5_rev1){

	string name("rev1");
	run_client(name);
}

BOOST_AUTO_TEST_CASE(t5_rev2){

	string name("rev2");
	run_client(name);
}

BOOST_AUTO_TEST_CASE(t5_rev3){

	string name("rev3");
	run_client(name);
}

BOOST_AUTO_TEST_CASE(t5_snd){

	string dest_name = "rev[12]";					// Send data to 'rev1' and 'rev2'
	string sour_name = "snd";
	thread t(client_send_data, ref(dest_name),ref(sour_name));

	string name = "snd";
	run_client(name);

	t.join();

	// Server-output:
//	insert handler success, hdr:0x1126280, time:2016-03-07T12:21:54
//	receive register handler's name success, name:rev1,hdr:0x1126280
//	insert handler success, hdr:0x1124e10, time:2016-03-07T12:22:03
//	receive register handler's name success, name:rev2,hdr:0x1124e10
//	insert handler success, hdr:0x1125b20, time:2016-03-07T12:22:21
//	receive register handler's name success, name:rev3,hdr:0x1125b20
//	insert handler success, hdr:0x1126bb0, time:2016-03-07T12:22:31
//	receive register handler's name success, name:snd,hdr:0x1126bb0
//	receive data success, name:rev[12],data:01 02 03 04 05
//	dispatch data success, data:02 45 00 72 65 76 5b 31 32 5d 00 00... 00 00 01 02 03 04 05 ,hdr:0x1126280		// dispatch to 'rev1'
//	dispatch data success, data:02 45 00 72 65 76 5b 31 32 5d 00 00... 00 00 01 02 03 04 05 ,hdr:0x1124e10		// dispatch to 'rev2'
//	receive heartjump success, hdr:0x1126280
//	receive data success, name:rev[12],data:01 02 03 04 05
//	dispatch data success, data:02 45 00 72 65 76 5b 31 32 5d 00 00... 00 00 01 02 03 04 05 ,hdr:0x1126280		// dispatch to 'rev1'
//	dispatch data success, data:02 45 00 72 65 76 5b 31 32 5d 00 00... 00 00 01 02 03 04 05 ,hdr:0x1124e10		// dispatch to 'rev2'
//	receive heartjump success, hdr:0x1126bb0
//	receive heartjump success, hdr:0x1125b20
//	receive data success, name:rev[12],data:01 02 03 04 05
//	dispatch data success, data:02 45 00 72 65 76 5b 31 32 5d 00 00... 00 00 01 02 03 04 05 ,hdr:0x1126280
//	dispatch data success, data:02 45 00 72 65 76 5b 31 32 5d 00 00... 00 00 01 02 03 04 05 ,hdr:0x1124e10
//	receive heartjump success, hdr:0x1124e10
//	receive data success, name:rev[12],data:01 02 03 04 05
//	dispatch data success, data:02 45 00 72 65 76 5b 31 32 5d 00 00... 00 00 01 02 03 04 05 ,hdr:0x1126280
//	dispatch data success, data:02 45 00 72 65 76 5b 31 32 5d 00 00... 00 00 01 02 03 04 05 ,hdr:0x1124e10
//	receive heartjump success, hdr:0x1126280
//	receive heartjump success, hdr:0x1126bb0
//	receive heartjump success, hdr:0x1125b20
//	receive data success, name:rev[12],data:01 02 03 04 05
//	dispatch data success, data:02 45 00 72 65 76 5b 31 32 5d 00 00... 00 00 01 02 03 04 05 ,hdr:0x1126280
//	dispatch data success, data:02 45 00 72 65 76 5b 31 32 5d 00 00... 00 00 01 02 03 04 05 ,hdr:0x1124e10
}

// ----------------- 测试sever分发消息性能 ----------------------- //

void client_receive_complete6(string& channel_name, char*& rev_data, size_t& rev_data_size){

	stringt t;
//	cout << "client name:" << channel_name << ",receive data:" << t.stream_to_string(rev_data, rev_data_size) << endl;

	int offset = 0;
	if(channel_name.compare("rev1")==0 || channel_name.compare("rev2")==0)
		offset = MAX_SIZE_OF_MQ_PACKAGE_HEADER + MAX_SIZE_OF_MQ_PACKAGE_NAME_SECTION;
	else
		offset = MAX_SIZE_OF_MQ_PACKAGE_HEADER + MAX_SIZE_OF_MQ_PACKAGE_REQUEST_ONLY_NAME_SECTION;

	if(channel_name.compare("rev1")==0 && rev_data[offset+1] == 0x0A){
		amount_of_client_receive_message[0]++;
		cout << "amount_of_client_receive_message[0]:" << amount_of_client_receive_message[0] << endl;
	}
	if(channel_name.compare("rev2")==0 && rev_data[offset+1] == 0x0A){
		amount_of_client_receive_message[1]++;
		cout << "amount_of_client_receive_message[1]:" << amount_of_client_receive_message[1] << endl;
	}
	if(channel_name.compare("rev3")==0 && rev_data[offset+1] == 0x0B){
		amount_of_client_receive_message[2]++;
		cout << "amount_of_client_receive_message[2]:" << amount_of_client_receive_message[2] << endl;
	}


}

void run_client6(string& name){

	string err_what;
	string ip = "127.0.0.1";
	u16 port = 18000;

	mq_client cet;
	if(cet.run(ip, port, client_receive_complete6, name, err_what) == -1){
		cout << "create client fail, name:" << name << ",err:" << err_what << endl;
	}
}

void client_send_data6(string& dest_name, string& sour_name){
	this_thread::sleep(seconds(5));

	char data[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
	if(dest_name.compare("rev[12]") ==0)
		data[1] = 0x0A;
	if(dest_name.compare("rev3") ==0)
		data[1] = 0x0B;

	int i=0;
	while(i< max_send_amount){
		u16 err_code = 0;
		mem_guard<mq_package> pk;

		if(dest_name.compare("rev[12]") ==0){

			if(make_mq_data_pk(pk, dest_name, sour_name, &data[0], 5, err_code) == 0){

				mq_queue_client_data_message* message = new mq_queue_client_data_message();
				if(message->data.copy(pk.header(), pk.length(), err_code) == -1){
					cout << "mq_queue_client_data_message create fail!,err_code:" << err_code << endl;
					delete message;
				}else{

					// send data
					if(mq_queue_client_task::instance()->put(message, err_code) == -1){
						cout << "put() fail!,err_code:" << err_code << endl;
					}
				}

			}else{
				cout << "make_mq_data_pk fail!,err_code:" << err_code << endl;
			}
		}

		if(dest_name.compare("rev3") ==0){
			if(make_mq_data_request_only_pk(pk, dest_name, &data[0], 5, err_code) == 0){

				mq_queue_client_data_message* message = new mq_queue_client_data_message();
				if(message->data.copy(pk.header(), pk.length(), err_code) == -1){
					cout << "mq_queue_client_data_message create fail!,err_code:" << err_code << endl;
					delete message;
				}else{

					// send data
					if(mq_queue_client_task::instance()->put(message, err_code) == -1){
						cout << "put() fail!,err_code:" << err_code << endl;
					}
				}

			}else{
				cout << "make_mq_data_pk fail!,err_code:" << err_code << endl;
			}
		}

//		this_thread::sleep(seconds(2));
		this_thread::sleep(milliseconds(5));
		i++;
	}
}

BOOST_AUTO_TEST_CASE(t6_rev1){

	string name("rev1");
	run_client6(name);
}

BOOST_AUTO_TEST_CASE(t6_rev2){

	string name("rev2");
	run_client6(name);
}

BOOST_AUTO_TEST_CASE(t6_rev3){

	string name("rev3");
	run_client6(name);
}

BOOST_AUTO_TEST_CASE(t6_snd){

	max_send_amount = 400000;

	string sour_name = "snd";
	string name1 = "rev[12]";					// send data to channel 'rev1' and 'rev2'
	thread t1(client_send_data6, ref(name1),ref(sour_name));

	string name2 = "rev3";						// send data to channel 'rev3'
	thread t2(client_send_data6, ref(name2),ref(sour_name));

	string name = "snd";
	run_client6(name);

	t1.join();
//	t2.join();
}

// ----------------- 测试sever注册cannel_name重复 ----------------------- //

BOOST_AUTO_TEST_CASE(t7_rev1){

	string name("rev1");
	run_client(name);
}

BOOST_AUTO_TEST_CASE(t7_rev2){

	string name("rev1");
	run_client(name);
}

BOOST_AUTO_TEST_SUITE_END()


