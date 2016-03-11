/*
 * mem_pool.h
 *
 *  Created on: 2016-1-19
 *      Author: root
 */

#ifndef MEM_POOL_H_
#define MEM_POOL_H_

#include <vector>
#include <queue>
using namespace std;

namespace bingo {

// T is assign struct-type.
template<class T>
class mem_pool_vector{
public:
	typedef vector<void*>::iterator set_iterator;

	mem_pool_vector(u32& container_size){

		sets_.assign(container_size, 0x00);

		// Assign elements into sets_.
		size_t size = container_size * sizeof(T);
		void* p = malloc(size);
		memset(p, 0x00, size);

		for (u32 i = 0; i < container_size; ++i) {

			char* p1 = static_cast<char*>(p);
			p1 += i * sizeof(T);

			sets_[i] = p1;
		}
	}

	virtual ~mem_pool_vector(){
		if(sets_.size() > 0){
			free(sets_[0]);
		}
	}

	size_t size(){
		return sets_.size();
	}

protected:
	void* header_pointer() const{
		return (sets_.size() == 0)? 0x00:sets_[0];
	}

	vector<void*> sets_;
};

// T is assign struct-type.
template<class T>
class mem_pool_queue{
public:
	mem_pool_queue():queue_header_(0){}
	mem_pool_queue(u32& container_size){

		// Assign elements into sets_.
		size_t size = container_size * sizeof(T);
		queue_header_ = malloc(size);
		memset(queue_header_, 0x00, size);

		for (u32 i = 0; i < container_size; ++i) {

			char* p1 = static_cast<char*>(queue_header_);
			p1 += i * sizeof(T);

			sets_.push(p1);
		}
	}

	virtual ~mem_pool_queue(){
		if(queue_header_){
			free(queue_header_);
		}
	}

	size_t size(){
		return sets_.size();
	}

	size_t block_size(){

		return sizeof(T);
	}

protected:
	void* header_pointer() const{
		return queue_header_;
	}

	queue<void*> sets_;

	void* queue_header_;
};

}


#endif /* MEM_POOL_H_ */
