/*
 * bingo_atomic_lock.cpp
 *
 *  Created on: 2016-1-26
 *      Author: root
 */


#include <boost/test/unit_test.hpp>

#include <iostream>
using namespace std;

#include "cost_time.h"

#define BINGO_TCP_ATOMIC_VERSION

#include "bingo/pb.h"
using namespace bingo;
//using namespace bingo::internet::tcp;

BOOST_AUTO_TEST_SUITE(bingo_atomic_lock)

#pragma pack(1)
struct package{
	char data[4];
};
#pragma pack()

//typedef tcp_data_alloction<package> my_allocation;
//typedef bingo::singleton_v1<my_allocation, u32> my1;
//
//void put(){
//
//	u32 max = 600 * 4;
//	for (u32 i = 0; i < max; ++i) {
//
//		this_thread::sleep(milliseconds(10));
//
//		char* out = 0;
//		u16 err_code = 0;
//		if(my1::instance()->top(out, err_code) == 0){
//
//			this_thread::sleep(milliseconds(10));
//			my1::instance()->push(out);
//		}else{
//			cout << "put(),err_code:" << err_code << endl;
//		}
//
//	}
//}

BOOST_AUTO_TEST_CASE(t_nthread_lock){

//	my1::create(1000);
//
//	SHOW_CURRENT_TIME("start")
//	thread t1(put);
//	thread t2(put);
//	thread t3(put);
//	thread t4(put);
//
//	t1.join();
//	t2.join();
//	t3.join();
//	t4.join();
//	SHOW_CURRENT_TIME("end")
//
//	BOOST_CHECK(my1::instance()->size() == 1000u);
//
//	my1::release();

	// output:
	//  atomic version
	//	start time:2016-01-26T21:16:47
	//	end   time:2016-01-26T21:17:42   // 55s


	//	start time:2016-01-26T21:13:54
	//	end   time:2016-01-26T21:14:49	 // 55s

}

BOOST_AUTO_TEST_SUITE_END()

