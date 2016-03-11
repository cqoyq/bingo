/*
 * bingo_random.cpp
 *
 *  Created on: 2016-3-8
 *      Author: root
 */

#include <boost/test/unit_test.hpp>

#include <iostream>
using namespace std;

#include "cost_time.h"

#include "bingo/security/all.h"
using namespace bingo;
using namespace bingo::security;

BOOST_AUTO_TEST_SUITE(bingo_random)

#pragma pack(1)
struct random_mblk{
	char data[16];
};
#pragma pack()

struct my_parser{
	static const char* chars;
};
const char* my_parser::chars = "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "1234567890"
        "!@#$%^&*()"
        "`~-_=+[{]}\\|;:'\",<.>/?";

BOOST_AUTO_TEST_CASE(t_boost_random){

	typedef boost_random_algorithm<
			random_mblk,
//			boost_random_parser
			my_parser
			> algorithm_type;
	typedef singleton_v0<security::random_generator<algorithm_type, random_mblk> > rgn;

	rgn::create();

	int n = 0;
	while(n < 5){
		mem_guard<random_mblk> pk;
		rgn::instance()->make(pk);

		stringt t;
		cout << "random stream:" << t.stream_to_string(pk.header(), pk.length()) << endl;
		cout << "random string:" << t.stream_to_char(pk.header(), pk.length()) << endl;
		cout << endl;

		n++;
	}
	rgn::release();

	// output:
	//	random stream:66 59 48 61 65 64 38 78 74 38 38 4d 6c 65 6b 75
	//	random string:fYHaed8xt88Mleku

	//	random stream:58 77 6b 34 62 48 48 77 4c 78 38 69 6f 34 53 75
	//	random string:Xwk4bHHwLx8io4Su

	//	random stream:33 78 31 55 33 6a 50 4b 67 32 42 45 55 4a 68 61
	//	random string:3x1U3jPKg2BEUJha

	//	random stream:64 65 68 68 41 46 45 68 63 6e 77 4f 4a 38 36 39
	//	random string:dehhAFEhcnwOJ869

	//	random stream:6c 30 72 6a 36 55 73 33 79 46 37 78 49 6b 31 4d
	//	random string:l0rj6Us3yF7xIk1M

}

BOOST_AUTO_TEST_CASE(t_uuid){

	// random_generator
	{
		typedef singleton_v0<security::random_generator<uuid_random_algorithm, uuid_random_mblk> > rgn;

		rgn::create();

		int n = 0;
		while(n < 5){

			mem_guard<uuid_random_mblk> pk;
			rgn::instance()->make(pk);

			stringt t;
			cout << "random stream:" << t.stream_to_string(pk.header(), pk.length()) << endl;
			cout << "random string:" << t.convert(make_uuid(pk.header())) << endl;
			cout << endl;

			n++;
		}

		rgn::release();

		// output:
		//		random stream:af a9 1d c6 b8 64 43 7a a0 07 fd cc a4 09 75 3b
		//		random string:afa91dc6-b864-437a-a007-fdcca409753b
		//
		//		random stream:64 9e 32 bf f7 d9 44 c4 97 38 0b af 97 7b 24 83
		//		random string:649e32bf-f7d9-44c4-9738-0baf977b2483
		//
		//		random stream:ce be 0e 9c 81 e4 4d 25 b0 80 75 19 4d 30 c3 f0
		//		random string:cebe0e9c-81e4-4d25-b080-75194d30c3f0
		//
		//		random stream:6e fa 43 e5 55 9d 45 8d 8a 88 23 d1 cf 92 d6 c9
		//		random string:6efa43e5-559d-458d-8a88-23d1cf92d6c9
		//
		//		random stream:2c 9a 59 8c 6c c1 4f 71 ae a6 c8 b1 65 c5 c3 fb
		//		random string:2c9a598c-6cc1-4f71-aea6-c8b165c5c3fb
	}

	// uuid_name_generator
	{
		uuid_name_generator<> gen;
		int n = 0;
		while(n < 5){

			string istr = lexical_cast<string>(n);

			mem_guard<uuid_random_mblk> pk;
			gen.make(istr.c_str(), pk);

			stringt t;
			cout << "uuid_name_generator stream:" << t.stream_to_string(pk.header(), pk.length()) << endl;
			cout << "uuid_name_generator string:" << t.convert(make_uuid(pk.header())) << endl;
			cout << endl;

			n++;
		}

		// output:
		//		uuid_name_generator stream:77 8c 25 69 a1 49 5d e5 bf 7b 22 a4 3b 6f 77 60
		//		uuid_name_generator string:778c2569-a149-5de5-bf7b-22a43b6f7760
		//
		//		uuid_name_generator stream:08 fb 8d 7b 9e 55 50 f3 b5 5c 4c f0 98 d0 b2 38
		//		uuid_name_generator string:08fb8d7b-9e55-50f3-b55c-4cf098d0b238
		//
		//		uuid_name_generator stream:1b 61 32 0f 0b 1f 5b eb 8f e5 e9 ee 5b 7f 2e eb
		//		uuid_name_generator string:1b61320f-0b1f-5beb-8fe5-e9ee5b7f2eeb
		//
		//		uuid_name_generator stream:14 a0 ad 92 77 93 53 73 ae c7 fd d6 b3 f5 07 12
		//		uuid_name_generator string:14a0ad92-7793-5373-aec7-fdd6b3f50712
		//
		//		uuid_name_generator stream:09 de f7 45 90 f0 52 fc ba b0 f6 7f 2c 79 1d 87
		//		uuid_name_generator string:09def745-90f0-52fc-bab0-f67f2c791d87
	}
}

BOOST_AUTO_TEST_SUITE_END()
