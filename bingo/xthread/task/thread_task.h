/*
 * thread_task.h
 *
 *  Created on: 2016-1-25
 *      Author: root
 */

#ifndef THREAD_TASK_H_
#define THREAD_TASK_H_

#include <boost/function.hpp>
#include <boost/static_assert.hpp>
#include "../../tss.h"

namespace bingo { namespace xthread { namespace task {

template<typename IMP,
		 typename TASK_MESSAGE_DATA,
		 typename TSS = thread_tss_data>
class thread_task{
private:

	BOOST_STATIC_ASSERT(static_check_class_inherit(TSS, thread_tss_data));
public:
	typedef function<void(TSS* tss, TASK_MESSAGE_DATA*& data)> 		thr_top_callback;
	typedef function<void(TSS* tss)> 								thr_init_callback;

	thread_task(thr_top_callback& f){
		p_ = new IMP(f);
	}

	thread_task(thr_top_callback& f, thr_init_callback& f1){
		p_ = new IMP(f, f1);
	}

	~thread_task(){
		delete p_;
	}

	int put(TASK_MESSAGE_DATA*& data, u16& err_code){
		return p_->put(data, err_code);
	}

private:
	IMP* p_;
};

} } }


#endif /* THREAD_TASK_H_ */
