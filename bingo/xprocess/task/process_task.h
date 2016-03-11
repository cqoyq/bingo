/*
 * process_task.h
 *
 *  Created on: 2016-1-28
 *      Author: root
 */

#ifndef PROCESS_TASK_H_
#define PROCESS_TASK_H_

#include "../../tss.h"
using bingo::thread_tss_data;

#include <boost/function.hpp>
#include <boost/static_assert.hpp>
#include <boost/interprocess/exceptions.hpp>
using namespace boost::interprocess;

namespace bingo { namespace xprocess { namespace task {

template<typename IMP,
		 typename TASK_MESSAGE_DATA,
		 typename TSS = thread_tss_data
		 >
class process_task_req{
private:

	BOOST_STATIC_ASSERT(static_check_class_inherit(TSS, thread_tss_data));
public:
	typedef function<void(TSS* tss, char*& data)> 								req_rev_callback;
	typedef function<void(TSS* tss, u16& err_code, interprocess_exception& ex)> req_rev_error_callback;
	typedef function<void(u16& err_code, interprocess_exception& ex)> 			req_snd_error_callback;


	// Construct request send object.
	process_task_req(req_snd_error_callback& f3){
		p_ = new IMP(f3);
	}

	// Construct request receive object.
	process_task_req(req_rev_callback& f1,
			req_rev_error_callback& f2){
		p_ = new IMP(f1, f2);
	}

	~process_task_req(){
		delete p_;
	}

	int put(TASK_MESSAGE_DATA& msg, u16& err_code){
		return p_->put(msg, err_code);
	}

private:
	IMP* p_;
};

template<typename IMP,
		 typename TASK_MESSAGE_DATA,
		 typename TSS = thread_tss_data
		 >
class process_task_req_and_rep{
private:

	BOOST_STATIC_ASSERT(static_check_class_inherit(TSS, thread_tss_data));
public:
	typedef function<void(TSS* tss, char*& data, TASK_MESSAGE_DATA& rep_data)> 	req_rev_callback;
	typedef function<void(TSS* tss, u16& err_code, interprocess_exception& ex)> req_rev_error_callback;
	typedef function<void(u16& err_code, interprocess_exception& ex)> 			req_snd_error_callback;

	typedef function<void(TSS* tss, char*& data)> 									rep_rev_callback;
	typedef function<void(TSS* tss, u16& err_code, interprocess_exception& ex)> 	rep_rev_error_callback;
	typedef function<void(u16& err_code, interprocess_exception& ex)> 				rep_snd_error_callback;

	// Construct request send object.
	process_task_req_and_rep(req_snd_error_callback& f3, rep_rev_callback& f11, rep_rev_error_callback& f12){
		p_ = new IMP(f3, f11, f12);
	}

	// Construct request receive object.
	process_task_req_and_rep(req_rev_callback& f1, req_rev_error_callback& f2, rep_snd_error_callback& f13){
		p_ = new IMP(f1, f2, f13);
	}

	~process_task_req_and_rep(){
		delete p_;
	}

	int put(TASK_MESSAGE_DATA& msg, u16& err_code){
		return p_->put(msg, err_code);
	}

private:
	IMP* p_;
};

} } }

#endif /* PROCESS_TASK_H_ */
