/*
 * tcp_handler_manager.h
 *
 *  Created on: 2016-1-19
 *      Author: root
 */

#ifndef TCP_HANDLER_MANAGER_H_
#define TCP_HANDLER_MANAGER_H_

#include <vector>
#include <iostream>
using namespace std;

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/thread/thread.hpp>
using namespace boost;

#include "../../xthread/spinlock.h"
using bingo::xthread::spinlock;
#include "Ihandler.h"

namespace bingo { namespace internet { namespace tcp {

// tcp_handler_data: The struct save handler information.
struct tcp_handler_data{
	string name;				// Named tcp_connection.
	void* handler_pointer;		// Pointer to tcp_connection.

	tcp_handler_data(void* p):handler_pointer(p){}
};

// tcp_handler_manager's filters.
struct find_first_tcp_handler_same_pointer{
	find_first_tcp_handler_same_pointer(const void* p):p_(p){}

	bool operator()(tcp_handler_data& n){
		return (n.handler_pointer == p_)?true: false;
	}
private:
	const void* p_;
};


// tcp_handler_manager: This is a container which manage tcp_handler_data's object.
// the T is handler type.
template<class T>
class tcp_handler_manager {
public:
	tcp_handler_manager(){}

	// Insert tcp_handler pointer into the container,
	void insert(void* hdr){
#ifdef BINGO_TCP_ATOMIC_VERSION
		spinlock::scoped_lock lock(mu_);
#else
		// lock part field.
		mutex::scoped_lock lock(mu_);
#endif
		sets_.push_back(new tcp_handler_data(hdr));
	}

	// Erase tcp_handler pointer from the container,
	// return 0 if do success, otherwise return -1.
	int erase(const void* hdr, u16& err_code){
#ifdef BINGO_TCP_ATOMIC_VERSION
		spinlock::scoped_lock lock(mu_);
#else
		// lock part field.
		mutex::scoped_lock lock(mu_);
#endif

		// Find the first element which member handler_pointer is same with p.
		set_iterator iter = boost::find_if(sets_, find_first_tcp_handler_same_pointer(hdr));

		if(iter != sets_.end()){
			sets_.erase(iter);

			err_code = 0;
			return 0;
		}else{

			err_code = error_tcp_handler_mgr_element_no_exist;
			return  -1;
		}
	}

	// Send data to handler by pointer of handler in another thread, return 0 if do success,
	// otherwise return -1.
	int send_data(void* hdr, const char* data, size_t data_size, u16& err_code){
#ifdef BINGO_TCP_ATOMIC_VERSION
		spinlock::scoped_lock lock(mu_);
#else
		// lock part field.
		mutex::scoped_lock lock(mu_);
#endif

		// Find the first element which member handler_pointer is same with p.
		set_iterator iter = boost::find_if(sets_, find_first_tcp_handler_same_pointer(hdr));

		if(iter != sets_.end()){
			T* p0 = static_cast<T*>((*iter).handler_pointer);

			if(p0){
				char* d = const_cast<char*>(data);
				p0->send_data_in_thread(d, data_size);
			}
			return 0;
		}else{

			err_code = error_tcp_handler_mgr_element_no_exist;
			return  -1;
		}
	}

	// Call handler's send_close() by pointer of handler in another thread, return 0 if do success,
	// otherwise return -1.
	int send_close(void* hdr, u16& err_code){

#ifdef BINGO_TCP_ATOMIC_VERSION
		spinlock::scoped_lock lock(mu_);
#else
		// lock part field.
		mutex::scoped_lock lock(mu_);
#endif

		// Find the first element which member handler_pointer is same with p.
		set_iterator iter = boost::find_if(sets_, find_first_tcp_handler_same_pointer(hdr));

		if(iter != sets_.end()){

			T* p0 = static_cast<T*>((*iter).handler_pointer);

			if(p0){
				p0->send_close_in_thread(error_tcp_server_close_socket_because_package);
			}
			return 0;
		}else{

			err_code = error_tcp_handler_mgr_element_no_exist;
			return  -1;
		}
	}

	size_t size(){
#ifdef BINGO_TCP_ATOMIC_VERSION
		spinlock::scoped_lock lock(mu_);
#else
		// lock part field.
		mutex::scoped_lock lock(mu_);
#endif
		return sets_.size();
	}

protected:

#ifdef BINGO_TCP_ATOMIC_VERSION
	spinlock mu_;
#else
	mutex mu_;
#endif

	boost::ptr_vector<tcp_handler_data> sets_;
	typedef boost::ptr_vector<tcp_handler_data>::iterator set_iterator;
};

template<class T>
struct find_all_of_handler_because_heartjump_timeout{
	bool operator()(tcp_handler_data& n){

		T* p = static_cast<T*>(n.handler_pointer);
		if(p){
			if(p->get_authentication_pass() && p->check_heartjump_timeout()) {
				return true;
			}
		}
		return false;
	}
};

template<class T>
class tcp_handler_svr_manager : public tcp_handler_manager<T>, public IServerhandler{
public:
	tcp_handler_svr_manager() : tcp_handler_manager<T>(), IServerhandler(){}

	// Check whether tcp_handler is heartjump timeout.
	void check_heartjump(){
#ifdef BINGO_TCP_ATOMIC_VERSION
		spinlock::scoped_lock lock(this->mu_);
#else
		// lock part field.
		mutex::scoped_lock lock(this->mu_);
#endif

		foreach_(tcp_handler_data& n, this->sets_ | adaptors::filtered(find_all_of_handler_because_heartjump_timeout<T>())){
			T* p = static_cast<T*>(n.handler_pointer);
			if(p){
				p->catch_error(error_tcp_server_close_socket_because_heartjump);
				p->close_socket();
			}
		}
	}

	// Check whether tcp_handler is authentication pass.
	void check_authentication_pass(){
#ifdef BINGO_TCP_ATOMIC_VERSION
		spinlock::scoped_lock lock(this->mu_);
#else
		// lock part field.
		mutex::scoped_lock lock(this->mu_);
#endif

		foreach_(tcp_handler_data& n, this->sets_){
			T* p = static_cast<T*>(n.handler_pointer);
			if(p){
				if(!p->check_authentication_pass()){
					p->catch_error(error_tcp_server_close_socket_because_authrication_pass);
					p->close_socket();
				}
			}
		}
	}
};

template<class T>
class tcp_handler_cet_manager : public tcp_handler_manager<T>{
public:
	tcp_handler_cet_manager() : tcp_handler_manager<T>(){

	}
};

} } }


#endif /* TCP_HANDLER_MANAGER_H_ */
