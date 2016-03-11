/*
 * bingo_security.cpp
 *
 *  Created on: 2016-2-20
 *      Author: root
 */

#include <boost/test/unit_test.hpp>

#include <iostream>
using namespace std;

#include "cost_time.h"

#include "bingo/security/all.h"
using namespace bingo;
using namespace bingo::security;

BOOST_AUTO_TEST_SUITE(bingo_security)

BOOST_AUTO_TEST_CASE(t_crc32){

	char data[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

	cout << security::make_crc32(&data[0], 6) << endl;
	// output:
	//	2180413220
}

#pragma pack(1)
struct des_cbc_mblk{
	char data[1024];
};
#pragma pack()

struct des_cbc_parser{
	static u8 key[8];
	static u8 iv[8];
	static int evp_max_key_length;
};
u8 des_cbc_parser::key[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
u8 des_cbc_parser::iv[8] = 	{0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18};
int des_cbc_parser::evp_max_key_length = 8;

BOOST_AUTO_TEST_CASE(t_des_cbc){

	typedef des_cbc_algorithm<des_cbc_mblk, des_cbc_parser> des_cbc_algorithm_type;
	typedef symmetric_encryptor<
				des_cbc_algorithm_type,
				des_cbc_mblk
				> encryptor_type;

	// 测试加密字符串
	{
		bingo::stringt t;
		string s_out;

		string in = "this is a test", err;
		cout << "string:" << in << endl;

		{
			mem_guard<des_cbc_mblk> pk;
			u16 err_code = 0;
			string err_what;

			des_cbc_algorithm_type type_;
			type_.encrypt(in, pk, err_what, err_code);
			cout << "string encrypt:" << t.stream_to_string(pk.header(), pk.length()) << endl;

			s_out.clear();
			type_.decrypt(pk.header(), pk.length(), s_out, err_what, err_code);
			cout << "string decrypt:" << s_out << endl;
			cout << endl;
			// output:
			//		string:			this is a test
			//		string encrypt:	e0 2e 47 e5 16 ad 9b 4c 89 f1 52 ae b5 42 2e 17
			//		string decrypt:	this is a test
		}

		{
			mem_guard<des_cbc_mblk> pk;
			u16 err_code = 0;
			string err_what;

			encryptor_type type_;
			type_.encrypt(in, pk, err_what, err_code);
			cout << "mem_guard encrypt:" << t.stream_to_string(pk.header(), pk.length()) << endl;

			s_out.clear();
			type_.decrypt(pk.header(), pk.length(), s_out, err_what, err_code);
			cout << "mem_guard decrypt:" << s_out << endl;
			cout << endl;

			// output:
			//		mem_guard encrypt:e0 2e 47 e5 16 ad 9b 4c 89 f1 52 ae b5 42 2e 17
			//		mem_guard decrypt:this is a test
		}

	}

	// 测试加密流
	{
		bingo::stringt t;

		string err;
		char data[11] = {0x68, 0x32, 0x02, 0x00, 0x01, 0x02, 0x03, 0x04, 0xb1, 0xe2, 0x01};
		cout << "stream:" << t.stream_to_string(data, 11) << endl;

		{
			mem_guard<des_cbc_mblk> pk;
			u16 err_code = 0;
			string err_what;

			des_cbc_algorithm_type type_;
			type_.encrypt(data, 11, pk, err_what, err_code);
			cout << "stream encrypt:" << t.stream_to_string(pk.header(), pk.length()) << endl;

			mem_guard<des_cbc_mblk> out_pk;
			type_.decrypt(pk.header(), pk.length(), out_pk, err_what, err_code);
			cout << "stream decrypt:" << t.stream_to_string(out_pk.header(), out_pk.length()) << endl;
			cout << endl;
			// output:
			//		stream:			68 32 02 00 01 02 03 04 b1 e2 01
			//		stream encrypt:	b3 03 a4 48 95 72 ec 6f 63 ea 05 8b f9 a4 be de
			//		stream decrypt:	68 32 02 00 01 02 03 04 b1 e2 01
		}

		{
			mem_guard<des_cbc_mblk> pk;
			u16 err_code = 0;
			string err_what;

			encryptor_type type_;
			type_.encrypt(data, 11, pk, err_what, err_code);
			cout << "mem_guard encrypt:" << t.stream_to_string(pk.header(), pk.length()) << endl;

			mem_guard<des_cbc_mblk> out_pk;
			type_.decrypt(pk.header(), pk.length(), out_pk, err_what, err_code);
			cout << "mem_guard decrypt:" << t.stream_to_string(out_pk.header(), out_pk.length()) << endl;
			cout << endl;

			// output:
			//		mem_guard encrypt:b3 03 a4 48 95 72 ec 6f 63 ea 05 8b f9 a4 be de
			//		mem_guard decrypt:68 32 02 00 01 02 03 04 b1 e2 01
		}
	}
}

#pragma pack(1)
struct xhm_sec_mblk{
	char data[256];
};
#pragma pack()

BOOST_AUTO_TEST_CASE(t_xhm_sec){

	string or_str = "abcdefg1234567,this is a test!";
	char or_data[] = {0x68, 0x23, 0x23, 0x01, 0x02, 0x03, 0x04, 0x05, 0xb1, 0xb2, 0x16};

	{
		stringt t;
		cout << "orginal data:" << t.stream_to_string(or_data, 11) << endl;

		typedef xhm_sec_algorithm<xhm_sec_mblk> xhm_sec_algorithm_type;
		typedef symmetric_encryptor<
					xhm_sec_algorithm_type,
					xhm_sec_mblk
					> encryptor_type;


		mem_guard<xhm_sec_mblk> pk;
		u16 err_code = 0;
		string err_what;

		encryptor_type type_;
		if(type_.encrypt(or_data, 11, pk, err_what, err_code) == 0){
			cout << "stream encrypt success:" << t.stream_to_string(pk.header(), pk.length()) << endl;

			mem_guard<xhm_sec_mblk> out_pk;
			if(type_.decrypt(pk.header(), pk.length(), out_pk, err_what, err_code) == 0)
				cout << "stream decrypt success:" << t.stream_to_string(out_pk.header(), out_pk.length()) << endl;
			else
				cout << "stream decrypt fail, err:" << err_code << endl;
		}else{
			cout << "stream encrypt fail, err:" << err_code << endl;
		}

		cout << "orginal str:" << or_str << endl;
		mem_guard<xhm_sec_mblk> strpk;
		if(type_.encrypt(or_str, strpk, err_what, err_code) == 0){
			cout << "str encrypt success:" << t.stream_to_string(strpk.header(), strpk.length()) << endl;

			string outstr;
			if(type_.decrypt(strpk.header(), strpk.length(), outstr, err_what, err_code) == 0)
				cout << "str decrypt success:" << outstr << endl;
			else
				cout << "str decrypt fail, err:" << err_code << endl;
		}else{
			cout << "str encrypt fail, err:" << err_code << endl;
		}
	}

	// output:
	//	orginal data:68 23 23 01 02 03 04 05 b1 b2 16
	//	stream encrypt:eb d9 6f e9 df cf c0 f2 db 4f 8f 80 c0 74 7f a1 0b
	//	stream decrypt:68 23 23 01 02 03 04 05 b1 b2 16

	//	orginal str:abcdefg1234567,this is a test!
	//	str encrypt:a6 80 04 12 09 4f a8 9e e6 44 5c e8 b3 31 1c dd f3 71 08 5c 16 94 4d a9 b7 84 59 9c c7 10 78 c8 92 55 e5 3e 1e
	//	str decrypt:abcdefg1234567,this is a test!

}

BOOST_AUTO_TEST_SUITE_END()


