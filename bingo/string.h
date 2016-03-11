/*
 * string.h
 *
 *  Created on: 2016-1-18
 *      Author: root
 */

#ifndef STRING_H_
#define STRING_H_

#include <stdio.h>
#include <iostream>
using namespace std;

#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
using namespace boost;

namespace bingo {

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

class stringt{
public:
	stringt(){
		memset(&chr_[0], 0x00, MAX_SIZE_OF_CHR);
	}
public:

	// byte's stream -> string.
	// for example: {0x00, 0x01, 0x02} -> '00 01 02'
	const char* stream_to_string(const char* p, size_t p_size){

		clear();

		char tmp[4] = {0x00};
		for (size_t i = 0; i < p_size; ++i) {
			u8 ch = (u8)(*(p + i));
			memset(tmp, 0x00, 4);
			sprintf(tmp, "%02x ", ch);

			str_.append(&tmp[0]);
		}

		return str_.c_str();
	}

	void stream_to_string(const char* p, size_t p_size, string& out){
		char tmp[4] = {0x00};
		for (size_t i = 0; i < p_size; ++i) {
			u8 ch = (u8)(*(p + i));
			memset(tmp, 0x00, 4);
			sprintf(tmp, "%02x ", ch);

			out.append(&tmp[0]);
		}
	}

	// byte's stream -> string.
	// for example: {0x66, 0x59, 0x48, 0x61} -> 'fYHa'
	const char* stream_to_char(const char* p, size_t p_size){
		clear();


		char str[p_size + 1];
		memset(str, 0x00, p_size+1);
		memcpy(str, p, p_size);

		str_.append(str);

		return str_.c_str();
	}

	// short -> byte of stream.
	// for example: 260 -> {0x04, 0x01}
	const char* short_to_stream(u16 value){
		clear();

		chr_[0] = value % 256;
		chr_[1] = value / 256;

		return chr_;
	}

	// byte of stream -> short
	// for example: {0x04, 0x01} -> 260
	u16 stream_to_short(const char* p){

		u16 v;
		memcpy(&v, p, 2);

		return v;
	}

	// memory's address -> long.
	// for example: char* p = new char[10];
	//              p -> '12548741244422'
	u64 pointer_to_long(const void* p){
		u64 address;
		memcpy(&address, &p, 8);

		return address;
	}

	// Ip string -> int.
	u32 ip_to_int(string& s){

		vector<string> v;
		split(v, s, is_any_of("."), token_compress_on);

		if(v.size() != 4)
			return 0;
		else{

			u8 data[4] = {0x00, 0x00, 0x00, 0x00};
			data[0] = lexical_cast<u32>(v[0]);
			data[1] = lexical_cast<u32>(v[1]);
			data[2] = lexical_cast<u32>(v[2]);
			data[3] = lexical_cast<u32>(v[3]);

			return data[0] + data[1]*256 + data[2]*256*256 + data[3]*256*256*256;
		}
	}

	// Int -> ip string
	const char* int_to_ip(u32 n){
		clear();

		u32 n_part1 = n % 256;
		u32 n_part2 = n / 256 % 256;
		u32 n_part3 = n / 256 / 256 % 256;
		u32 n_part4 = n / 256 / 256 / 256;

		char tmp[16] = {0x00};
		memset(tmp, 0x00, 16);
		sprintf(&tmp[0], "%d.%d.%d.%d", n_part1, n_part2, n_part3, n_part4);

		str_.append(&tmp[0]);

		return str_.c_str();
	}

	// Convert T to string.
	template<typename T>
	const char* convert(const T& t){
		clear();
		oss_ << t;
		oss_ >> str_;
		return str_.c_str();
	}

	const char* convert(const char& t){
		clear();
		u8 c = (u8)t;
		oss_ << (u16)c;
		oss_ >> str_;
		return str_.c_str();
	}

	const char* convert(const u8& t){
		clear();
		oss_ << (u16)t;
		oss_ >> str_;
		return str_.c_str();
	}


	template<typename T>
	void convert(const T& t, string& out){
		oss_.clear();
		oss_ << t;
		oss_ >> out;
	}

private:
	void clear(){
		oss_.clear();
		str_ .clear();
		memset(&chr_[0], 0x00, MAX_SIZE_OF_CHR);
	}
private:
	std::stringstream oss_;
	std::string str_;
	enum { MAX_SIZE_OF_CHR = 8, };
	char chr_[MAX_SIZE_OF_CHR];
};




#define test_output(str) cout << str << endl;
#define test_output_with_time(str) { \
		ptime p1 = second_clock::local_time(); \
		cout << str << ", time:" << to_iso_extended_string(p1) << endl; \
		}

}


#endif /* STRING_H_ */
