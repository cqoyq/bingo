/*
 * tss.cpp
 *
 *  Created on: 2016-1-28
 *      Author: root
 */

#include "tss.h"

boost::thread_specific_ptr<bingo::thread_tss_data> this_thread_tss;


