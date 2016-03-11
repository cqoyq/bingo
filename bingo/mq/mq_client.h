/*
 * mq_client.h
 *
 *  Created on: 2016-3-4
 *      Author: root
 */

#ifndef MQ_CLIENT_H_
#define MQ_CLIENT_H_

#include "../internet/tcp/all.h"
using namespace bingo;
using namespace bingo::internet::tcp;

#include "../xthread/all.h"
using bingo::xthread::task::thread_task;
using bingo::xthread::task::atomic_imp_one_to_one;
using bingo::xthread::task::mutex_imp_one_to_one;
using bingo::xthread::task::task_message_data;
using bingo::xthread::task::task_exit_data;

#include "mq_package.h"
#include "mq_client_hgr.h"

namespace bingo { namespace mq {

struct mq_queue_client_tss : public thread_tss_data{
	bingo::stringt t;
};

struct mq_queue_client_parser{
	// task_message_data use
	static const int message_size = MAX_SIZE_OF_MQ_PACKAGE;
};


typedef task_message_data<mq_queue_client_parser> mq_queue_client_data_message;
typedef task_exit_data<mq_queue_client_parser>    mq_queue_client_exit_message;

typedef thread_task<
		atomic_imp_one_to_one<mq_queue_client_data_message,
							  mq_queue_client_exit_message,
							  mq_queue_client_tss>,
		mq_queue_client_data_message,
		mq_queue_client_tss
	> mq_queue_client_task_type;
typedef bingo::singleton_v1<mq_queue_client_task_type, mq_queue_client_task_type::thr_top_callback> mq_queue_client_task;

class mq_client {
	typedef function<void(string& current_channel_name, char*& rev_data, size_t& rev_data_size)> 	receive_complete_callback;
public:
	mq_client();
	virtual ~mq_client();

	int run(string& server_ipv4, u16& server_port,
			receive_complete_callback f, string& channel_name, string& err_what);

private:
	void top(mq_queue_client_tss* tss, mq_queue_client_data_message*& message);

private:

	int connet_success_handler(mq_client_type::pointer /*ptr*/, u16& /*err_code*/);
	void connet_error_handler(mq_client_type::pointer /*ptr*/, int& /*retry_delay_seconds*/);

	void catch_error_handler(mq_client_type::pointer /*p*/, u16& /*err_code*/);

	int read_pk_header_complete_handler(mq_client_type::pointer /*p*/,
			char*& /*rev_data*/, size_t& /*rev_data_size*/,
			size_t& /*remain_size*/,
			u16& /*err_code*/);

	int read_pk_full_complete_handler(mq_client_type::pointer /*p*/,
			char*& /*rev_data*/, size_t& /*rev_data_size*/,
			u16& /*err_code*/);

	void write_pk_full_complete_handler(mq_client_type::pointer /*p*/,
			char*& /*snd_p*/, size_t& /*snd_size*/,
			const boost::system::error_code& /*ec*/);

	int active_send_in_ioservice_handler(mq_client_type::pointer /*p*/,
			char*& /*snd_p*/, size_t& /*snd_size*/,
			u16& /*err_code*/);

	void close_complete_handler(mq_client_type::pointer /*p*/, int& /*ec_value*/);

	int make_send_first_package_handler(mq_client_type::package*& /*pk*/, u16& /*err_code*/);

private:
	receive_complete_callback f_;
	string channel_name_;
};

} }

#endif /* MQ_CLIENT_H_ */


