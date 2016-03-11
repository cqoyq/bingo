/*
 * pool.h
 *
 *  Created on: 2016-2-19
 *      Author: root
 */

#ifndef PERSISTENT_MYSQL_POOL_H_
#define PERSISTENT_MYSQL_POOL_H_

#include <vector>
using namespace std;

#include <boost/range/algorithm.hpp>
#include <boost/thread.hpp>
using namespace boost;

#include "handler.h"

namespace bingo { namespace persistent { namespace mysql  {

#pragma pack(1)
struct handler_data{
	bool is_used;
	void* handler_pointer;
	handler_data():is_used(false), handler_pointer(0x00){}
};
#pragma pack()


struct find_first_handler_is_no_use{
	bool operator()(handler_data*& n){
		return (!n->is_used)?true: false;
	}
};

struct find_first_handler_as_same_pointer{
	find_first_handler_as_same_pointer(void* p):p_(p){}

	bool operator()(handler_data*& n){
		return (n->handler_pointer == p_)?true: false;
	}
private:
	const void* p_;
};

template<typename PARSER>
class pool {
public:
	pool(u32& container_size){

		// Make thread_n new thread to read queue.
		for(u32 i = 0;i < container_size; i++){
			thr_group_.create_thread(bind(&pool<PARSER>::svc, this));
		}
	}
	virtual ~pool(){
		vector<handler_data*>::iterator iter = ar_.begin();
		while(iter != ar_.end()){

			handler<PARSER>* hdr = static_cast<handler<PARSER>* >((*(iter))->handler_pointer);
			hdr->stop_heartjump();

			delete *(iter);
			iter++;
		}

		ar_.clear();

		thr_group_.join_all();
	}

	handler<PARSER>* top(){
		spinlock::scoped_lock lock(slock_);

		// Find the first element which member is_use is false.
		vector<handler_data*>::iterator iter = boost::find_if(this->ar_ , find_first_handler_is_no_use());
		if(iter != this->ar_.end()){
			handler<PARSER>* hdr = static_cast<handler<PARSER>* >((*(iter))->handler_pointer);
			(*(iter))->is_used = true;

#ifdef BINGO_PERSISTENT_MYSQL_DEBUG
			test_output("thr:" << this_thread::get_id() << ",top hdr:" << hdr)
#endif

			return hdr;
		}else{
			return 0;
		}
	}

	void push(handler<PARSER>*& pointer){
		spinlock::scoped_lock lock(slock_);

		vector<handler_data*>::iterator iter = boost::find_if(this->ar_ , find_first_handler_as_same_pointer(pointer));
		if(iter != this->ar_.end()){

			(*(iter))->is_used = false;

#ifdef BINGO_PERSISTENT_MYSQL_DEBUG
			test_output("thr:" << this_thread::get_id() << ",push hdr:" << pointer)
#endif
		}
	}

private:
	void svc(){
		string err;

		handler<PARSER> hdr;
		if(hdr.create(err) == 0){

#ifdef BINGO_PERSISTENT_MYSQL_DEBUG
			test_output("thr:" << this_thread::get_id() << ",make hdr:" << &hdr)
#endif

			handler_data* data = new handler_data();
			data->handler_pointer = &hdr;
			push(data);

			hdr.start_heartjump(); // Start heart-jump io_service
		}

#ifdef BINGO_PERSISTENT_MYSQL_DEBUG
		test_output("thr:" << this_thread::get_id() << ",io_service thread exit!")
#endif
	}

	void push(handler_data*& hdr){

		spinlock::scoped_lock lock(slock_);
		ar_.push_back(hdr);
	}

private:
	vector<handler_data*> ar_;

	// The thread call svc().
	thread_group thr_group_;

	spinlock slock_;
};

} } }

#endif /* PERSISTENT_MYSQL_POOL_H_ */
