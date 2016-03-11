/*
 * task_data.h
 *
 *  Created on: 2016-2-1
 *      Author: root
 */

#ifndef PROCESS_TASK_DATA_H_
#define PROCESS_TASK_DATA_H_

namespace bingo { namespace xprocess { namespace task {

enum{
	task_data_type_data  			= 0x00,
	task_data_type_exit_thread  	= 0x01,
};

#pragma pack(1)
template<typename PARSER>
struct task_data{
	char message[PARSER::message_size];		// Task message
};
#pragma pack()

template<typename PARSER>
struct task_message_data {
	u8 type;									// Type of task data
	mem_guard<task_data<PARSER> > data;			// Task message

	task_message_data():type(task_data_type_data){}
	task_message_data(u8 t):type(t){}
};

template<typename PARSER>
struct task_exit_data : public task_message_data<PARSER> {
	task_exit_data():task_message_data<PARSER>(task_data_type_exit_thread){}
};

} } }

#endif /* PROCESS_TASK_DATA_H_ */
