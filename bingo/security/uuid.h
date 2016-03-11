/*
 * uuid.h
 *
 *  Created on: 2016-3-9
 *      Author: root
 */

#ifndef SECURITY_UUID_H_
#define SECURITY_UUID_H_

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
using namespace boost::uuids;

#include "../xthread/spinlock.h"
using bingo::xthread::spinlock;

namespace bingo { namespace security {

#pragma pack(1)
struct uuid_random_mblk{
	char data[16];
};
#pragma pack()

struct uuid_name_parser{
	static const char* chars;
};

template<typename PARSER = uuid_name_parser>
class uuid_name_generator{
public:
	uuid_name_generator(){
		www_xxx_com = string_generator()(PARSER::chars);
		ngen_ = new name_generator(www_xxx_com);
	};
	virtual ~uuid_name_generator(){
		delete ngen_;
	}

public:
	void make(const char* in, mem_guard<uuid_random_mblk>& out){
		uuid u1 = (*ngen_)(in);

		u16 err_code = 0;
		out.copy(u1.data, u1.size(), err_code);
	}

private:
	name_generator* ngen_;
	uuid www_xxx_com;
};

class uuid_random_algorithm : public IRandomHandler<uuid_random_mblk>{
public:
	uuid_random_algorithm(): IRandomHandler<uuid_random_mblk>(){}

public:
	void make(mem_guard<uuid_random_mblk>& out){
		uuid u = rgen_();

		u16 err_code = 0;
		out.copy(u.data, u.size(), err_code);
	}
private:
	boost::uuids::random_generator rgen_;
};

extern uuid make_uuid(const char* data_header);

} }

#endif /* SECURITY_UUID_H_ */
