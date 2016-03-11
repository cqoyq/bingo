/*
 * atomic_imp_one_to_many.h
 *
 *  Created on: 2016-1-27
 *      Author: root
 */

#ifndef THREAD_ATOMIC_IMP_ONE_TO_MANY_H_
#define THREAD_ATOMIC_IMP_ONE_TO_MANY_H_

#include <boost/function.hpp>
#include <boost/atomic.hpp>
#include <boost/static_assert.hpp>
#include "../../tss.h"

namespace bingo { namespace xthread { namespace task {

template<typename TASK_MESSAGE_DATA,
		 typename TASK_EXIT_DATA,
		 typename PARSER,
		 typename TSS = thread_tss_data>
class atomic_imp_one_to_many {
private:

	BOOST_STATIC_ASSERT(static_check_class_inherit(TSS, thread_tss_data));
	BOOST_STATIC_ASSERT(static_check_u32(PARSER::thread_n));

public:
	struct node {
		TASK_MESSAGE_DATA* data;
		node * next;
	};

	typedef function<void(TSS* tss, TASK_MESSAGE_DATA*& data)> 		thr_top_callback;
	typedef function<void(TSS* tss)> 								thr_init_callback;

	atomic_imp_one_to_many(thr_top_callback& f) :
		f_(f),
		f1_(0),
		head_(0) {

		thread_exit_flag_.store(false, boost::memory_order_relaxed);

		// Make thread_n new thread to read queue.
		for(int i = 0;i < PARSER::thread_n; i++)
			thr_group_.create_thread(bind(&atomic_imp_one_to_many::svc, this));
	}

	atomic_imp_one_to_many(thr_top_callback& f,
			thr_init_callback& f1) :
		f_(f),
		f1_(f1),
		head_(0) {

		thread_exit_flag_.store(false, boost::memory_order_relaxed);

		// Make thread_n new thread to read queue.
		for(int i = 0;i < PARSER::thread_n; i++)
			thr_group_.create_thread(bind(&atomic_imp_one_to_many::svc, this));
	}

	~atomic_imp_one_to_many(){

#ifdef BINGO_THREAD_TASK_ATOMIC_IMP_DEBUG
		test_output("is_thread_exit_ is true")
#endif
		thread_exit_flag_.store(true, boost::memory_order_release);

		// notify to read data from queue.
		cond_.notify_all();

		// Wait for all thread exit.
		thr_group_.join_all();

	}

	int put(TASK_MESSAGE_DATA*& data, u16& err_code){

		node * n = new node;
		n->data = data;
		node * stale_head = head_.load(boost::memory_order_relaxed);
		do {
			n->next = stale_head;
		} while (!head_.compare_exchange_weak(stale_head, n, boost::memory_order_release));

		// notify to read data from queue.
		cond_.notify_all();

		return 0;
	}

private:

	void svc(){

		this_thread_tss.reset(new TSS());	// initialize the thread's storage

		if(f1_) f1_((TSS*)this_thread_tss.get());

#ifdef BINGO_THREAD_TASK_ATOMIC_IMP_DEBUG
		std::stringstream stream;
		stream << this_thread::get_id();
		string id = stream.str();
		test_output("thr:" << id << ",create success!")
#endif

		while(true){

				bool thr_exit = thread_exit_flag_.load(boost::memory_order_acquire);
				if(thr_exit){
#ifdef BINGO_THREAD_TASK_ATOMIC_IMP_DEBUG
					std::stringstream stream;
					stream << this_thread::get_id();
					string id = stream.str();
					test_output("thr_exit:" << thr_exit << ",thr:" << id)
#endif
					return;
				}


			atomic_imp_one_to_many::node* x = pop_all();



			while(x) {

				// Call top_callback method.
				if(x->data->type != task_data_type_exit_thread){
					f_((TSS*)this_thread_tss.get(), x->data);

					free_message(x->data);
				}

				atomic_imp_one_to_many::node* tmp = x;

				x = x->next;

				delete tmp;
			}

			{
				// lock part field.
				mutex::scoped_lock lock(mu_);

				// wait for notify.
				cond_.wait(mu_);
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
	thread_group thr_group_;

	boost::atomic<node *> head_;
	boost::atomic_bool    thread_exit_flag_;

};

} } }


#endif /* THREAD_ATOMIC_IMP_ONE_TO_MANY_H_ */
