/*
 * tcp_data.h
 *
 *  Created on: 2016-1-19
 *      Author: root
 */

#ifndef TCP_DATA_H_
#define TCP_DATA_H_

namespace bingo { namespace internet { namespace tcp {

class tcp_data_manager{
public:
	tcp_data_manager()
		: header_(0),
		  size_(0),
		  length_(0){}

	// Init header_ and size_.
	void init(char*& data, size_t& data_size){
		header_ = data;
		size_ = data_size;
	}

	// Clear old data.
	void clear(){

		memset(header_, 0x00, size_);
		length_ = 0;
	}

	// Return header pointer of block.
	char*& header(){
		return header_;
	}

	// Return length of data.
	size_t& length(){
		return length_;
	}

	// Return current use pointer of block.
	char* current() const{
		return header_ + length_;
	}

	// Increase length_
	int increase_length(size_t len, u16& err_code){
		if((len + length_) > size_){
			err_code = error_tcp_package_rev_exceed_max_size;
			return -1;
		}

		length_ += len;

		return 0;
	}

private:
	char* header_;
	size_t size_;

	size_t length_;
};

} } }

#endif /* TCP_DATA_H_ */
