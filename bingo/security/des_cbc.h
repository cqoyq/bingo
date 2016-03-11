/*
 * des_cbc.h
 *
 *  Created on: 2016-2-20
 *      Author: root
 */

#ifndef SECURITY_DES_CBC_H_
#define SECURITY_DES_CBC_H_

#include "../string.h"
using bingo::u32;

#include "openssl/evp.h"

#include "Ihandler.h"

namespace bingo { namespace security {

class des_cbc_key {
public:
	des_cbc_key(){
		key_ = 0;
		key_size_ = 0;

		iv_ = 0;
		iv_size_ = 0;
	}
	virtual ~des_cbc_key(){
		if(key_ != 0) delete[] key_;
		if(iv_  != 0) delete[] iv_;
	}

	void key(u8* key, size_t key_size){

		key_size_ = key_size;

		key_ = new u8[key_size];
		memset(key_, 0x00, key_size);
		memcpy(&key_[0], key, key_size);
	}

	size_t key(u8*& key){

		key = &key_[0];

		return key_size_;
	}

	void iv(u8* iv, size_t iv_size){

		iv_size_ = iv_size;

		iv_ = new u8[iv_size];
		memset(iv_, 0x00, iv_size);
		memcpy(&iv_[0], iv, iv_size);
	}

	size_t iv(u8*& iv){

		iv = &iv_[0];

		return iv_size_;
	}

protected:
	u8* key_;
	size_t key_size_;

	u8* iv_;
	size_t iv_size_;
};

template<typename MEMORY_BLOCK,
		 typename PARSER>
class des_cbc_algorithm : public IEncryptorHandler<MEMORY_BLOCK>{
public:
	des_cbc_algorithm() :
		IEncryptorHandler<MEMORY_BLOCK>(){

		// Set des-cbc key.
		key_ = new des_cbc_key();
		key_->key(PARSER::key, PARSER::evp_max_key_length);
		key_->iv(PARSER::iv, PARSER::evp_max_key_length);
	}
	virtual ~des_cbc_algorithm(){
		if(key_!=0) delete key_;
	}
public:
	int encrypt(string& in, mem_guard<MEMORY_BLOCK>& block, string& err_what, u16& err_code){
		u8* key = 0;
		u8* iv = 0;
		key_->key(key);
		key_->iv(iv);

		u8 out[block.size()];//输出密文缓冲区
		int out_size;
		int out_size_temp;
		int out_total_size; //密文长度
		memset(out, 0x00, block.size());

		// 加密
		{
			EVP_CIPHER_CTX ctx;//evp算法上下文

			EVP_CIPHER_CTX_init(&ctx);

			int result = EVP_EncryptInit_ex(&ctx,EVP_des_cbc(),0,key,iv);
			if(result){

			}else{

				err_what = "encrypt data init fail.";
				err_code = error_security_des_cbc_encrypt_is_wrong;

				EVP_CIPHER_CTX_cleanup(&ctx);

				return -1;
			}

			size_t data_size = strlen(in.c_str());

			u8 s_data[data_size];
			for (size_t i = 0; i < data_size; ++i) {

				const char* p1 = in.c_str();

				s_data[i] = (u8)(*(p1 + i));
			}

			result = EVP_EncryptUpdate(&ctx, out, &out_size, s_data, data_size);

			if(result){

			}else{

				err_what = "encrypt data update fail.";
				err_code = error_security_des_cbc_encrypt_is_wrong;

				EVP_CIPHER_CTX_cleanup(&ctx);

				return -1;
			}

			result = EVP_EncryptFinal_ex(&ctx, out + out_size, &out_size_temp);

			if(result){

				out_total_size = out_size + out_size_temp;

			}else{

				err_what = "encrypt data final fail.";
				err_code = error_security_des_cbc_encrypt_is_wrong;

				EVP_CIPHER_CTX_cleanup(&ctx);

				return -1;
			}

			EVP_CIPHER_CTX_cleanup(&ctx);
		}

		char output_[out_total_size];
		memcpy(output_, out, out_total_size);

		int res = block.copy(&output_[0], out_total_size, err_code);
		if(res == -1) err_what = "encrypt data copy block fail.";
		return res;
	}

	int encrypt(const char* in, size_t in_size, mem_guard<MEMORY_BLOCK>& block, string& err_what, u16& err_code){
		u8* key = 0;
		u8* iv = 0;
		key_->key(key);
		key_->iv(iv);

		u8 out[block.size()];//输出密文缓冲区
		int out_size;
		int out_size_temp;
		int out_total_size; //密文长度
		memset(out, 0x00, block.size());

		// 加密
		{
			EVP_CIPHER_CTX ctx;//evp算法上下文

			EVP_CIPHER_CTX_init(&ctx);

			int result = EVP_EncryptInit_ex(&ctx,EVP_des_cbc(),0,key,iv);
			if(result){

			}else{

				err_what = "encrypt data init fail.";
				err_code = error_security_des_cbc_encrypt_is_wrong;

				EVP_CIPHER_CTX_cleanup(&ctx);

				return -1;
			}

			size_t data_size = in_size;

			u8 s_data[data_size];
			for (size_t i = 0; i < data_size; ++i) {

				const char* p1 = in;

				s_data[i] = (u8)(*(p1 + i));
			}

			result = EVP_EncryptUpdate(&ctx, out, &out_size, s_data, data_size);

			if(result){

			}else{

				err_what = "encrypt data update fail.";
				err_code = error_security_des_cbc_encrypt_is_wrong;

				EVP_CIPHER_CTX_cleanup(&ctx);

				return -1;
			}

			result = EVP_EncryptFinal_ex(&ctx, out + out_size, &out_size_temp);

			if(result){

				out_total_size = out_size + out_size_temp;

			}else{

				err_what = "encrypt data final fail.";
				err_code = error_security_des_cbc_encrypt_is_wrong;

				EVP_CIPHER_CTX_cleanup(&ctx);

				return -1;
			}

			EVP_CIPHER_CTX_cleanup(&ctx);
		}

		char output_[out_total_size];
		memcpy(output_, out, out_total_size);

		int res = block.copy(&output_[0], out_total_size, err_code);
		if(res == -1) err_what = "encrypt data copy block fail.";
		return res;
	}

	int decrypt(const char* in, size_t in_size, string& out, string& err_what, u16& err_code){
		u8 s_input[in_size];
		for (size_t i = 0; i < in_size; ++i) {
			s_input[i] = (u8)(*(in + i));
		}

		u8* key = 0;
		u8* iv = 0;
		key_->key(key);
		key_->iv(iv);

		int buf_size = sizeof(MEMORY_BLOCK);
		u8 de[buf_size];//解码缓冲区
		int de_size;
		int de_size_temp;
		int de_total_size;//解码总长度
		memset(de, 0x00, buf_size);

		// 解密
		{
			EVP_CIPHER_CTX ctx;//evp算法上下文
			EVP_CIPHER_CTX_init(&ctx);

			int result = EVP_DecryptInit_ex(&ctx,EVP_des_cbc(),0,key,iv);

			if(result){
			}
			else{

				err_what = "decrypt data init fail.";
				err_code = error_security_des_cbc_encrypt_is_wrong;

				EVP_CIPHER_CTX_cleanup(&ctx);

				return -1;
			}


			result = EVP_DecryptUpdate(&ctx, de, &de_size, s_input, in_size);

			if(result){
			}
			else{

				err_what = "decrypt data update fail.";
				err_code = error_security_des_cbc_encrypt_is_wrong;

				EVP_CIPHER_CTX_cleanup(&ctx);

				return -1;
			}

			result = EVP_DecryptFinal_ex(&ctx, de + de_size, &de_size_temp);

			if(result){

				// Get total size.
				de_total_size = de_size + de_size_temp;

			}
			else{

				err_what = "decrypt data final fail.";
				err_code = error_security_des_cbc_encrypt_is_wrong;

				EVP_CIPHER_CTX_cleanup(&ctx);

				return -1;
			}

			EVP_CIPHER_CTX_cleanup(&ctx);
		}


		char s_de[de_total_size + 1];
		memset(s_de, 0x00, de_total_size + 1);
		memcpy(s_de, de, de_total_size);

		out.append(&s_de[0]);

		return 0;
	}

	int decrypt(const char* in, size_t in_size, mem_guard<MEMORY_BLOCK>& block, string& err_what, u16& err_code){
		u8 s_input[in_size];
		for (size_t i = 0; i < in_size; ++i) {
			s_input[i] = (u8)(*(in + i));
		}

		u8* key = 0;
		u8* iv = 0;
		key_->key(key);
		key_->iv(iv);


		u8 de[block.size()];//解码缓冲区
		int de_size;
		int de_size_temp;
		int de_total_size;//解码总长度
		memset(de, 0x00, block.size());

		// 解密
		{
			EVP_CIPHER_CTX ctx;//evp算法上下文
			EVP_CIPHER_CTX_init(&ctx);

			int result = EVP_DecryptInit_ex(&ctx,EVP_des_cbc(),0,key,iv);

			if(result){
			}
			else{

				err_what = "decrypt data init fail.";
				err_code = error_security_des_cbc_encrypt_is_wrong;

				EVP_CIPHER_CTX_cleanup(&ctx);

				return -1;
			}


			result = EVP_DecryptUpdate(&ctx, de, &de_size, s_input, in_size);

			if(result){
			}
			else{

				err_what = "decrypt data update fail.";
				err_code = error_security_des_cbc_encrypt_is_wrong;

				EVP_CIPHER_CTX_cleanup(&ctx);

				return -1;
			}

			result = EVP_DecryptFinal_ex(&ctx, de + de_size, &de_size_temp);

			if(result){

				// Get total size.
				de_total_size = de_size + de_size_temp;

			}
			else{

				err_what = "decrypt data final fail.";
				err_code = error_security_des_cbc_encrypt_is_wrong;

				EVP_CIPHER_CTX_cleanup(&ctx);

				return -1;
			}

			EVP_CIPHER_CTX_cleanup(&ctx);
		}

		char output_[de_total_size];
		memcpy(output_, de, de_total_size);


		int res = block.copy(&output_[0], de_total_size, err_code);
		if(res == -1) err_what = "decrypt data copy block fail.";
		return res;
	}

private:
	des_cbc_key* key_;
};

} }

#endif /* SECURITY_DES_CBC_H_ */
