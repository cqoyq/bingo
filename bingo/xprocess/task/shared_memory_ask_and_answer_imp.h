/*
 * shared_memory_ask_and_answer_imp.h
 *
 *  Created on: 2016-3-9
 *      Author: root
 */

#ifndef PROCESS_TASK_SHARED_MEMORY_ASK_AND_ANSWER_IMP_H_
#define PROCESS_TASK_SHARED_MEMORY_ASK_AND_ANSWER_IMP_H_

#include <boost/static_assert.hpp>
#include "../../tss.h"

#include <boost/function.hpp>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
using namespace boost::interprocess;

#include <boost/coroutine/all.hpp>
using namespace boost::coroutines;

namespace bingo { namespace xprocess { namespace task {

template<typename TASK_MESSAGE_DATA>
struct ask_and_answer_data
{
	function<void(TASK_MESSAGE_DATA& req_data)> 			req_snd_callback;
	function<int(TASK_MESSAGE_DATA& msg, u16& err_code)>	req_put_callback;
	function<void(TASK_MESSAGE_DATA& rep_data)>				rep_rev_callback;
	TASK_MESSAGE_DATA rep_data;
	bool is_receive_data;
};

template<typename PARSER,
		 typename TASK_MESSAGE_DATA,
		 typename TSS = thread_tss_data
		 >
class shared_memory_ask_and_answer_imp{
private:

	BOOST_STATIC_ASSERT(static_check_class_inherit(TSS, thread_tss_data));
public:
	typedef function<void(TSS* tss, char*& data, TASK_MESSAGE_DATA& rep_data)> 	req_rev_callback;
	typedef function<void(TSS* tss, u16& err_code, interprocess_exception& ex)> req_rev_error_callback;
	typedef function<void(TASK_MESSAGE_DATA& req_data)> 						req_snd_callback;
	typedef function<void(u16& err_code, interprocess_exception& ex)> 			req_snd_error_callback;

	typedef function<void(TASK_MESSAGE_DATA& rep_data)>								rep_rev_callback;
	typedef function<void(TSS* tss, u16& err_code, interprocess_exception& ex)> 	rep_rev_error_callback;
	typedef function<void(u16& err_code, interprocess_exception& ex)> 				rep_snd_error_callback;


	// Construct object on the side of sender.
	shared_memory_ask_and_answer_imp(req_snd_error_callback f3,
									 req_snd_callback f4,
									 rep_rev_callback f11,
									 rep_rev_error_callback f12):
		f1_(0),
		f2_(0),
		f3_(f3),
		f4_(f4),
		f11_(f11),
		f12_(f12),
		f13_(0),
		is_make_req_thread_(false),
		is_make_rep_thread_(true),
		rep_thr_(boost::bind(&shared_memory_ask_and_answer_imp::rep_svc, this))
	{
		req_shm_name_.append(PARSER::shared_memory_name);
		req_shm_name_.append("_req");
		rep_shm_name_.append(PARSER::shared_memory_name);
		rep_shm_name_.append("_rep");

		async_call_ = new symmetric_coroutine< void*& >::call_type(
				boost::bind(&shared_memory_ask_and_answer_imp::ask_yield, this, _1));

		async_call_data_.req_snd_callback = f4_;
		async_call_data_.req_put_callback = boost::bind(&shared_memory_ask_and_answer_imp::req_put, this, _1, _2);
		async_call_data_.rep_rev_callback = f11_;
		async_call_data_.is_receive_data = false;

	};
	// Construct object on the side of receiver.
	shared_memory_ask_and_answer_imp(req_rev_callback f1,
									 req_rev_error_callback f2,
									 rep_snd_error_callback f13):
		f1_(f1),
		f2_(f2),
		f3_(0),
		f4_(0),
		f11_(0),
		f12_(0),
		f13_(f13),
		is_make_req_thread_(true),
		is_make_rep_thread_(false),
		req_thr_(boost::bind(&shared_memory_ask_and_answer_imp::req_svc, this))
	{
		req_shm_name_.append(PARSER::shared_memory_name);
		req_shm_name_.append("_req");
		rep_shm_name_.append(PARSER::shared_memory_name);
		rep_shm_name_.append("_rep");

		async_call_ = 0;
	}

	~shared_memory_ask_and_answer_imp(){
		// Destroy object on the side of receiver.
		if(is_make_req_thread_){

			req_exit_thread();
			req_thr_.join();

#ifdef BINGO_PROCESS_TASK_SHARED_MEMORY_DEBUG
			test_output("req_thr: exit success")
#endif
			req_remove();
			rep_remove();
		}

		// Destroy object on the side of sender.
		if(is_make_rep_thread_){

			if(async_call_) delete async_call_;

			rep_exit_thread();
			rep_thr_.join();

#ifdef BINGO_PROCESS_TASK_SHARED_MEMORY_DEBUG
			test_output("rep_thr: exit success")
#endif
		}
	}

	// Start to command on the side of the sender.
	void run(){

		void* p = &async_call_data_;
		(*async_call_)(p);
	}

	void ask_yield(symmetric_coroutine< void*& >::yield_type & yield){
		for(;;){

			ask_and_answer_data<TASK_MESSAGE_DATA>* a =
					static_cast<ask_and_answer_data<TASK_MESSAGE_DATA>*>(yield.get());

			if(!a->is_receive_data){
				a->rep_data.data.clear();
				a->req_snd_callback(a->rep_data);

				if(a->rep_data.data.length() > 0){
					u16 err_code = 0;
					a->req_put_callback(a->rep_data, err_code);

					a->is_receive_data = true;
				}

				yield();

			}else{
				a->rep_rev_callback(a->rep_data);

				a->is_receive_data = false;
			}

		}
	}


private:
	int rep_remove(){

		bool suc = shared_memory_object::remove(rep_shm_name_.c_str());

#ifdef BINGO_PROCESS_TASK_SHARED_MEMORY_DEBUG
		test_output("rep_remove() return " << suc)
#endif
		return (suc)? 0 : -1;
	}

	int rep_put(TASK_MESSAGE_DATA& msg, u16& err_code){

		try{
			//Create a shared memory object.
			shared_memory_object shm
			  (open_only               				//open
			  ,rep_shm_name_.c_str()		       	//name
			  ,read_write                			//read-write mode
			  );

			//Map the whole shared memory in this process
			mapped_region region
				 (shm                       //What to map
				 ,read_write 				//Map it as read-write
				 );

			//Get the address of the mapped region
			void * addr       = region.get_address();

			//Obtain a pointer to the shared structure
			shared_memory_rep_data<PARSER>* data = static_cast<shared_memory_rep_data<PARSER>* >(addr);

			{
				 scoped_lock<interprocess_mutex> lock(data->mutex);

				 if(data->message_in){
					 if(!data->cond_full.timed_wait(lock, get_system_time() + milliseconds(500))){
						 err_code = error_process_task_rep_send_data_fail;
						 return  -1;
					 }
				 }

				 // Put message into shared_memory.
				 char* p = &data->items[0];
				 memset(p, 0x00, PARSER::message_size);
				 memcpy(p, msg.data.header(), msg.data.length());
				 data->type = msg.type;

				 //Mark message buffer as full
				 data->message_in = true;

				 //Notify to the other process that there is a message
				 data->cond_empty.notify_one();

				 return 0;

			}
		}catch(interprocess_exception &ex){
			err_code = error_process_task_rep_send_data_fail;
			if(f13_) f13_(err_code, ex);

			return -1;
		}

	}

	void rep_exit_thread(){

		// Send exit thread message.
		u16 err_code = 0;
		while(rep_put(exit_thr_, err_code) == -1){}
	}

	void rep_svc(){

#ifdef BINGO_PROCESS_TASK_SHARED_MEMORY_DEBUG
		test_output("thr:" << this_thread::get_id() <<",rep_thr start!")
#endif

		this_thread_tss.reset(new TSS());	// initialize the thread's storage

		while(true){
			try{
				//Create a shared memory object.
				shared_memory_object shm
				  (open_or_create                	//open
				  ,rep_shm_name_.c_str() 			//name
				  ,read_write                     	//read-write mode
				  );

				 //Set size
				shm.truncate(sizeof(shared_memory_rep_data<PARSER>));

				//Map the whole shared memory in this process
				mapped_region region
					 (shm                       //What to map
					 ,read_write 				//Map it as read-write
					 );

				//Get the address of the mapped region
				void * addr       = region.get_address();

				//Construct the shared structure in memory
				shared_memory_rep_data<PARSER> * data = new (addr) shared_memory_rep_data<PARSER>;

#ifdef BINGO_PROCESS_TASK_SHARED_MEMORY_DEBUG
				test_output("rep_thr: shm create success!")
#endif

				//Print messages until the other process marks the end
				bool end_loop = false;
				do{
					{
						scoped_lock<interprocess_mutex> lock(data->mutex);
						if(!data->message_in){
							data->cond_empty.wait(lock);
						}

						if(data->type == task_data_type_exit_thread){
	//						end_loop = true;
							return;
						}
						else{
							//Call rev_callback.
							char* p = &data->items[0];

							// Write rep_data_
							u16 err_code;
							async_call_data_.rep_data.data.clear();
							async_call_data_.rep_data.data.copy(p, PARSER::message_size, err_code);

							//Notify the other process that the buffer is empty
							data->message_in = false;
							data->cond_full.notify_one();
						}
					}

					void* p1 = &async_call_data_;
					(*async_call_)(p1);
				}
				while(!end_loop);
			}
			catch(interprocess_exception &ex){

				u16 err_code = error_process_task_rep_receive_data_fail;
				f12_((TSS*)this_thread_tss.get(), err_code, ex);

				// Interval 1 seconds open shm again.
				this_thread::sleep(seconds(1));

			}
		}
	}





	int req_remove(){

		bool suc = shared_memory_object::remove(req_shm_name_.c_str());

#ifdef BINGO_PROCESS_TASK_SHARED_MEMORY_DEBUG
		test_output("req_remove() return " << suc)
#endif
		return (suc)? 0 : -1;
	}

	int req_put(TASK_MESSAGE_DATA& msg, u16& err_code){

		int counter = 0;
		while(counter < 5){

			try{
				//Create a shared memory object.
				shared_memory_object shm
				  (open_only               				//open
				  ,req_shm_name_.c_str()		       	//name
				  ,read_write                			//read-write mode
				  );

				//Map the whole shared memory in this process
				mapped_region region
					 (shm                       					//What to map
					 ,read_write 				//Map it as read-write
					 );

				//Get the address of the mapped region
				void * addr       = region.get_address();

				//Obtain a pointer to the shared structure
				shared_memory_req_data<PARSER>* data = static_cast<shared_memory_req_data<PARSER>* >(addr);

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
				err_code = error_process_task_req_send_data_fail;
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

	void req_exit_thread(){

		// Send exit message to receiver that to exit req_svc().
		u16 err_code = 0;
		req_put(exit_thr_, err_code);
	}

	void req_svc(){

#ifdef BINGO_PROCESS_TASK_SHARED_MEMORY_DEBUG
		test_output("req_thr: start")
#endif

		this_thread_tss.reset(new TSS());	// initialize the thread's storage

		try{
			//Create a shared memory object.
			shared_memory_object shm
			  (open_or_create                	//create
			  ,req_shm_name_.c_str() 			//name
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
			test_output("req_thr: shm create success!")
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
					TASK_MESSAGE_DATA rep_data;

					char* p = &data->items[0];
					f1_((TSS*)this_thread_tss.get(), p, rep_data);

					data->message_in = false;

					// Send response message to sender as far as success.
					u16 err_code = 0;
					int counter = 0;
					// Give up after 5 order
					while(counter < 5){
						if(rep_put(rep_data, err_code) == -1)
							counter++;
						else
							break;
					}

					//Notify the other process that the buffer is empty
					data->cond_full.notify_one();

				}
			}
			while(!end_loop);
		}
		catch(interprocess_exception &ex){

			u16 err_code = error_process_task_req_receive_data_fail;
			f2_((TSS*)this_thread_tss.get(), err_code, ex);

		}
	}

private:

	string req_shm_name_;
	string rep_shm_name_;

	task_exit_data<PARSER> exit_thr_;

	boost::thread rep_thr_;
	bool is_make_rep_thread_;

	boost::thread req_thr_;
	bool is_make_req_thread_;

	req_rev_callback 		f1_;
	req_rev_error_callback 	f2_;
	req_snd_error_callback 	f3_;
	req_snd_callback		f4_;

	rep_rev_callback 		f11_;
	rep_rev_error_callback 	f12_;
	rep_snd_error_callback 	f13_;

	symmetric_coroutine< void*& >::call_type* async_call_;
	ask_and_answer_data<TASK_MESSAGE_DATA> async_call_data_;
};

} } }


#endif /* PROCESS_TASK_SHARED_MEMORY_ASK_AND_ANSWER_IMP_H_ */
