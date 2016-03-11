/*
 * cost_time.h
 *
 *  Created on: 2016-1-14
 *      Author: root
 */

#ifndef COST_TIME_H_
#define COST_TIME_H_

#include <boost/progress.hpp>
using namespace boost;

#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;

#define START_COST_TIMER { progress_timer tmr ;
#define END_COST_TIMER(str) cout << str <<" cost time:";};

#define SHOW_CURRENT_TIME(str) { ptime p1 = second_clock::local_time(); cout << str << " time:" << to_iso_extended_string(p1) << endl; }


#endif /* COST_TIME_H_ */
