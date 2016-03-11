/*
 * tss.h
 *
 *  Created on: 2016-1-28
 *      Author: root
 */

#ifndef TSS_H_
#define TSS_H_

#include <boost/thread.hpp>


namespace bingo {
	struct thread_tss_data{
		virtual ~thread_tss_data(){}
	};
}

extern boost::thread_specific_ptr<bingo::thread_tss_data> this_thread_tss;

#endif /* TSS_H_ */
