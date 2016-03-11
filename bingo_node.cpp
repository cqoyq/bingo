/*
 * bingo_node.cpp
 *
 *  Created on: 2016-2-19
 *      Author: root
 */

#include <boost/test/unit_test.hpp>

#include <iostream>
using namespace std;

#include "cost_time.h"

#include "bingo/cfg/all.h"
using namespace bingo;

using bingo::cfg::node;
using bingo::cfg::node_set;
using bingo::cfg::node_attr;
using bingo::cfg::node_attr_set;
using bingo::cfg::xml::xml_parse_handler;
using bingo::cfg::json::json_parse_handler;
using bingo::cfg::ini::ini_parse_handler;

BOOST_AUTO_TEST_SUITE(bingo_node)

node g_node;

// 测试夹具，每个测试用例都会调用
struct s_bingo_node_fixture
{
	//测试用例执行前调用
	s_bingo_node_fixture()
	{
		g_node.name = "application";

		node* item1 = new node("gateway");
		item1->attrs.push_back(new node_attr("id","1"));
		item1->child.push_back(new node("has_deploy","0"));
		{
			node* ips = item1->child.push_back(new node("ips"));
			ips->child.push_back(new node("ip"))->attrs.push_back(new node_attr("value", "110.1.0.0/16"));
			ips->child.push_back(new node("ip"))->attrs.push_back(new node_attr("value", "110.2.0.0/16"));
		}
		g_node.child.push_back(item1);


		node* item2 = new node("phone");
		item2->child.push_back(new node("has_deploy", "1"));
		{
			node* ips = item2->child.push_back(new node("ips"));
			ips->child.push_back(new node("ip"))->attrs.push_back(new node_attr("value", "220.1.0.0/16"));
			ips->child.push_back(new node("ip"))->attrs.push_back(new node_attr("value", "220.2.0.0/16"));
		}
		g_node.child.push_back(item2);

		node* item3 = new node("name", "this is a test");
		g_node.child.push_back(item3);

		node* item4 = new node("items");
		item4->child.push_back(new node("item", "one"));
		item4->child.push_back(new node("item", "two"));
		item4->child.push_back(new node("item", "three"));
		g_node.child.push_back(item4);
	}

	//测试用例测试后调用
	~s_bingo_node_fixture()
	{


	}
};

BOOST_FIXTURE_TEST_SUITE(sxx,s_bingo_node_fixture)

BOOST_AUTO_TEST_CASE(t_xml){
	// 测试写
	stringstream stream;
	g_node.write<xml_parse_handler<> >(stream);		// write to stream

	cout << stream.str() << endl;
	// output:
	//	<?xml version="1.0" encoding="utf-8"?>
	//	<application>
	//		<gateway id="1">
	//			<has_deploy>0</has_deploy>
	//			<ips>
	//				<ip value="110.1.0.0/16"/>
	//				<ip value="110.2.0.0/16"/>
	//			</ips>
	//		</gateway>
	//		<phone>
	//			<has_deploy>1</has_deploy>
	//			<ips>
	//				<ip value="220.1.0.0/16"/>
	//				<ip value="220.2.0.0/16"/>
	//			</ips>
	//		</phone>
	//		<name>this is a test</name>
	//		<items>
	//			<item>one</item>
	//			<item>two</item>
	//			<item>three</item>
	//		</items>
	//	</application>

	g_node.write<xml_parse_handler<> >("./confw.xml");		// write to file

	// 测试读
	string err;
	xml_parse_handler<> xml;
	BOOST_CHECK_EQUAL(xml.read("./confw.xml", err), 0);

	BOOST_CHECK(xml.get_value("application.ab", err) == 0);							// read error
	cout << "read value error:" << err << endl;
	// output:
	//	read value error:No such node (application.ab)

	BOOST_CHECK_EQUAL(xml.get_value("application.name", err), "this is a test");

	BOOST_CHECK(xml.get_attr("application.gateways", "t", err) == 0);				// read error
	cout << "read attr of node error:" << err << endl;
	// output:
	//	read attr of node error:No such node (application.gateways)

	BOOST_CHECK(xml.get_attr("application.gateway", "t", err) == 0);				// read error
	cout << "read attr error:" << err << endl;
	// output:
	//  read attr error:attribute isn't exit!

	BOOST_CHECK(strcmp(xml.get_attr("application.gateway", "id", err), "1") == 0);

	BOOST_CHECK(xml.get_node("application.gateway.qa", err) == 0);					// read error
	cout << "read node error:" << err << endl;
	// output:
	//	read node error:No such node (application.gateway.qa)

	node* n = xml.get_node("application.gateway.ips", err);
	BOOST_CHECK(n->child.set[0].attrs.set[0].name.compare("value") == 0);
	BOOST_CHECK(n->child.set[0].attrs.set[0].value.compare("110.1.0.0/16") == 0);

	BOOST_CHECK(n->child.set[1].attrs.set[0].name.compare("value") == 0);
	BOOST_CHECK(n->child.set[1].attrs.set[0].value.compare("110.2.0.0/16") == 0);
}

// Json parse without attribute
BOOST_AUTO_TEST_CASE(t_json){

	// 写测试
	stringstream stream;
	g_node.write<json_parse_handler<> >(stream);		// write to stream

	cout << stream.str() << endl;
	// output:
	//	{
	//	    "application": {
	//	        "gateway": {
	//	            "has_deploy": "0",
	//	            "ips": {
	//	                "ip": "",
	//	                "ip": ""
	//	            }
	//	        },
	//	        "phone": {
	//	            "has_deploy": "1",
	//	            "ips": {
	//	                "ip": "",
	//	                "ip": ""
	//	            }
	//	        },
	//	        "name": "this is a test",
	//	        "items": {
	//	            "item": "one",
	//	            "item": "two",
	//	            "item": "three"
	//	        }
	//	    }
	//	}


	g_node.write<json_parse_handler<> >("./jsonw.xml");		// write to file

	// 读测试
	string err;
	json_parse_handler<> json;
	BOOST_CHECK_EQUAL(json.read("./jsonw.xml", err), 0);

	BOOST_CHECK(strcmp(json.get_value("application.gateway", err), "") == 0);
	BOOST_CHECK(strcmp(json.get_value("application.gateway.has_deploy", err), "0") == 0);

	BOOST_CHECK(json.get_value("application.gateways", err) == 0);							// read error
	cout << "read value error:" << err << endl;
	// output:
	//	read value error:No such node (application.gateways)

	BOOST_CHECK(json.get_node("application.gateway.qa", err) == 0);					// read error
	cout << "read node error:" << err << endl;
	// output:
	//	read node error:No such node (application.gateway.qa)

	node* n = json.get_node("application.items", err);
	BOOST_CHECK(n->child.set[0].value.compare("one") == 0);
	BOOST_CHECK(n->child.set[1].value.compare("two") == 0);
	BOOST_CHECK(n->child.set[2].value.compare("three") == 0);
}


/*
 * Requires:
 * 	1.pt cannot have data in its root.
 * 	2.pt cannot have keys both data and children
 * 	3.pt cannot be deeper than two levels
 */
BOOST_AUTO_TEST_CASE(t_ini){

	// 写测试
	node mynode;
	node* node1 = mynode.child.push_back(new node("gateway"));
	node1->child.push_back(new node("start_family_id", "1"));
	node1->child.push_back(new node("end_family_id", "1000"));
	node1->child.push_back(new node("listen_port", "17005"));
	node1->child.push_back(new node("local_ip", "127.0.0.1"));

	node* node2 = mynode.child.push_back(new node("log_server"));
	node2->child.push_back(new node("remote_ip", "127.0.0.1"));
	node2->child.push_back(new node("remote_port", "17015"));

	stringstream stream;
	mynode.write<ini_parse_handler>(stream);		// write to stream

	cout << stream.str() << endl;
	// output:
	//	[gateway]
	//		start_family_id=1
	//		end_family_id=1000
	//		listen_port=17005
	//		local_ip=127.0.0.1
	//	[log_server]
	//		remote_ip=127.0.0.1
	//		remote_port=17015

	mynode.write<ini_parse_handler>("./iniw.xml");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

