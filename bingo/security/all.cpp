/*
 * all.cpp
 *
 *  Created on: 2016-3-8
 *      Author: root
 */

#include "all.h"

using bingo::security::boost_random_parser;
const char* boost_random_parser::chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

using bingo::security::uuid_name_parser;
const char* uuid_name_parser::chars = "{0123456789abcdef0123456789ABCDED}";

uuid bingo::security::make_uuid(const char* data_header){
	uuid u1;
	memcpy(u1.data, data_header, 16);

	return u1;
}

#include <boost/crc.hpp>
u32 bingo::security::make_crc32(const char* data, size_t data_size){
	boost::crc_32_type crc;
	crc.process_bytes(data, data_size);

	return crc.checksum();
}
