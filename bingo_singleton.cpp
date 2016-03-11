/*
 * bingo_singleton.cpp
 *
 *  Created on: 2016-2-2
 *      Author: root
 */

#include <boost/test/unit_test.hpp>

#include <iostream>
using namespace std;

#include "cost_time.h"

#include "bingo/singleton.h"

BOOST_AUTO_TEST_SUITE(bingo_singleton)

class test{};

class A : public test{};
class B : public test{};

typedef bingo::singleton_v0<A> type_a;
typedef bingo::singleton_v0<B> type_b;

BOOST_AUTO_TEST_CASE(t){

	type_a::create();
	type_b::create();

	cout << "type_a::instance():" << type_a::instance() << endl;
	cout << "type_b::instance():" << type_b::instance() << endl;
	// output:
	//	type_a::instance():0x574f050
	//	type_b::instance():0x574f0a0

	type_a::release();
	type_b::release();
}

BOOST_AUTO_TEST_SUITE_END()


