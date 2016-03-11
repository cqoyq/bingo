/*
 * mutex_imp_one_to_many.h
 *
 *  Created on: 2016-1-26
 *      Author: root
 */

#ifndef THREAD_MUTEX_IMP_ONE_TO_MANY_H_
#define THREAD_MUTEX_IMP_ONE_TO_MANY_H_

#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/static_assert.hpp>
#include "../../tss.h"

namespace bingo { namespace xthread { namespace task {

template<typename TASK_MESSAGE_DATA,
		 typename TASK_EXIT_DATA,
		 typename PARSER,
		 typename TSS = thread_tss_data>
class mutex_imp_one_to_many{
private:

	BOOST_STATIC_ASSERT(static_check_class_inherit(TSS, thread_tss_data));
	BOOST_STATIC_ASSERT(static_check_u32(PARSER::thread_n));

public:
	typedef function<void(TSS* tss, TASK_MESSAGE_DATA*& data)> 		thr_top_callback;
	typedef function<void(TSS* tss)> 								thr_init_callback;

	mutex_imp_one_to_many(thr_top_callback& f):
		f_(f),
		f1_(0),
		is_thread_exit_(false){

		// Make thread_n new thread to read queue.
		for(int i = 0;i < PARSER::thread_n; i++)
			thr_group_.create_thread(bind(&mutex_imp_one_to_many::svc, this));
	}

	mutex_imp_one_to_many(thr_top_callback& f,
			thr_init_callback& f1):
		f_(f),
		f1_(f1),
		is_thread_exit_(false){

		// Make thread_n new thread to read queue.
		for(int i = 0;i < PARSER::thread_n; i++)
			thr_group_.create_thread(bind(&mutex_imp_one_to_many::svc, this));
	}

	~mutex_imp_one_to_many(){

		// Interrupt all thread.
		thr_group_.interrupt_all();

		// Wait for all thread exit.
		thr_group_.join_all();
	}

	int put(TASK_MESSAGE_DATA*& data, u16& err_code){

		{
			// lock part field.
			mutex::scoped_lock lock(mu_);

			// Check is_thread_exit, the value is true that thread exit.
			if(is_thread_exit_) {
				err_code = error_thread_task_svc_thread_exit;
				return -1;
			}

			// Check buffer whether full.
			while(is_full())
			{

				// Wait for notify, time out that return false right now.
				if(!cond_put_.timed_wait(mu_, get_system_time())){

					err_code = error_thread_task_queue_is_full;

					return -1;

				}
			}

			// Condition is satisfy, then stop to wait.
			queue_.push(data);
		}

		// notify to read data from queue.
		cond_get_.notify_all();

		return 0;
	}

private:
	// Whether buffer is full.
	bool is_full() {
		return false;
	}

	// Whether buffer is empty.
	bool is_empty() {
		return (queue_.size() ==0)?true:false;
	}

	// Thread call the method to output xhm_base_message_block from queue.
	void svc(){

		this_thread_tss.reset(new TSS());	// initialize the thread's storage

		if(f1_) f1_((TSS*)this_thread_tss.get());

#ifdef BINGO_THREAD_TASK_MUTEX_IMP_DEBUG
		std::stringstream stream;
		stream << this_thread::get_id();
		string id = stream.str();
		test_output("thr:" << id << ",create success!");
#endif
		while(true){

			TASK_MESSAGE_DATA* data_p = 0;

			{

				// lock part field.
				mutex::scoped_lock lock(mu_);

				// Check buffer whether empty.
				while(is_empty())
				{
					if(is_thread_exit_){
#ifdef BINGO_THREAD_TASK_MUTEX_IMP_DEBUG
						test_output("is_thread_exit_ is true in is_empty()")
#endif
						return;
					}
					// wait for notify.
					cond_get_.wait(mu_);
				}

				// Condition is satisfy, then stop to wait.
				// TASK_DATA top from queue.
				data_p = queue_.front();
				queue_.pop();

				if(data_p->type == task_data_type_exit_thread){
					is_thread_exit_ = true;
#ifdef BINGO_THREAD_TASK_MUTEX_IMP_DEBUG
					test_output("received task_data_type_exit_thread")
#endif
				}
			}

			if(data_p->type != task_data_type_exit_thread){
				// Call top_callback method.
				f_((TSS*)this_thread_tss.get(), data_p);

				free_message(data_p);
			}

			// notify to write data to queue.
			cond_put_.notify_all();

		}
	}

	void free_message(TASK_MESSAGE_DATA*& p){

		delete p;
		p = 0;
	}

private:
	thr_top_callback f_;
	thr_init_callback f1_;

	// The thread call svc().
	thread_group thr_group_;
	bool is_thread_exit_;

	queue<TASK_MESSAGE_DATA*> queue_;

	mutex mu_;
	condition_variable_any cond_put_;
	condition_variable_any cond_get_;

};

} } }


#endif /* THREAD_MUTEX_IMP_ONE_TO_MANY_H_ */
