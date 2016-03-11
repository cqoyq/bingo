/*
 * mq_server.h
 *
 *  Created on: 2016-3-2
 *      Author: root
 */

#ifndef MQ_SERVER_H_
#define MQ_SERVER_H_

#include "../internet/tcp/all.h"
using namespace bingo;
using namespace bingo::internet::tcp;

#include "mq_server_hgr.h"

namespace bingo { namespace mq {

class mq_server {
public:
	mq_server();
	virtual ~mq_server();

	int run(string& ipv4, u16& port, string& err_what);


private:
	int accept_success_handler(mq_server_type::pointer p, u16& err_code);
	void accept_error_handler(const system::error_code& /*ec*/);


	void catch_error_handler(mq_server_type::pointer /*p*/, u16& /*err_code*/);


	int read_pk_header_complete_handler(mq_server_type::pointer /*p*/,
			char*& /*rev_data*/, size_t& /*rev_data_size*/,
			size_t& /*remain_size*/,
			u16& /*err_code*/);

	int read_pk_full_complete_handler(mq_server_type::pointer /*p*/,
			char*& /*rev_data*/, size_t& /*rev_data_size*/,
			u16& /*err_code*/);

	void write_pk_full_complete_handler(mq_server_type::pointer /*p*/,
			char*& /*snd_p*/, size_t& /*snd_size*/,
			const boost::system::error_code& /*ec*/);

	int active_send_in_ioservice_handler(mq_server_type::pointer /*p*/,
			char*& /*snd_p*/, size_t& /*snd_size*/,
			u16& /*err_code*/);

	void close_complete_handler(mq_server_type::pointer p, int& ec_value);
private:

	int do_register_tcp_handler_name(mq_server_type::pointer p, char*& rev_data, size_t& rev_data_size,u16& err_code);
	int do_heartjump(mq_server_type::pointer p, char*& rev_data, size_t& rev_data_size, u16& err_code);
	int do_transfer_data(mq_server_type::pointer p,char*& rev_data, size_t& rev_data_size, u16& err_code);
};

} }


#endif /* MQ_SERVER_H_ */
