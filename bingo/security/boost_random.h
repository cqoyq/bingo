/*
 * boost_random.h
 *
 *  Created on: 2016-3-8
 *      Author: root
 */

#ifndef SECURITY_BOOST_RANDOM_H_
#define SECURITY_BOOST_RANDOM_H_

#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "../pb.h"
using namespace bingo;

#include "Ihandler.h"

namespace bingo { namespace security {

#pragma pack(1)
struct boost_random_mblk{
	char data[16];
};
#pragma pack()

struct boost_random_parser{
	static const char* chars;
};

template<typename MEMORY_BLOCK,
		 typename PARSER>
class boost_random_algorithm : public IRandomHandler<MEMORY_BLOCK>{
public:
	boost_random_algorithm():IRandomHandler<MEMORY_BLOCK>(){
		chars_ = PARSER::chars;
	};
public:
	void make(mem_guard<MEMORY_BLOCK>& out){
		boost::random::uniform_int_distribution<> index_dist(0, chars_.size() - 1);

		char* data = out.header();
		size_t size = out.size();

		for(size_t i = 0; i < size; ++i) {
			data[i] = chars_[index_dist(rng_)];
		}

		u16 err_code = 0;
		out.change_length(size, err_code);
	}

private:
	string chars_;
	boost::random::random_device rng_;
};

} }


#endif /* SECURITY_BOOST_RANDOM_H_ */
