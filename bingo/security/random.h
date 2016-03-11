/*
 * random.h
 *
 *  Created on: 2016-3-8
 *      Author: root
 */

#ifndef SECURITY_RANDOM_H_
#define SECURITY_RANDOM_H_

#include "../xthread/spinlock.h"
using bingo::xthread::spinlock;

#include "Ihandler.h"

namespace bingo { namespace security {

template<typename ALGORITHM, typename MEMORY_BLOCK>
class random_generator : public IRandomHandler<MEMORY_BLOCK>{
public:
	random_generator(): IRandomHandler<MEMORY_BLOCK>(){
		p_ = new ALGORITHM();
	};
	virtual ~random_generator(){
		delete p_;
	}

public:
	void make(mem_guard<MEMORY_BLOCK>& out){
		spinlock::scoped_lock lock(mu_);
		p_->make(out);
	}
private:
	ALGORITHM* p_;
	spinlock mu_;

};

} }


#endif /* RANDOM_H_ */
