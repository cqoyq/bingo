/*
 * symmetric_encryptor.h
 *
 *  Created on: 2016-3-7
 *      Author: root
 */

#ifndef SECURITY_SYMMETRIC_ENCRYPTOR_H_
#define SECURITY_SYMMETRIC_ENCRYPTOR_H_

#include "../pb.h"
using bingo::u32;
using bingo::mem_guard;

#include "../xthread/spinlock.h"
using bingo::xthread::spinlock;

#include "Ihandler.h"

namespace bingo { namespace security {

template<typename ALGORITHM, typename MEMORY_BLOCK>
class symmetric_encryptor : public IEncryptorHandler<MEMORY_BLOCK>{
public:
	symmetric_encryptor(): IEncryptorHandler<MEMORY_BLOCK>(){
		p_ = new ALGORITHM();
	}
	virtual ~symmetric_encryptor(){
		delete p_;
	}

public:
	int encrypt(string& in, mem_guard<MEMORY_BLOCK>& out, string& err_what, u16& err_code){
		spinlock::scoped_lock lock(mu_);
		return p_->encrypt(in, out, err_what, err_code);
	}
	int encrypt(const char* in, size_t in_size, mem_guard<MEMORY_BLOCK>& out, string& err_what, u16& err_code){
		spinlock::scoped_lock lock(mu_);
		return p_->encrypt(in, in_size, out, err_what, err_code);
	}

	int decrypt(const char* in, size_t in_size, string& out, string& err_what, u16& err_code){
		spinlock::scoped_lock lock(mu_);
		return p_->decrypt(in, in_size, out, err_what, err_code);
	}
	int decrypt(const char* in, size_t in_size, mem_guard<MEMORY_BLOCK>& out, string& err_what, u16& err_code){
		spinlock::scoped_lock lock(mu_);
		return p_->decrypt(in, in_size, out, err_what, err_code);
	}
private:
	ALGORITHM* p_;
	spinlock mu_;

};

} }

#endif /* SECURITY_SYMMETRIC_ENCRYPTOR_H_ */
