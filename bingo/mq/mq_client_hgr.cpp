/*
 * mq_client_hgr.cpp
 *
 *  Created on: 2016-3-4
 *      Author: root
 */

#include "mq_client_hgr.h"

using bingo::mq::mq_cet_handler_manager;
using bingo::mq::mq_client_parse;

int mq_client_parse::retry_delay_seconds_when_connec = 3;
int mq_client_parse::max_retry_delay_seconds_when_connec = 60;
int mq_client_parse::max_interval_seconds_when_send_heartjump = 5;



mq_cet_handler_manager::mq_cet_handler_manager() {
	// TODO Auto-generated constructor stub
	handler_pointer_ = 0;
}

mq_cet_handler_manager::~mq_cet_handler_manager() {
	// TODO Auto-generated destructor stub
}


void mq_cet_handler_manager::insert(mq_client_connection_type* p){
	spinlock::scoped_lock lock(mu_);

	handler_pointer_ = p;
}

void mq_cet_handler_manager::erase(){
	spinlock::scoped_lock lock(mu_);

	handler_pointer_ = 0;
}

int mq_cet_handler_manager::send_data(const char* data, size_t data_size, u16& err_code){
	spinlock::scoped_lock lock(mu_);

	if(handler_pointer_){

		char* d = const_cast<char*>(data);
		handler_pointer_->send_data_in_thread(d, data_size);

		return 0;
	}else{
		err_code = error_mq_send_data_fail_because_server_go_away;
		return -1;
	}
}
