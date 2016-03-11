/*
 * mq_client.cpp
 *
 *  Created on: 2016-3-4
 *      Author: root
 */

#include "mq_client.h"
#include "mq_package.h"

using bingo::mq::mq_client;
using bingo::mq::mq_client_mgr_type;
using bingo::mq::mq_client_type;
using bingo::mq::mq_package;

using bingo::mq::mq_queue_client_tss;
using bingo::mq::mq_queue_client_data_message;

void mq_client::top(mq_queue_client_tss* tss, mq_queue_client_data_message*& message){
	char* snd_data = message->data.this_object()->message;
	size_t snd_data_size = message->data.length();

	u16 err_code = 0;
#ifdef BINGO_MQ_DEBUG
	int res = mq_client_mgr_type::instance()->send_data(snd_data, snd_data_size, err_code);
	if(res == 0){
		test_output("send data success, data:" << tss->t.stream_to_string(snd_data, snd_data_size));
	}else{
		test_output("send data fail, err:" << err_code);
	}
#else
	mq_client_mgr_type::instance()->send_data(snd_data, snd_data_size, err_code);
#endif
}

mq_client::mq_client(){

	f_ = 0;

	// Make mgr instance.
	mq_client_mgr_type::create();
}

mq_client::~mq_client(){

	// Free mgr instance.
	mq_client_mgr_type::release();

	// Free task thread.
	mq_queue_client_task::release();
}

int mq_client::run(string& server_ipv4, u16& server_port, receive_complete_callback f, string& channel_name, string& err_what){

	// Save receive_complete_callback.
	f_ = f;

	// Make task thread.
	mq_queue_client_task::create(
			boost::bind(&mq_client::top, 			this, _1, _2) 	// thread_task queue top callback
				);

	this_thread::sleep(seconds(2));

	channel_name_ = channel_name;

	try{
		 boost::asio::io_service io_service;
		 mq_client_type c(io_service, server_ipv4, server_port,
				 boost::bind(&mq_client::connet_success_handler, 			this, _1, _2), 				// connet_success_callback
				 boost::bind(&mq_client::connet_error_handler, 				this, _1, _2),				// connet_error_callback

				 boost::bind(&mq_client::catch_error_handler, 				this, _1, _2),				// catch_error_callback
				 boost::bind(&mq_client::read_pk_header_complete_handler, 	this, _1, _2, _3, _4, _5),	// read_pk_header_complete_callback
				 boost::bind(&mq_client::read_pk_full_complete_handler, 	this, _1, _2, _3, _4),		// read_pk_full_complete_callback
				 boost::bind(&mq_client::write_pk_full_complete_handler, 	this, _1, _2, _3, _4),		// write_pk_full_complete_callback
				 boost::bind(&mq_client::active_send_in_ioservice_handler, 	this, _1, _2, _3, _4),		// active_send_in_ioservice_callback
				 boost::bind(&mq_client::close_complete_handler, 			this, _1, _2),				// close_complete_callback
		 	 	 boost::bind(&mq_client::make_send_first_package_handler,	this, _1, _2)				// make_send_first_package_callback
				 );

		 io_service.run();

		 return 0;

	} catch (std::exception& e) {
		err_what = e.what();
		return -1;
	}

}



int mq_client::connet_success_handler(mq_client_type::pointer p, u16& err_code){
	mq_client_mgr_type::instance()->insert(p.get());
#ifdef BINGO_MQ_DEBUG
	test_output("insert handler success, hdr:" << p.get());
#endif

	return 0;
}

void mq_client::connet_error_handler(mq_client_type::pointer p, int& retry_delay_seconds){
#ifdef BINGO_MQ_DEBUG
	cout << "hdr:" << p.get() << ",reconnect after " << retry_delay_seconds << " seconds." << endl;
#endif
}

void mq_client::catch_error_handler(mq_client_type::pointer p, u16& err_code){
#ifdef BINGO_MQ_DEBUG
	test_output("catch error, hdr:" << p.get() << ",err_code:" << err_code);
#endif
}

int mq_client::read_pk_header_complete_handler(mq_client_type::pointer p,
		char*& rev_data, size_t& rev_data_size,
		size_t& remain_size,
		u16& err_code){

	u8 pk_type = (u8)(rev_data[0]);
	if(pk_type > (mq_package_type_max_placeholder - 1)){
		err_code = error_tcp_package_header_is_wrong;
		return -1;
	}

	remain_size = p->this_tss()->t.stream_to_short(rev_data + 1);
	return 0;
}

int mq_client::read_pk_full_complete_handler(mq_client_type::pointer p,
		char*& rev_data, size_t& rev_data_size,
		u16& err_code){
	if(f_)
		f_(channel_name_, rev_data, rev_data_size);
	return 0;
}

void mq_client::write_pk_full_complete_handler(mq_client_type::pointer p,
		char*& snd_p, size_t& snd_size,
		const boost::system::error_code& ec){
#ifdef BINGO_MQ_DEBUG
	if(!ec)
		test_output("send data completed, data:" << p->this_tss()->t.stream_to_string(snd_p, snd_size) << ",hdr:" << p.get());
#endif
}

int mq_client::active_send_in_ioservice_handler(mq_client_type::pointer /*p*/,
		char*& /*snd_p*/, size_t& /*snd_size*/,
		u16& /*err_code*/){

	return 0;
}

void mq_client::close_complete_handler(mq_client_type::pointer p, int& ec_value){

	mq_client_mgr_type::instance()->erase();
#ifdef BINGO_MQ_DEBUG
	test_output("erase handler success, hdr:" << p.get());
#endif
}

int mq_client::make_send_first_package_handler(mq_client_type::package*& pk, u16& err_code){

	// Make register' name package.
	return make_mq_register_pk(*pk, channel_name_, err_code);
}
