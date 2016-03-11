/*
 * mq_package.cpp
 *
 *  Created on: 2016-3-3
 *      Author: root
 */

#include "mq_package.h"

using bingo::mq::mq_heartjump_package;
using namespace bingo::mq;

size_t mq_heartjump_package::data_size = MAX_SIZE_OF_MQ_HEARTJUMP_PACKAGE;
char mq_heartjump_package::data[MAX_SIZE_OF_MQ_HEARTJUMP_PACKAGE] = {0x03, 0x0B, 0x00,
		0x68, 0x23, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x16};



int bingo::mq::make_mq_data_pk(mem_guard<mq_package>& out, string& dest_name, string& sour_name, const char* data, size_t data_size, u16& err_code){

	// Check name length
	if(dest_name.length() > (MAX_SIZE_OF_MQ_PACKAGE_DEST_NAME_SECTION - 1)){
		err_code = error_mq_send_data_dest_name_length_more_than_max_length;
		return -1;
	}

	if(sour_name.length() > (MAX_SIZE_OF_MQ_PACKAGE_SOUR_NAME_SECTION - 1)){
		err_code = error_mq_send_data_sour_name_length_more_than_max_length;
		return -1;
	}

	// Check name isn't zero.
	if(dest_name.empty()){
		err_code = error_mq_send_data_dest_name_is_blank;
		return -1;
	}

	if(sour_name.empty()){
		err_code = error_mq_send_data_sour_name_is_blank;
		return -1;
	}

	// Check data length
	if(data_size > MAX_SIZE_OF_MQ_PACKAGE_DATA_SECTION){

		err_code = error_mq_send_data_length_more_than_max_length;
		return -1;
	}

	size_t snd_data_size = MAX_SIZE_OF_MQ_PACKAGE_HEADER + MAX_SIZE_OF_MQ_PACKAGE_NAME_SECTION + data_size;
	char snd_data[snd_data_size];
	memset(&snd_data[0], 0x00, snd_data_size);

	// type
	snd_data[0] = mq_package_type_data;

	// length
	u16 len = MAX_SIZE_OF_MQ_PACKAGE_NAME_SECTION + data_size;
	memcpy(snd_data + 1, &len, 2);

	// name
	memcpy(snd_data + MAX_SIZE_OF_MQ_PACKAGE_HEADER, dest_name.c_str(), dest_name.length());
	if(!sour_name.empty())
		memcpy(snd_data + MAX_SIZE_OF_MQ_PACKAGE_HEADER + MAX_SIZE_OF_MQ_PACKAGE_DEST_NAME_SECTION, sour_name.c_str(), sour_name.length());

	// data
	memcpy(snd_data + MAX_SIZE_OF_MQ_PACKAGE_HEADER + MAX_SIZE_OF_MQ_PACKAGE_NAME_SECTION, data, data_size);

	return out.copy(snd_data, snd_data_size, err_code);
}

int bingo::mq::make_mq_data_request_only_pk(mem_guard<mq_package>& out,
		string& dest_name,
		const char* data, size_t data_size, u16& err_code){

	// Check name length
	if(dest_name.length() > (MAX_SIZE_OF_MQ_PACKAGE_DEST_NAME_SECTION - 1)){
		err_code = error_mq_send_data_dest_name_length_more_than_max_length;
		return -1;
	}

	// Check name isn't zero.
	if(dest_name.empty()){
		err_code = error_mq_send_data_dest_name_is_blank;
		return -1;
	}

	// Check data length
	if(data_size > MAX_SIZE_OF_MQ_PACKAGE_REQUEST_ONLY_DATA_SECTION){

		err_code = error_mq_send_data_length_more_than_max_length;
		return -1;
	}

	size_t snd_data_size = MAX_SIZE_OF_MQ_PACKAGE_HEADER + MAX_SIZE_OF_MQ_PACKAGE_REQUEST_ONLY_NAME_SECTION + data_size;
	char snd_data[snd_data_size];
	memset(&snd_data[0], 0x00, snd_data_size);

	// type
	snd_data[0] = mq_package_type_data_request_only;

	// length
	u16 len = MAX_SIZE_OF_MQ_PACKAGE_REQUEST_ONLY_NAME_SECTION + data_size;
	memcpy(snd_data + 1, &len, 2);

	// name
	memcpy(snd_data + MAX_SIZE_OF_MQ_PACKAGE_HEADER, dest_name.c_str(), dest_name.length());

	// data
	memcpy(snd_data + MAX_SIZE_OF_MQ_PACKAGE_HEADER + MAX_SIZE_OF_MQ_PACKAGE_REQUEST_ONLY_NAME_SECTION, data, data_size);

	return out.copy(snd_data, snd_data_size, err_code);
}

int bingo::mq::make_mq_register_pk(mem_guard<mq_package>& out, string& sour_name, u16& err_code){

	// Check name length.
	if(sour_name.length() > (MAX_SIZE_OF_MQ_PACKAGE_SOUR_NAME_SECTION - 1)){
		err_code = error_mq_send_register_name_length_more_than_max_length;
		return -1;
	}

	// Check name isn't zero.
	if(sour_name.empty()){
		err_code = error_mq_send_data_sour_name_is_blank;
		return -1;
	}


	char data[MAX_SIZE_OF_MQ_REGISTER_PACKAGE];
	memset(&data[0], 0x00, MAX_SIZE_OF_MQ_REGISTER_PACKAGE);

	// type
	data[0] = mq_package_type_register_tcp_handler_name;

	// length
	u16 len = MAX_SIZE_OF_MQ_PACKAGE_SOUR_NAME_SECTION;
	memcpy(data + 1, &len, 2);

	// data
	memcpy(data + MAX_SIZE_OF_MQ_PACKAGE_HEADER, sour_name.c_str(), sour_name.length());

	return out.copy(data, MAX_SIZE_OF_MQ_REGISTER_PACKAGE, err_code);
}
