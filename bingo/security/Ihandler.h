/*
 * Ihandler.h
 *
 *  Created on: 2016-3-7
 *      Author: root
 */

#ifndef SECURITY_IHANDLER_H_
#define SECURITY_IHANDLER_H_

#include "../pb.h"
using bingo::u32;
using bingo::mem_guard;

namespace bingo { namespace security {

template<typename MEMORY_BLOCK>
class IEncryptorHandler{
public:
	virtual ~IEncryptorHandler(){}

	virtual int encrypt(string& in, mem_guard<MEMORY_BLOCK>& out, string& err_what, u16& err_code) = 0;
	virtual int encrypt(const char* in, size_t in_size, mem_guard<MEMORY_BLOCK>& out, string& err_what, u16& err_code) = 0;
	virtual int decrypt(const char* in, size_t in_size, string& out, string& err_what, u16& err_code) = 0;
	virtual int decrypt(const char* in, size_t in_size, mem_guard<MEMORY_BLOCK>& out, string& err_what, u16& err_code) = 0;
};

template<typename MEMORY_BLOCK>
class IRandomHandler{
public:
	virtual ~IRandomHandler(){}

	virtual void make(mem_guard<MEMORY_BLOCK>& out) = 0;
};

} }

#endif /* SECURITY_IHANDLER_H_ */
