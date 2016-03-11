/*
 * shared_memory_req_imp.h
 *
 *  Created on: 2016-1-29
 *      Author: root
 */

#ifndef PROCESS_TASK_SHARED_MEMORY_REQ_IMP_H_
#define PROCESS_TASK_SHARED_MEMORY_REQ_IMP_H_

#include "shared_memory_data.h"
#include "../../xthread/spinlock.h"
using bingo::xthread::spinlock;

#include <boost/thread.hpp>
#include <boost/static_assert.hpp>
#include "../../tss.h"

#include <boost/function.hpp>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

using namespace boost::interprocess;

namespace bingo { namespace xprocess { namespace task {

/*
 * The shared_memory_req_imp class has implemented one-way communication from sender to receiver.
 */
template<typename PARSER,
		 typename TASK_MESSAGE_DATA,
		 typename TSS = thread_tss_data
		 >
class shared_memory_req_imp{
private:

	BOOST_STATIC_ASSERT(static_check_class_inherit(TSS, thread_tss_data));
public:
	typedef function<void(TSS* tss, char*& data)> 								req_rev_callback;
	typedef function<void(TSS* tss, u16& err_code, interprocess_exception& ex)> req_rev_error_callback;
	typedef function<void(u16& err_code, interprocess_exception& ex)> 			req_snd_error_callback;

	// Construct object on the side of sender.
	shared_memory_req_imp(req_snd_error_callback& f3):
		f1_(0),
		f2_(0),
		f3_(f3),
		is_make_thread_(false){};
	// Construct object on the side of receiver.
	shared_memory_req_imp(req_rev_callback& f1, req_rev_error_callback& f2):
		f1_(f1),
		f2_(f2),
		f3_(0),
		is_make_thread_(true),
		thr_(boost::bind(&shared_memory_req_imp::svc, this))
	{}

	~shared_memory_req_imp(){
		if(is_make_thread_){

			send_exit_thread();
			thr_.join();

#ifdef BINGO_PROCESS_TASK_SHARED_MEMORY_DEBUG
			test_output("thr: exit success")
#endif
			remove();
		}
	}

	int put(TASK_MESSAGE_DATA& msg, u16& err_code){

		spinlock::scoped_lock lock(slock_);

		int counter = 0;
		while(counter < 5){

			try{
				//Create a shared memory object.
				shared_memory_object shm
				  (open_only               				//open
				  ,PARSER::shared_memory_name       	//name
				  ,read_write                			//read-write mode
				  );

				//Map the whole shared memory in this process
				mapped_region region
					 (shm                       		//What to map
					 ,read_write 						//Map it as read-write
					 );

				//Get the address of the mapped region
				void * addr       = region.get_address();

				//Obtain a pointer to the shared structure
				shared_memory_req_data<PARSER>* data = static_cast<shared_memory_req_data<PARSER>* >(addr);

	#ifdef BINGO_PROCESS_TASK_SHARED_MEMORY_DEBUG
	//		test_output("create shm success")
	#endif

				{
					 scoped_lock<interprocess_mutex> lock(data->mutex);

					 if(data->message_in){
						 if(!data->cond_full.timed_wait(lock, get_system_time() + milliseconds(500))){
							 err_code = error_process_task_send_data_fail;
							 counter++;
							 continue;
						 }
					 }



					 // Put message into shared_memory.
					 memset(&data->items[0], 0x00, PARSER::message_size);
					 memcpy(&data->items[0], msg.data.header(), msg.data.length());
					 data->type = msg.type;

					 //Mark message buffer as full
					 data->message_in = true;

					 //Notify to the other process that there is a message
					 data->cond_empty.notify_one();

					 break;
				}
			}catch(interprocess_exception &ex){
				err_code = error_process_task_send_data_fail;
				if(f3_) f3_(err_code, ex);

				counter++;
			}

		}

		// Give up after 5 order
		if(counter == 5)
			 return -1;
		else
			return 0;

	}

private:

	int remove(){

		spinlock::scoped_lock lock(slock_);

		bool suc = shared_memory_object::remove(PARSER::shared_memory_name);

#ifdef BINGO_PROCESS_TASK_SHARED_MEMORY_DEBUG
		test_output("remove() return " << suc)
#endif
		return (suc)? 0 : -1;
	}

	void send_exit_thread(){

		// Send exit thread message.
		u16 err_code = 0;
		put(exit_data_, err_code);
	}

	void svc(){

#ifdef BINGO_PROCESS_TASK_SHARED_MEMORY_DEBUG
		test_output("thr: start")
#endif

		this_thread_tss.reset(new TSS());	// initialize the thread's storage

		try{
			//Create a shared memory object.
			shared_memory_object shm
			  (open_or_create                	//create
			  ,PARSER::shared_memory_name    	//name
			  ,read_write                     	//read-write mode
			  );

			 //Set size
			 shm.truncate(sizeof(shared_memory_req_data<PARSER>));

			//Map the whole shared memory in this process
			mapped_region region
					 (shm                       //What to map
					 ,read_write 				//Map it as read-write
					 );

			//Get the address of the mapped region
			void * addr       = region.get_address();

			//Construct the shared structure in memory
			shared_memory_req_data<PARSER> * data = new (addr) shared_memory_req_data<PARSER>;

#ifdef BINGO_PROCESS_TASK_SHARED_MEMORY_DEBUG
			test_output("thr: shm create success!")
#endif

			//Print messages until the other process marks the end
			bool end_loop = false;
			do{
				scoped_lock<interprocess_mutex> lock(data->mutex);
				if(!data->message_in){
					data->cond_empty.wait(lock);
				}

				if(data->type == task_data_type_exit_thread){
					end_loop = true;
				}
				else{
					//Call rev_callback.
					char* p = &data->items[0];
					f1_((TSS*)this_thread_tss.get(), p);
					//Notify the other process that the buffer is empty
					data->message_in = false;
					data->cond_full.notify_one();
				}
			}
			while(!end_loop);
		}
		catch(interprocess_exception &ex){

			u16 err_code = error_process_task_receive_data_fail;
			f2_((TSS*)this_thread_tss.get(), err_code, ex);

		}

	}

private:
	task_exit_data<PARSER> exit_data_;
	boost::thread thr_;
	bool is_make_thread_;

	req_rev_callback f1_;
	req_rev_error_callback f2_;
	req_snd_error_callback f3_;

	spinlock slock_;
};

} } }


#endif /* PROCESS_TASK_SHARED_MEMORY_REQ_IMP_H_ */
