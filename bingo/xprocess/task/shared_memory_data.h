/*
 * shared_memory_data.h
 *
 *  Created on: 2016-2-4
 *      Author: root
 */

#ifndef SHARED_MEMORY_DATA_H_
#define SHARED_MEMORY_DATA_H_

#include "task_data.h"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

using namespace boost::interprocess;

namespace bingo { namespace xprocess { namespace task {

template<typename PARSER>
struct shared_memory_req_data
{
   enum { LineSize = PARSER::message_size };

   shared_memory_req_data()
      :  message_in(false), type(task_data_type_data)
   {}

   //Mutex to protect access to the queue
   boost::interprocess::interprocess_mutex      mutex;

   //Condition to wait when the queue is empty
   boost::interprocess::interprocess_condition  cond_empty;

   //Condition to wait when the queue is full
   boost::interprocess::interprocess_condition  cond_full;

   //Items to fill
   char   items[LineSize];

   //Is there any message
   bool message_in;

   u8 type;
};

template<typename PARSER>
struct shared_memory_rep_data : public shared_memory_req_data<PARSER>{

};

} } }


#endif /* SHARED_MEMORY_DATA_H_ */
