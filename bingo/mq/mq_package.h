/*
 * mq_package.h
 *
 *  Created on: 2016-3-3
 *      Author: root
 */

#ifndef MQ_PACKAGE_H_
#define MQ_PACKAGE_H_

#include "../pb.h"
using namespace bingo;

namespace bingo { namespace mq {

enum {
	MAX_SIZE_OF_MQ_PACKAGE 					= 1024,

	MAX_SIZE_OF_MQ_PACKAGE_TYPE_SECTION 		= 1,
	MAX_SIZE_OF_MQ_PACKAGE_LENGTH_SECTION 		= 2,
	MAX_SIZE_OF_MQ_PACKAGE_DEST_NAME_SECTION 	= 64,
	MAX_SIZE_OF_MQ_PACKAGE_SOUR_NAME_SECTION 	= 64,

	MAX_SIZE_OF_MQ_PACKAGE_HEADER 		= MAX_SIZE_OF_MQ_PACKAGE_TYPE_SECTION +
										  MAX_SIZE_OF_MQ_PACKAGE_LENGTH_SECTION,

	MAX_SIZE_OF_MQ_PACKAGE_NAME_SECTION 	= MAX_SIZE_OF_MQ_PACKAGE_DEST_NAME_SECTION +
											  MAX_SIZE_OF_MQ_PACKAGE_SOUR_NAME_SECTION,
	MAX_SIZE_OF_MQ_PACKAGE_DATA_SECTION 	= MAX_SIZE_OF_MQ_PACKAGE -
										  	  MAX_SIZE_OF_MQ_PACKAGE_NAME_SECTION -
										  	  MAX_SIZE_OF_MQ_PACKAGE_LENGTH_SECTION -
										  	  MAX_SIZE_OF_MQ_PACKAGE_TYPE_SECTION,

	MAX_SIZE_OF_MQ_PACKAGE_REQUEST_ONLY_NAME_SECTION = MAX_SIZE_OF_MQ_PACKAGE_DEST_NAME_SECTION,
	MAX_SIZE_OF_MQ_PACKAGE_REQUEST_ONLY_DATA_SECTION = MAX_SIZE_OF_MQ_PACKAGE -
											  	  	   MAX_SIZE_OF_MQ_PACKAGE_REQUEST_ONLY_NAME_SECTION -
											  	  	   MAX_SIZE_OF_MQ_PACKAGE_LENGTH_SECTION -
											  	  	   MAX_SIZE_OF_MQ_PACKAGE_TYPE_SECTION,

	MAX_SIZE_OF_MQ_HEARTJUMP_PACKAGE 	= MAX_SIZE_OF_MQ_PACKAGE_TYPE_SECTION +
										  MAX_SIZE_OF_MQ_PACKAGE_LENGTH_SECTION + 11,

	MAX_SIZE_OF_MQ_REGISTER_PACKAGE  	= MAX_SIZE_OF_MQ_PACKAGE_TYPE_SECTION +
										  MAX_SIZE_OF_MQ_PACKAGE_LENGTH_SECTION +
										  MAX_SIZE_OF_MQ_PACKAGE_SOUR_NAME_SECTION,
};

#pragma pack(1)
/*
 * Package in message queue server, max size of package is 512 bytes.
 * register name package construct:
 * 		| 1-byte  2-byte      | 64-byte(register name)
 * 		| 0x01    0x20, 0x00  | source_name
 *
 * hear-jump package construct:
 * 		| 1-byte  2-byte      | 11-byte
 * 		| 0x03    0x0B, 0x00  | 0x68, 0x23, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x16
 *
 * data package with request and response construct:
 * 		| 1-byte  2-byte        | 64-byte       	 64-byte        n-byte(max is MAX_SIZE_OF_MQ_DATA_PACKAGE)
 * 		| 0x02  + data_length + | destination_name + source_name +  data
 *
 * * data package with request only construct:
 * 		| 1-byte  2-byte        | 64-byte       	 n-byte(max is MAX_SIZE_OF_MQ_DATA_PACKAGE)
 * 		| 0x04  + data_length + | destination_name + data
 *
 * type value:
 * {
 * 		mq_package_type_register_tcp_handler_name 	= 0x01,
 * 		mq_package_type_data						= 0x02,
 * 		mq_package_type_heartjump					= 0x03,
 * 		mq_package_type_data_request_only			= 0x04,
 * }
 */
struct mq_package{
	char data[MAX_SIZE_OF_MQ_PACKAGE];
};

#pragma pack()

struct mq_heartjump_package{
	static size_t data_size;
	static char data[MAX_SIZE_OF_MQ_HEARTJUMP_PACKAGE];
};



enum {
	mq_package_type_register_tcp_handler_name		= 0x01,
	mq_package_type_data							= 0x02,
	mq_package_type_heartjump						= 0x03,
	mq_package_type_data_request_only				= 0x04,
	mq_package_type_max_placeholder					= 0x05,
};

extern int make_mq_data_pk(mem_guard<mq_package>& out,
		string& dest_name, string& sour_name,
		const char* data, size_t data_size, u16& err_code);

extern int make_mq_data_request_only_pk(mem_guard<mq_package>& out,
		string& dest_name,
		const char* data, size_t data_size, u16& err_code);

extern int make_mq_register_pk(mem_guard<mq_package>& out,
		string& sour_name,
		u16& err_code);

} }






#endif /* MQ_PACKAGE_H_ */
