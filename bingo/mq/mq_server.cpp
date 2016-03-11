/*
 * mq_server.cpp
 *
 *  Created on: 2016-3-2
 *      Author: root
 */

#include "mq_server.h"

using bingo::mq::mq_server;
using bingo::mq::mq_server_mgr_type;
using bingo::mq::mq_server_type;

#include "mq_package.h"

#include "../xthread/all.h"
using bingo::xthread::task::thread_task;
using bingo::xthread::task::atomic_imp_one_to_one;
using bingo::xthread::task::mutex_imp_one_to_one;

using bingo::xthread::task::task_message_data;
using bingo::xthread::task::task_exit_data;

struct mq_queue_server_tss : public thread_tss_data{
	bingo::stringt t;
};

struct mq_queue_server_parser{
	// task_message_data use
	static const int message_size;
};

const int mq_queue_server_parser::message_size = bingo::mq::MAX_SIZE_OF_MQ_PACKAGE;

typedef task_message_data<mq_queue_server_parser> mq_queue_server_data_message;
typedef task_exit_data<mq_queue_server_parser>    mq_queue_server_exit_message;

typedef thread_task<
		atomic_imp_one_to_one<mq_queue_server_data_message,
							  mq_queue_server_exit_message,
							  mq_queue_server_tss>,
		mq_queue_server_data_message,
		mq_queue_server_tss
	> mq_queue_server_task_type;
typedef bingo::singleton_v1<mq_queue_server_task_type, mq_queue_server_task_type::thr_top_callback> mq_queue_server_task;

void top(mq_queue_server_tss* tss, mq_queue_server_data_message*& message){

	char* rev_data = message->data.this_object()->message;

	string dest_name;
	dest_name.append(rev_data + bingo::mq::MAX_SIZE_OF_MQ_PACKAGE_HEADER);

#ifdef BINGO_MQ_DEBUG
	u16 data_total_size = tss->t.stream_to_short(rev_data + 1);
	char* data = rev_data + bingo::mq::MAX_SIZE_OF_MQ_PACKAGE_HEADER + bingo::mq::MAX_SIZE_OF_MQ_PACKAGE_DEST_NAME_SECTION;
	u16 data_size = data_total_size - bingo::mq::MAX_SIZE_OF_MQ_PACKAGE_DEST_NAME_SECTION;
	test_output("receive data success, dest_name:" << dest_name << ",data:" << tss->t.stream_to_string(data, data_size));
#endif

	mq_server_mgr_type::instance()->send_data(dest_name, rev_data, message->data.length());
}

mq_server::mq_server(){

	// Make mgr instance.
	mq_server_mgr_type::create();
}

mq_server::~mq_server(){

	// Free mgr instance.
	mq_server_mgr_type::release();

	// Free task thread.
	mq_queue_server_task::release();
}



int mq_server::run(string& ipv4, u16& port, string& err_what){

	// Make task thread.
	mq_queue_server_task::create(
				top 	// thread_task queue top callback
				);

	this_thread::sleep(seconds(2));

	try{
		 boost::asio::io_service io_service;
		 mq_server_type server(io_service, ipv4, port,
				boost::bind(&mq_server::accept_success_handler, 			this, _1, _2), 				// accept_success_callback
				boost::bind(&mq_server::accept_error_handler, 				this, _1),					// accept_error_callback

				boost::bind(&mq_server::catch_error_handler, 				this, _1, _2),				// catch_error_callback
				boost::bind(&mq_server::read_pk_header_complete_handler, 	this, _1, _2, _3, _4, _5),	// read_pk_header_complete_callback
				boost::bind(&mq_server::read_pk_full_complete_handler, 		this, _1, _2, _3, _4),		// read_pk_full_complete_callback
				boost::bind(&mq_server::write_pk_full_complete_handler, 	this, _1, _2, _3, _4),		// write_pk_full_complete_callback
				boost::bind(&mq_server::active_send_in_ioservice_handler, 	this, _1, _2, _3, _4),		// active_send_in_ioservice_callback
				boost::bind(&mq_server::close_complete_handler, 			this, _1, _2)				// close_complete_callback
				 );

		 io_service.run();

		 return 0;

	} catch (std::exception& e) {

		err_what = e.what();
		return -1;
	}
}



int mq_server::accept_success_handler(mq_server_type::pointer p, u16& err_code){

	mq_server_mgr_type::instance()->insert(p.get());
#ifdef BINGO_MQ_DEBUG
	test_output_with_time("insert handler success, hdr:" << p.get());
#endif

	return 0;
}

void mq_server::accept_error_handler(const system::error_code& /*ec*/){

}



void mq_server::catch_error_handler(mq_server_type::pointer p, u16& err_code){

#ifdef BINGO_MQ_DEBUG
	test_output("catch error, hdr:" << p.get() << ",err_code:" << err_code);
#endif
}



int mq_server::read_pk_header_complete_handler(mq_server_type::pointer p,
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

int mq_server::read_pk_full_complete_handler(mq_server_type::pointer p,
		char*& rev_data, size_t& rev_data_size,
		u16& err_code){

	u8 pk_type = (u8)(rev_data[0]);

	if(pk_type == mq_package_type_register_tcp_handler_name){

		if(do_register_tcp_handler_name(p, rev_data, rev_data_size, err_code) == -1 &&
				err_code != error_mq_send_register_name_has_exist){
			return -1;
		}

	}else if(pk_type == mq_package_type_data || pk_type == mq_package_type_data_request_only){

		if(do_transfer_data(p, rev_data, rev_data_size, err_code) == -1)
			return -1;

	}else if(pk_type == mq_package_type_heartjump){

		if (do_heartjump(p, rev_data, rev_data_size, err_code) == -1)
			return -1;
	}

	return 0;
}

void mq_server::write_pk_full_complete_handler(mq_server_type::pointer p,
		char*& snd_p, size_t& snd_size,
		const boost::system::error_code& ec){
#ifdef BINGO_MQ_DEBUG
		if(!ec)
			test_output("dispatch data success, data:" << p->this_tss()->t.stream_to_string(snd_p, snd_size) << ",hdr:" << p.get());
#endif
}

int mq_server::active_send_in_ioservice_handler(mq_server_type::pointer /*p*/,
		char*& /*snd_p*/, size_t& /*snd_size*/,
		u16& /*err_code*/){

	return 0;
}

void mq_server::close_complete_handler(mq_server_type::pointer p, int& ec_value){

	u16 err_code = 0;

#ifdef BINGO_MQ_DEBUG
	int res = mq_server_mgr_type::instance()->erase(p.get(), err_code);
	if(res == 0){
		test_output_with_time("erase handler success, hdr:" << p.get());
	}else{
		test_output_with_time("erase handler fail, hdr:" << p.get() << ", err_code:" << err_code);
	}
#else
	mq_server_mgr_type::instance()->erase(p.get(), err_code);
#endif
}



int mq_server::do_register_tcp_handler_name(mq_server_type::pointer p,
		char*& rev_data, size_t& rev_data_size,
		u16& err_code){

	if(rev_data_size == MAX_SIZE_OF_MQ_REGISTER_PACKAGE){

		// Check handler's name.
		if(strlen(rev_data + MAX_SIZE_OF_MQ_PACKAGE_HEADER) <= (MAX_SIZE_OF_MQ_PACKAGE_SOUR_NAME_SECTION - 1)){

			string name;
			name.append(rev_data + MAX_SIZE_OF_MQ_PACKAGE_HEADER);

			if(mq_server_mgr_type::instance()->set_handler_name(p.get(), name, err_code) == 0){

				p->set_authentication_pass();

#ifdef BINGO_MQ_DEBUG
				test_output("receive register handler's name success, name:" << name << ",hdr:" << p.get());
#endif

				return 0;
			}else{
				return -1;
			}

		}else{

			err_code = error_mq_recevie_register_name_length_more_than_max_length;
			return -1;
		}
	}else{

		err_code = error_mq_recevie_register_name_is_wrong;
		return -1;
	}
}

int mq_server::do_heartjump(mq_server_type::pointer p,
		char*& rev_data, size_t& rev_data_size,
		u16& err_code){

	if(rev_data_size == MAX_SIZE_OF_MQ_HEARTJUMP_PACKAGE){

		// Check heartjump package.
		if(memcmp(rev_data, &mq_heartjump_package::data[0], MAX_SIZE_OF_MQ_HEARTJUMP_PACKAGE) == 0){

			p->set_heartjump_datetime();

#ifdef BINGO_MQ_DEBUG
			test_output("receive heartjump success, hdr:" << p.get());
#endif

			return 0;
		}else{
			err_code = error_mq_receive_heartjump_is_wrong;
			return -1;
		}
	}else{
		err_code = error_mq_receive_heartjump_is_wrong;
		return -1;
	}
}

int mq_server::do_transfer_data(mq_server_type::pointer p,
		char*& rev_data, size_t& rev_data_size,
		u16& err_code){

	if(rev_data_size <= MAX_SIZE_OF_MQ_PACKAGE){

		u8 pk_type = (u8)(rev_data[0]);

		if(pk_type == mq_package_type_data){

			// Check length of data
			u16 data_total_size = p->this_tss()->t.stream_to_short(rev_data + 1) - MAX_SIZE_OF_MQ_PACKAGE_NAME_SECTION;
			if(data_total_size > MAX_SIZE_OF_MQ_PACKAGE_DATA_SECTION){
				err_code = error_mq_recevie_data_length_more_than_max_length;
				return -1;
			}

			// Check name.
			if(strlen(rev_data + MAX_SIZE_OF_MQ_PACKAGE_HEADER) <= (MAX_SIZE_OF_MQ_PACKAGE_DEST_NAME_SECTION - 1)){

				// Put data into task thread.
				mq_queue_server_data_message* msg = new mq_queue_server_data_message();
				if(msg->data.copy(rev_data, rev_data_size, err_code) == 0){

					if(mq_queue_server_task::instance()->put(msg, err_code) == 0){

						return 0;
					}else{
						delete msg;
						return -1;
					}
				}else{
					delete msg;
					return -1;
				}

			}else{

				err_code = error_mq_recevie_data_name_length_more_than_max_length;
				return -1;
			}

		}else{

			// Check length of data
			u16 data_total_size = p->this_tss()->t.stream_to_short(rev_data + 1) - MAX_SIZE_OF_MQ_PACKAGE_REQUEST_ONLY_NAME_SECTION;
			if(data_total_size > MAX_SIZE_OF_MQ_PACKAGE_REQUEST_ONLY_DATA_SECTION){
				err_code = error_mq_recevie_data_length_more_than_max_length;
				return -1;
			}

			// Check name.
			if(strlen(rev_data + MAX_SIZE_OF_MQ_PACKAGE_HEADER) <= (MAX_SIZE_OF_MQ_PACKAGE_DEST_NAME_SECTION - 1)){

				// Put data into task thread.
				mq_queue_server_data_message* msg = new mq_queue_server_data_message();
				if(msg->data.copy(rev_data, rev_data_size, err_code) == 0){

					if(mq_queue_server_task::instance()->put(msg, err_code) == 0){

						return 0;
					}else{
						delete msg;
						return -1;
					}
				}else{
					delete msg;
					return -1;
				}

			}else{

				err_code = error_mq_recevie_data_name_length_more_than_max_length;
				return -1;
			}
		}
	}else{

		err_code = error_mq_recevie_data_is_wrong;
		return -1;
	}
}
