/*
 * error_code.h
 *
 *  Created on: 2016-1-18
 *      Author: root
 */

#ifndef ERROR_CODE_H_
#define ERROR_CODE_H_

namespace bingo {

enum{
	// Tcp error
	error_tcp_handler_mgr_sequence_is_full   		 			= 0x0101,		// 257
	error_tcp_handler_mgr_element_no_exist						= 0x0102,		// 258

	error_tcp_package_rev_exceed_max_size						= 0x0103,		// 259
	error_tcp_package_queue_is_empty							= 0x0104,		// 260
	error_tcp_package_header_is_wrong							= 0x0105,		// 261
	error_tcp_package_body_is_wrong								= 0x0106,		// 262

	error_tcp_client_close_socket_because_server				= 0x0107,		// 263
	error_tcp_client_close_socket_because_self					= 0x0108,		// 264

	error_tcp_server_close_socket_because_client				= 0x0109,		// 265
	error_tcp_server_close_socket_because_heartjump				= 0x010A,		// 266
	error_tcp_server_close_socket_because_authrication_pass		= 0x010B,		// 267
	error_tcp_server_close_socket_because_package				= 0x010C,		// 268
	error_tcp_package_snd_exceed_max_size						= 0x010D,		// 269
	error_tcp_unknown											= 0x010F,		// 271

	// Thread task error
	error_thread_task_queue_is_full								= 0x0110,		// 272
	error_thread_task_svc_thread_exit							= 0x0111,		// 273
	error_thread_task_allocation_queue_is_empty					= 0x0112,		// 274

	// Process task error
	error_process_task_check_send_data_is_wrong					= 0x0119,		// 281
	error_process_task_send_data_fail							= 0x011A,		// 282
	error_process_task_receive_data_fail						= 0x011B,		// 283
	error_process_task_req_send_data_fail						= 0x011C,		// 284
	error_process_task_req_receive_data_fail					= 0x011D,		// 285
	error_process_task_rep_send_data_fail						= 0x011E,		// 286
	error_process_task_rep_receive_data_fail					= 0x011F,		// 287

	// Other error
	error_mem_guard_copy_data_exceed_max_size					= 0x0120,		// 288
	error_mem_guard_append_data_exceed_max_size					= 0x0121,		// 289
	error_mem_guard_change_length_exceed_max_size				= 0x0122,		// 290

	// MQ error
	error_mq_receive_heartjump_is_wrong							= 0x0130,		// 304
	error_mq_recevie_register_name_is_wrong						= 0x0131,		// 305
	error_mq_recevie_register_name_length_more_than_max_length	= 0x0132,		// 306
	error_mq_recevie_data_is_wrong								= 0x0133,		// 307
	error_mq_recevie_data_name_length_more_than_max_length		= 0x0134,		// 308
	error_mq_recevie_data_length_more_than_max_length			= 0x0135,		// 309
	error_mq_send_data_fail_because_server_go_away				= 0x0136,		// 310
	error_mq_send_register_name_length_more_than_max_length		= 0x0137,		// 311
	error_mq_send_data_dest_name_length_more_than_max_length	= 0x0138,		// 312
	error_mq_send_data_length_more_than_max_length				= 0x0139,		// 313
	error_mq_send_data_dest_name_is_blank						= 0x013A,		// 314
	error_mq_send_register_name_has_exist						= 0x013B,		// 315
	error_mq_send_data_sour_name_length_more_than_max_length	= 0x013C,		// 316
	error_mq_send_data_sour_name_is_blank						= 0x013D,		// 317

	// Security error
	error_security_des_cbc_encrypt_is_wrong						= 0x0140,		// 320
	error_security_xhm_sec_encrypt_is_wrong						= 0x0141,		// 321
};

}

#endif /* ERROR_CODE_H_ */
