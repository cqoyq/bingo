/*
 * atomic_imp_one_to_one.h
 *
 *  Created on: 2016-1-26
 *      Author: root
 */

#ifndef THREAD_ATOMIC_IMP_ONE_TO_ONE_H_
#define THREAD_ATOMIC_IMP_ONE_TO_ONE_H_

#include "../../tss.h"
#include "../../xthread/task/task_data.h"

#include <boost/function.hpp>
#include <boost/atomic.hpp>
#include <boost/static_assert.hpp>

namespace bingo { namespace xthread { namespace task {

template<typename TASK_MESSAGE_DATA,
		 typename TASK_EXIT_DATA,
		 typename TSS = thread_tss_data>
class atomic_imp_one_to_one {
private:

	BOOST_STATIC_ASSERT(static_check_class_inherit(TSS, thread_tss_data));

public:
	struct node {
		TASK_MESSAGE_DATA* data;
		node * next;
	};

	typedef function<void(TSS* tss, TASK_MESSAGE_DATA*& data)> 		thr_top_callback;
	typedef function<void(TSS* tss)> 								thr_init_callback;

	atomic_imp_one_to_one(thr_top_callback& f) :
		f_(f),
		f1_(0),
		thr_(bind(&atomic_imp_one_to_one::svc, this)),
		head_(0) {

		is_thread_exit_.store(false, boost::memory_order_relaxed);
	}

	atomic_imp_one_to_one(thr_top_callback& f,
			thr_init_callback& f1) :
		f_(f),
		f1_(f1),
		thr_(bind(&atomic_imp_one_to_one::svc, this)),
		head_(0) {

		is_thread_exit_.store(false, boost::memory_order_relaxed);
	}

	~atomic_imp_one_to_one(){

		// Send message to exit thread.
		u16 err_code = 0;
		int suc = -1;
		do{
			TASK_MESSAGE_DATA* p = (TASK_MESSAGE_DATA*)(&exit_data_);
			suc = put(p, err_code);
			if(suc == -1 && err_code == error_thread_task_svc_thread_exit)
				break;
		}while(suc == -1);

		// Block until the thread exit.
		if(suc == 0){
#ifdef BINGO_THREAD_TASK_ATOMIC_IMP_DEBUG
			test_output("wait for thread exit,  ~mutex_imp_one_to_one()")
#endif
			thr_.join();
#ifdef BINGO_THREAD_TASK_ATOMIC_IMP_DEBUG
			test_output("thread exit,  ~mutex_imp_one_to_one()")
#endif
		}

	}

	int put(TASK_MESSAGE_DATA*& data, u16& err_code){

		bool thr_exit = is_thread_exit_.load(boost::memory_order_acquire);
		if(thr_exit){
			err_code = error_thread_task_svc_thread_exit;
			return -1;
		}

		node * n = new node;
		n->data = data;
		node * stale_head = head_.load(boost::memory_order_relaxed);
		do {
			n->next = stale_head;
		} while (!head_.compare_exchange_weak(stale_head, n, boost::memory_order_release));

		// notify to read data from queue.
		cond_.notify_one();

		return 0;
	}

private:
	void svc(){
		this_thread_tss.reset(new TSS());	// initialize the thread's storage

		if(f1_) f1_((TSS*)this_thread_tss.get());

		while(true){

			atomic_imp_one_to_one::node* x = pop_all();

			if(x == 0){

				bool thr_exit = is_thread_exit_.load(boost::memory_order_acquire);
				if(thr_exit)
					return;
				else{
					// lock part field.
					mutex::scoped_lock lock(mu_);

					// wait for notify.
					cond_.wait(mu_);
				}
			}

			while(x) {

				if(x->data->type == task_data_type_exit_thread){
					is_thread_exit_.store(true, boost::memory_order_relaxed);
#ifdef BINGO_THREAD_TASK_ATOMIC_IMP_DEBUG
					test_output("received task_data_type_exit_thread")
#endif
				}

				// Call top_callback method.
				if(x->data->type != task_data_type_exit_thread){
					f_((TSS*)this_thread_tss.get(), x->data);

					free_message(x->data);
				}

				atomic_imp_one_to_one::node* tmp = x;

				x = x->next;

				delete tmp;
			}
		}
	}

	void free_message(TASK_MESSAGE_DATA*& p){

		delete p;
		p = 0;
	}

	node* pop_all(void){
		node* last = pop_all_reverse(), * first = 0;
		while(last) {
			node* tmp = last;
			last = last->next;
			tmp->next = first;
			first = tmp;
		}
		return first;
	}

	// alternative interface if ordering is of no importance
	node* pop_all_reverse(void){
		return head_.exchange(0, boost::memory_order_consume);
	}

private:
	thr_top_callback f_;
	thr_init_callback f1_;

	mutex mu_;
	condition_variable_any cond_;

	// The thread call svc().
	boost::thread thr_;
	TASK_EXIT_DATA exit_data_;
	boost::atomic_bool is_thread_exit_;

	boost::atomic<node *> head_;
};

} } }

#endif /* THREAD_ATOMIC_IMP_ONE_TO_ONE_H_ */
