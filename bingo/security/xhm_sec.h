/*
 * xhm_sec.h
 *
 *  Created on: 2016-2-20
 *      Author: root
 */

#ifndef SECURITY_XHM_SEC_H_
#define SECURITY_XHM_SEC_H_

#include "../string.h"
using bingo::u8;
using bingo::u32;

#include "Ihandler.h"
#include "crc32.h"
#include "boost_random.h"

namespace bingo { namespace security {

template<typename MEMORY_BLOCK>
class xhm_sec_algorithm : public IEncryptorHandler<MEMORY_BLOCK>{
public:
	xhm_sec_algorithm() :
		IEncryptorHandler<MEMORY_BLOCK>(){}
	virtual ~xhm_sec_algorithm(){}
public:
	int encrypt(string& in, mem_guard<MEMORY_BLOCK>& block, string& err_what, u16& err_code){

		size_t data_size = strlen(in.c_str());

		// Check length, max length is data_size + max_added_bit + random + real_length
		if(data_size + 3 + 4 + 1 > sizeof(MEMORY_BLOCK)){
			err_what = "encrypt buffer is too small!";
			err_code = error_security_xhm_sec_encrypt_is_wrong;
			return -1;
		}

		char out[sizeof(MEMORY_BLOCK)];
		memset(out, 0x00, sizeof(MEMORY_BLOCK));

		u32 random = make_random();
		u8 out_random[4] = {0x00, 0x00, 0x00, 0x00};

		// Make added-bit
		mem_guard<boost_random_mblk> added_bit_pk;
		make_random(added_bit_pk);


		u8 data_to_encrypt[data_size];
		for (size_t i = 0; i < data_size; ++i) {

			const char* p1 = in.c_str();

			data_to_encrypt[i] = (u8)(*(p1 + i));
		}

		u8 exchange_low=0;
		u8 exdata=0;
		u8 data_length=0;

		//低四位变到高四位
		for(size_t exi=0; exi < data_size ; exi++)
		{
			if(data_to_encrypt[exi]==0){
				break;
			}
			out[exi]=data_to_encrypt[exi];

			exchange_low=(out[exi]&0x0F) <<4;
			out[exi]=(out[exi]>>4 )+exchange_low;

			exchange_low=0;
			++data_length;
		}

		//检测字符串长度是否为4的倍数
		switch(data_length%4)
		{
		case 0:break;
		case 3:
			out[data_length]	=added_bit_pk.this_object()->data[0];
			data_length++;
			break;
		case 2:
			out[data_length]  	=added_bit_pk.this_object()->data[0];
			out[data_length+1]	=added_bit_pk.this_object()->data[1];
			data_length+=2;
			break;
		case 1:
			out[data_length]	=added_bit_pk.this_object()->data[0];
			out[data_length+1]	=added_bit_pk.this_object()->data[1];
			out[data_length+2]	=added_bit_pk.this_object()->data[2];
			data_length+=3;
			break;
		default:break;
		}

		//连续2个一组交换
		for(u8 ex_border=0;ex_border<data_length;ex_border+=2)
		{
			exdata=out[ex_border];
			out[ex_border]=out[ex_border+1];
			out[ex_border+1]=exdata;
			exdata=0;
		}

		//异或随机数
		out_random[3]= (u8)(random);
		out_random[2]= (u8)(random>>8);
		out_random[1]= (u8)(random>>16);
		out_random[0]= (u8)(random>>24);

		for(u8 xori=0;xori<data_length;xori+=4)
		{
			out[xori]^=out_random[0];
			out[xori+1]^=out_random[1];
			out[xori+2]^=out_random[2];
			out[xori+3]^=out_random[3];
		}

		// Make random and encrypt data.
		size_t new_data_size = data_length + 4 + 1;
		char new_out[new_data_size];
		memset(new_out, 0x00, new_data_size);

		if((out[data_length-1] % 2) == 0){

			// even

			// random
			new_out[1] = out_random[0];
			new_out[3] = out_random[1];
			new_out[5] = out_random[2];
			new_out[7] = out_random[3];

			// encrypt data
			new_out[0] = out[0];
			new_out[2] = out[1];
			new_out[4] = out[2];
			new_out[6] = out[3];
			memcpy(new_out + 8, out + 4, data_length - 4);


		}else{

			// odd

			// random
			new_out[0] = out_random[0];
			new_out[2] = out_random[1];
			new_out[4] = out_random[2];
			new_out[6] = out_random[3];

			// encrypt data
			new_out[1] = out[0];
			new_out[3] = out[1];
			new_out[5] = out[2];
			memcpy(new_out + 7, out + 3, data_length - 3);
		}

		// orginal-data size
		new_out[data_length + 4] = data_size;

		return block.copy(new_out, new_data_size, err_code);
	}

	int encrypt(const char* in, size_t in_size, mem_guard<MEMORY_BLOCK>& block, string& err_what, u16& err_code){

		// Check length, max length is in_size + max_added_bit + random + real_length
		if(in_size + 3 + 4 + 1 > sizeof(MEMORY_BLOCK)){
			err_what = "encrypt buffer is too small!";
			err_code = error_security_xhm_sec_encrypt_is_wrong;
			return -1;
		}

		u8 out[sizeof(MEMORY_BLOCK)];
		memset(out, 0x00, sizeof(MEMORY_BLOCK));

		u32 random = make_random();
		u8 out_random[4] = {0x00, 0x00, 0x00, 0x00};

		// Make added-bit
		mem_guard<boost_random_mblk> added_bit_pk;
		make_random(added_bit_pk);

		u8 data_to_encrypt[in_size];
		memset(data_to_encrypt, 0x00, in_size);
		memcpy(data_to_encrypt, in, in_size);

		u8 exchange_low=0;
		u8 exdata=0;
		u8 data_length=0;

		//低四位变到高四位
		for(size_t exi=0; exi < in_size ; exi++)
		{
			out[exi]=data_to_encrypt[exi];

			exchange_low=(out[exi]&0x0F) <<4;
			out[exi]=(out[exi]>>4 )+exchange_low;

			exchange_low=0;
			++data_length;
		}

		//检测字符串长度是否为4的倍数
		switch(data_length%4)
		{
		case 0:break;
		case 3:
			out[data_length]	=added_bit_pk.this_object()->data[0];
			data_length++;
			break;
		case 2:
			out[data_length]	=added_bit_pk.this_object()->data[0];;
			out[data_length+1]	=added_bit_pk.this_object()->data[1];
			data_length+=2;
			break;
		case 1:
			out[data_length]	=added_bit_pk.this_object()->data[0];
			out[data_length+1]	=added_bit_pk.this_object()->data[1];
			out[data_length+2]	=added_bit_pk.this_object()->data[2];
			data_length+=3;
			break;
		default:break;
		}

		//连续2个一组交换
		for(u8 ex_border=0;ex_border<data_length;ex_border+=2)
		{
			exdata=out[ex_border];
			out[ex_border]=out[ex_border+1];
			out[ex_border+1]=exdata;
			exdata=0;
		}

		//异或随机数
		out_random[3]= (u8)(random);
		out_random[2]= (u8)(random>>8);
		out_random[1]= (u8)(random>>16);
		out_random[0]= (u8)(random>>24);

		for(u8 xori=0;xori<data_length;xori+=4)
		{
			out[xori]^=out_random[0];
			out[xori+1]^=out_random[1];
			out[xori+2]^=out_random[2];
			out[xori+3]^=out_random[3];
		}

		// Make random and encrypt data.
		size_t new_data_size = data_length + 4 + 1;
		char new_out[new_data_size];
		memset(new_out, 0x00, new_data_size);

		if((out[data_length-1] % 2) == 0){

			// even

			// random
			new_out[1] = out_random[0];
			new_out[3] = out_random[1];
			new_out[5] = out_random[2];
			new_out[7] = out_random[3];

			// encrypt data
			new_out[0] = out[0];
			new_out[2] = out[1];
			new_out[4] = out[2];
			new_out[6] = out[3];
			memcpy(new_out + 8, out + 4, data_length - 4);

		}else{

			// odd

			// random
			new_out[0] = out_random[0];
			new_out[2] = out_random[1];
			new_out[4] = out_random[2];
			new_out[6] = out_random[3];

			// encrypt data
			new_out[1] = out[0];
			new_out[3] = out[1];
			new_out[5] = out[2];
			memcpy(new_out + 7, out + 3, data_length - 3);
		}

		// orginal-data size
		new_out[data_length + 4] = in_size;

		return block.copy(new_out, new_data_size, err_code);
	}

	int decrypt(const char* in, size_t in_size, string& out, string& err_what, u16& err_code){
		size_t encrypt_data_size = in_size - 4 - 1;

		// Check length, max length is in_size - random - real_length
		if(encrypt_data_size > sizeof(MEMORY_BLOCK)){
			err_what = "decrypt buffer is too small!";
			err_code = error_security_xhm_sec_encrypt_is_wrong;
			return -1;
		}

		// Parse random and encrypt data.
		u8 encrypt_data[sizeof(MEMORY_BLOCK)];
		memset(encrypt_data, 0x00, sizeof(MEMORY_BLOCK));

		u8 in_random[4] = {0x00, 0x00, 0x00, 0x00};
		u8 last = (u8)(in[in_size - 2]);

		if(last % 2 == 0){

			// even

			// random
			in_random[0] = (u8)in[1];
			in_random[1] = (u8)in[3];
			in_random[2] = (u8)in[5];
			in_random[3] = (u8)in[7];

			// data
			encrypt_data[0] = in[0];
			encrypt_data[1] = in[2];
			encrypt_data[2] = in[4];
			encrypt_data[3] = in[6];
			memcpy(encrypt_data + 4, in + 8, encrypt_data_size - 4);

		}else{

			// odd

			// random
			in_random[0] = (u8)in[0];
			in_random[1] = (u8)in[2];
			in_random[2] = (u8)in[4];
			in_random[3] = (u8)in[6];

			// data
			encrypt_data[0] = in[1];
			encrypt_data[1] = in[3];
			encrypt_data[2] = in[5];
			memcpy(encrypt_data + 3, in + 7, encrypt_data_size - 3);
		}

		u8 decrypt_data[sizeof(MEMORY_BLOCK)];
		memset(decrypt_data, 0x00, sizeof(MEMORY_BLOCK));


		u8 exdata=0;
		//异或随机数
		size_t dei=0;
		while(dei < encrypt_data_size)
		{
			encrypt_data[dei]^=in_random[0];
			decrypt_data[dei]=encrypt_data[dei];

			encrypt_data[dei+1]^=in_random[1];
			decrypt_data[dei+1]=encrypt_data[dei+1];

			encrypt_data[dei+2]^=in_random[2];
			decrypt_data[dei+2]=encrypt_data[dei+2];

			encrypt_data[dei+3]^=in_random[3];
			decrypt_data[dei+3]=encrypt_data[dei+3];

			dei+=4;
		}

		//互换连续间隔2者位置
		for(size_t dexi=0;dexi < encrypt_data_size;dexi+=2)
		{
		   exdata=decrypt_data[dexi];
		   decrypt_data[dexi]=decrypt_data[dexi+1];
		   decrypt_data[dexi+1]=exdata;
		   exdata=0;
		}

		//互换高4位，低4位
		for(size_t decxi=0;decxi < encrypt_data_size;decxi++)
		{
		   exdata=(decrypt_data[decxi]&0x0F) <<4;
		   decrypt_data[decxi]=((decrypt_data[decxi] & 0xF0)>>4 )+exdata;
		   exdata=0;
		}

		size_t len = (u8)in[in_size - 1];
		char data_to_decrypt[len + 1];
		memset(data_to_decrypt, 0x00, len + 1);
		for (size_t i = 0; i < len; ++i) {
			data_to_decrypt[i] = (char)(decrypt_data[i]);
		}


		out.append(data_to_decrypt);

		return 0;
	}

	int decrypt(const char* in, size_t in_size, mem_guard<MEMORY_BLOCK>& block, string& err_what, u16& err_code){

		size_t encrypt_data_size = in_size - 4 - 1;

		// Check length, max length is in_size - random - real_length
		if(encrypt_data_size > sizeof(MEMORY_BLOCK)){
			err_what = "decrypt buffer is too small!";
			err_code = error_security_xhm_sec_encrypt_is_wrong;
			return -1;
		}

		// Parse random and encrypt data.
		u8 encrypt_data[sizeof(MEMORY_BLOCK)];
		memset(encrypt_data, 0x00, sizeof(MEMORY_BLOCK));

		u8 in_random[4] = {0x00, 0x00, 0x00, 0x00};
		u8 last = (u8)(in[in_size - 2]);

		if(last % 2 == 0){

			// even

			// random
			in_random[0] = (u8)in[1];
			in_random[1] = (u8)in[3];
			in_random[2] = (u8)in[5];
			in_random[3] = (u8)in[7];

			// data
			encrypt_data[0] = in[0];
			encrypt_data[1] = in[2];
			encrypt_data[2] = in[4];
			encrypt_data[3] = in[6];
			memcpy(encrypt_data + 4, in + 8, encrypt_data_size - 4);

		}else{

			// odd

			// random
			in_random[0] = (u8)in[0];
			in_random[1] = (u8)in[2];
			in_random[2] = (u8)in[4];
			in_random[3] = (u8)in[6];

			// data
			encrypt_data[0] = in[1];
			encrypt_data[1] = in[3];
			encrypt_data[2] = in[5];
			memcpy(encrypt_data + 3, in + 7, encrypt_data_size - 3);
		}

		u8 decrypt_data[sizeof(MEMORY_BLOCK)];
		memset(decrypt_data, 0x00, sizeof(MEMORY_BLOCK));


		u8 exdata=0;
		//异或随机数
		size_t dei=0;
		while(dei < encrypt_data_size)
		{
			encrypt_data[dei]^=in_random[0];
			decrypt_data[dei]=encrypt_data[dei];

			encrypt_data[dei+1]^=in_random[1];
			decrypt_data[dei+1]=encrypt_data[dei+1];

			encrypt_data[dei+2]^=in_random[2];
			decrypt_data[dei+2]=encrypt_data[dei+2];

			encrypt_data[dei+3]^=in_random[3];
			decrypt_data[dei+3]=encrypt_data[dei+3];

			dei+=4;
		}

		//互换连续间隔2者位置
		for(size_t dexi=0;dexi < encrypt_data_size;dexi+=2)
		{
		   exdata=decrypt_data[dexi];
		   decrypt_data[dexi]=decrypt_data[dexi+1];
		   decrypt_data[dexi+1]=exdata;
		   exdata=0;
		}

		//互换高4位，低4位
		for(size_t decxi=0;decxi < encrypt_data_size;decxi++)
		{
		   exdata=(decrypt_data[decxi]&0x0F) <<4;
		   decrypt_data[decxi]=((decrypt_data[decxi] & 0xF0)>>4 )+exdata;
		   exdata=0;
		}

		size_t len = (u8)(in[in_size - 1]);
		char data_to_decrypt[len];
		memset(data_to_decrypt, 0x00, len);
		for (size_t i = 0; i < len; ++i) {
			data_to_decrypt[i] = (char)(decrypt_data[i]);
		}

		return block.copy(data_to_decrypt, len, err_code);
	}

private:
	typedef boost_random_algorithm<
						boost_random_mblk,
						boost_random_parser
						> algorithm_type;

	algorithm_type rng_;

	u32 make_random(){
		mem_guard<boost_random_mblk> pk;
		rng_.make(pk);
		return make_crc32(pk.header(), pk.length());
	}

	void make_random(mem_guard<boost_random_mblk>& pk){
		rng_.make(pk);
	}
};

} }

#endif /* SECURITY_XHM_SEC_H_ */
