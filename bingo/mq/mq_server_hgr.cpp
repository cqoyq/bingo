/*
 * mq_server_hgr.cpp
 *
 *  Created on: 2016-3-2
 *      Author: root
 */

#include "mq_server_hgr.h"

#include "../pb.h"

using bingo::mq::mq_svr_handler_data;
using bingo::mq::mq_svr_handler_manager;
using bingo::mq::mq_server_parse;
using bingo::mq::mq_server_connection_type;

#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/regex.hpp>

// tcp_handler_manager's filters.
struct find_all_of_mq_handler_without_name{

	bool operator()(mq_svr_handler_data& n){
		return (n.name.length() == 0)?true: false;
	}
};

struct find_all_of_mq_handler_with_name{

	bool operator()(mq_svr_handler_data& n){
		return (n.name.length() > 0)?true: false;
	}
};

struct find_first_mq_handler_same_pointer{
	find_first_mq_handler_same_pointer(mq_server_connection_type*& p):p_(p){}

	bool operator()(mq_svr_handler_data& n){
		return (n.handler_pointer == p_)?true: false;
	}
private:
	const mq_server_connection_type* p_;
};

struct find_all_of_mq_handler_same_name{
	find_all_of_mq_handler_same_name(string& name):name_(name){}

	bool operator()(mq_svr_handler_data& n) const{
		return (n.name.compare(name_.c_str()) == 0) ? true : false;
	}
private:
	const string name_;
};

struct find_all_of_mq_handler_satisfy_name{
	find_all_of_mq_handler_satisfy_name(string& name):name_(name){}

	bool operator()(mq_svr_handler_data& n) const{
		iterator_range<string::iterator> rge;
		boost::regex rx(name_.c_str());
		rge = find_regex( n.name, rx );
		return (rge.empty()) ? false : true;
	}
private:
	const string name_;
};





mq_svr_handler_manager::mq_svr_handler_manager(): IServerhandler(){

}

mq_svr_handler_manager::~mq_svr_handler_manager(){
	sets_.clear();
}


void mq_svr_handler_manager::insert(mq_server_connection_type* p){
	spinlock::scoped_lock lock(mu_);

	sets_.push_back(new mq_svr_handler_data(p));
}

int mq_svr_handler_manager::erase(mq_server_connection_type* p, u16& err_code){
	spinlock::scoped_lock lock(mu_);

	// Find the first element which member handler_pointer is same with p.
	set_iterator iter = boost::find_if(sets_, find_first_mq_handler_same_pointer(p));

	if(iter != sets_.end()){
		sets_.erase(iter);

		err_code = 0;
		return 0;
	}else{

		err_code = error_tcp_handler_mgr_element_no_exist;
		return  -1;
	}
}

int mq_svr_handler_manager::set_handler_name(mq_server_connection_type* p, string& name, u16& err_code){
	spinlock::scoped_lock lock(mu_);

	// Check whether exist same name.
	{
		set_iterator iter = boost::find_if(sets_, find_all_of_mq_handler_same_name(name));

		if(iter != sets_.end()){

			err_code = error_mq_send_register_name_has_exist;
			return -1;
		}
	}

	// Find the first element which member handler_pointer is same with p.
	{
		set_iterator iter = boost::find_if(sets_, find_first_mq_handler_same_pointer(p));

		if(iter != sets_.end()){

			(*iter).name = name;

			err_code = 0;
			return 0;
		}else{

			err_code = error_tcp_handler_mgr_element_no_exist;
			return  -1;
		}
	}
}


void mq_svr_handler_manager::send_data(string& name, const char* data, size_t data_size){
	spinlock::scoped_lock lock(mu_);

	foreach_(mq_svr_handler_data& n, sets_ | adaptors::filtered(find_all_of_mq_handler_satisfy_name(name))){

		char* d = const_cast<char*>(data);
		n.handler_pointer->send_data_in_thread(d, data_size);
	}
}



void mq_svr_handler_manager::check_heartjump(){
	spinlock::scoped_lock lock(mu_);

	foreach_(mq_svr_handler_data& n, sets_ | adaptors::filtered(find_all_of_mq_handler_with_name())){
		if (n.handler_pointer->check_heartjump_timeout()){
			n.handler_pointer->catch_error(error_tcp_server_close_socket_because_heartjump);
			n.handler_pointer->close_socket();
		}
	}
}

void mq_svr_handler_manager::check_authentication_pass(){
	spinlock::scoped_lock lock(mu_);

	foreach_(mq_svr_handler_data& n, sets_ | adaptors::filtered(find_all_of_mq_handler_without_name())){
		if(!n.handler_pointer->check_authentication_pass()){
			n.handler_pointer->catch_error(error_tcp_server_close_socket_because_authrication_pass);
			n.handler_pointer->close_socket();
		}
	}
}
