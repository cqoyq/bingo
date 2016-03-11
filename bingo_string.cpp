/*
 * bingo_string.cpp
 *
 *  Created on: 2016-1-27
 *      Author: root
 */

#include <boost/test/unit_test.hpp>

#include <iostream>
using namespace std;

#include "cost_time.h"

#include "bingo/pb.h"
using namespace bingo;

#include <boost/thread.hpp>
using namespace boost;

BOOST_AUTO_TEST_SUITE(bingo_string)

BOOST_AUTO_TEST_CASE(t_stream_to_string){

}

BOOST_AUTO_TEST_CASE(t_ip_to_int){
	string ip("255.230.200.100");

	stringt t;
	u32 v = t.ip_to_int(ip);

	cout << t.int_to_ip(v) << endl;
	// output:
	//  255.230.200.100
}

BOOST_AUTO_TEST_CASE(t_short_to_stream){

	u16 v = 260;
	stringt t;
	const char* p = t.short_to_stream(v);
	char out[2];
	memcpy(&out[0], p, 2);
	cout << "out stream:" << t.stream_to_string(out, 2) << endl;

	u16 v1 = t.stream_to_short(out);
	cout << "out u16:" << v1 << endl;
	// output:
	//	out stream:04 01
	//	out u16:260
}

BOOST_AUTO_TEST_CASE(t_convert){

	u8 ch = 0x02;
	u16 a = 1;
	u32 b = 22;
	u64 c = 333;

	bingo::stringt t;
	cout << "char:" << t.convert<u16>(ch) << endl;
	cout << "short:" << t.convert(a) << endl;
	cout << "int:" << t.convert(b) << endl;
	cout << "long:" << t.convert(c) << endl;
	cout << "pointer:" << t.convert(&c) << endl;
	// output:
	//	char:2
	//	short:1
	//	int:22
	//	long:333
	//	pointer:0x7feffe210


	string c_ch, c_a, c_b, c_c, c_pointer;
	t.convert<u16>(ch, c_ch);
	t.convert(a, c_a);
	t.convert(b, c_b);
	t.convert(c, c_c);
	t.convert(&c, c_pointer);
	cout << "char:" << c_ch << endl;
	cout << "short:" << c_a << endl;
	cout << "int:" << c_b << endl;
	cout << "long:" << c_c << endl;
	cout << "pointer:" << c_pointer << endl;
	// output:
	//	char:2
	//	short:1
	//	int:22
	//	long:333
	//	pointer:0x7feffe210

	cout << "thread_id:" << t.convert(this_thread::get_id()) << endl;
	// output:
	//	thread_id:570a2e0


}

BOOST_AUTO_TEST_SUITE_END()


