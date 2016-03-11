/*
 * spinlock.h
 *
 *  Created on: 2016-1-26
 *      Author: root
 */

#ifndef THREAD_SPINLOCK_H_
#define THREAD_SPINLOCK_H_

#include <boost/atomic.hpp>

namespace bingo { namespace xthread {

class spinlock {
private:
	typedef enum {Locked, Unlocked} LockState;
	boost::atomic<LockState> state_;
public:
	typedef void(*f)();

	class scoped_lock{
	public:
		scoped_lock(spinlock& lock){
			p_ = &lock;
			p_->lock();
		}
		~scoped_lock(){
			p_->unlock();
		}
	private:
		spinlock* p_;
	};
public:
	spinlock() : state_(Unlocked) {}

	void lock()
	{
		while (state_.exchange(Locked, boost::memory_order_acquire) == Locked) {
		  /* busy-wait */
		}
	}
	void unlock()
	{
		state_.store(Unlocked, boost::memory_order_release);
	}
};

} }


#endif /* THREAD_SPINLOCK_H_ */
