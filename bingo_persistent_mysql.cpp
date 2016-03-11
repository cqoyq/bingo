/*
 * bingo_persistent_mysql.cpp
 *
 *  Created on: 2016-2-17
 *      Author: root
 */

#include <boost/test/unit_test.hpp>

#include <iostream>
using namespace std;

#include "cost_time.h"

#include "bingo/xthread/all.h"
using namespace bingo::xthread;
using namespace bingo::xthread::task;

#define BINGO_PERSISTENT_MYSQL_DEBUG
#include "bingo/persistent/mysql/all.h"
using namespace bingo;
using namespace bingo::persistent;

#include <boost/thread.hpp>
using namespace boost;

BOOST_AUTO_TEST_SUITE(bingo_persistent_mysql)

struct connect_info{
	static const char* ip;
	static const char* user;
	static const char* pwd;
	static const char* dbname;
	static const u32 port;
};
const char* connect_info::ip = "127.0.0.1";
const char* connect_info::user = "root";
const char* connect_info::pwd = "1234";
const char* connect_info::dbname = "xhome_community";
const u32 connect_info::port = 3306;

typedef bingo::singleton_v0<
			mysql::handler<connect_info>
			> mydb;

/*
 * 1.测试mysql链接是否成功
 * 2.测试mysql链接成功，发送心跳
 */

void run(){

	mydb::create();

	string err;
	int res = mydb::instance()->create(err);
	if(res == -1)
		cout << "create pool fail, error:" << err << endl;
	else
		mydb::instance()->start_heartjump();	// Start heart-jump io_service

	SHOW_CURRENT_TIME("io_service thread exit!")
	mydb::release();
}

BOOST_AUTO_TEST_CASE(t_base){

	thread t(run);

	this_thread::sleep(seconds(10));

	SHOW_CURRENT_TIME("start to stop heartjump!")
	mydb::instance()->stop_heartjump();	// Stop heart-jump io_service

	t.join();
	SHOW_CURRENT_TIME("stop heartjump finish!")

	// output:
	//	start to stop heartjump! 	time:2016-02-19T11:47:29
	//	io_service thread exit! 	time:2016-02-19T11:47:29
	//	stop heartjump finish! 		time:2016-02-19T11:47:30

}

/*
 * 测试心跳超时
 */
BOOST_AUTO_TEST_CASE(t_timeout){

	thread t(run);

	this_thread::sleep(seconds(5));

	int n = 0;
	while(n < 4){

		db_result result;
		string err;
		string sql("select version();");
		BOOST_CHECK_EQUAL(mydb::instance()->query_result(sql, result, err), 0);

		SHOW_CURRENT_TIME("test")
		this_thread::sleep(seconds(15));
		n++;
	}

	this_thread::sleep(seconds(60*2));

	SHOW_CURRENT_TIME("start to stop heartjump!")
	mydb::instance()->stop_heartjump();	// Stop heart-jump io_service

	t.join();
	SHOW_CURRENT_TIME("stop heartjump finish!")

	// output:
	//	test 							time:2016-02-19T11:48:11
	//	test 							time:2016-02-19T11:48:26
	//	test 							time:2016-02-19T11:48:41
	//	test 							time:2016-02-19T11:48:56
	//	mysql send heartjump success, 	time:2016-02-19T11:50:06
	//	mysql send heartjump success, 	time:2016-02-19T11:51:06
	//	start to stop heartjump! 		time:2016-02-19T11:51:11
	//	io_service thread exit! 		time:2016-02-19T11:51:11
	//	stop heartjump finish! 			time:2016-02-19T11:51:11
}

/*
 * 测试sql操作
 */
BOOST_AUTO_TEST_CASE(t_sql){

	string err;
	mysql::handler<connect_info> hdr;
	hdr.create(err);

	this_thread::sleep(seconds(5));

	// 测试单表操作
	{

		string sql("insert into T_BS_DeviceInfo (F_FamilyID,F_Name,F_TypeID,F_DeviceID,F_SID,F_Data,F_isValid) values (1,'设备1',1,1,1,20,1);");
		BOOST_CHECK_EQUAL(hdr.query_result(sql, err), 0);

		sql.clear();
		sql.append("insert into T_BS_DeviceInfo (F_FamilyID,F_Name,F_TypeID,F_DeviceID,F_SID,F_Data,F_isValid) values (1,'设备2',2,2,2,40,0);");
		BOOST_CHECK_EQUAL(hdr.query_result(sql, err), 0);

		sql.clear();
		sql.append("insert into T_BS_DeviceInfo (F_FamilyID,F_Name,F_TypeID,F_DeviceID,F_SID,F_Data,F_isValid3) values (1,'设备1',1,1,1,20,1);");
		BOOST_CHECK_EQUAL(hdr.query_result(sql, err), -1);
		cout << "insert error:" << err << endl;
		// output:
		//	insert error:Unknown column 'F_isValid3' in 'field list'

		sql.clear();
		sql.append("select * from T_BS_DeviceInfo;");
		{
			db_result res;
			BOOST_CHECK_EQUAL(hdr.query_result(sql, res, err), 0);
			BOOST_CHECK_EQUAL(res.rows().size(), 2u);
			BOOST_CHECK_EQUAL(res.columns().size(), 8u);

			int i = 0;
			// get row's data in result.
			foreach_(db_row& n , res.rows()){
				db_field* f_familyid = n[1];
				int db_familyid = atoi(f_familyid->value());

				db_field* f_name = n[2];
				string db_name = f_name->value();

				db_field* f_type = n[3];
				int db_type = atoi(f_type->value());

				db_field* f_deviceid = n[4];
				int db_deviceid = atoi(f_deviceid->value());

				db_field* f_sid = n[5];
				int db_sid = atoi(f_sid->value());

				db_field* f_data = n[6];
				int db_data = atoi(f_data->value());

				db_field* f_isvalid = n[7];
				int db_isvalid = atoi(f_isvalid->value());

				if(i == 0){

					// Check gain field error
					{
						db_field* f_noexist = n[8];
						BOOST_CHECK(f_noexist == 0);

						db_field* f_noexist2 = n["F_familyid"];		// distinguish case by name
						BOOST_CHECK(f_noexist2 == 0);				// gain fail

						db_field* f_exist = n["F_FamilyID"];		// gain success
						BOOST_CHECK(f_exist != 0);
					}

//					BOOST_CHECK_EQUAL(db_autoid, 1);
					BOOST_CHECK_EQUAL(db_familyid, 1);
					BOOST_CHECK(db_name.compare("设备1") == 0);
					BOOST_CHECK_EQUAL(db_type, 1);
					BOOST_CHECK_EQUAL(db_deviceid, 1);
					BOOST_CHECK_EQUAL(db_sid, 1);
					BOOST_CHECK_EQUAL(db_data, 20);
					BOOST_CHECK_EQUAL(db_isvalid, 1);
				}else{
//					BOOST_CHECK_EQUAL(db_autoid, 2);
					BOOST_CHECK_EQUAL(db_familyid, 1);
					BOOST_CHECK(db_name.compare("设备2") == 0);
					BOOST_CHECK_EQUAL(db_type, 2);
					BOOST_CHECK_EQUAL(db_deviceid, 2);
					BOOST_CHECK_EQUAL(db_sid, 2);
					BOOST_CHECK_EQUAL(db_data, 40);
					BOOST_CHECK_EQUAL(db_isvalid, 0);
				}

				i++;

			}
		}

		sql.clear();
		sql.append("delete from T_BS_DeviceInfo;");
		BOOST_CHECK_EQUAL(hdr.query_result(sql, err), 0);
		return;

		sql.clear();
		sql.append("select * from T_BS_FamilyInfo;");
		{
			db_result res;
			BOOST_CHECK_EQUAL(hdr.query_result(sql, res, err), 0);
			BOOST_CHECK_EQUAL(res.rows().size(), 0u);
		}
	}

	// 测试多表操作
	{
		err.clear();

		string sql0("insert into T_BS_FamilyInfo (F_FamilyID,F_Name,F_LogName,F_Pwd,F_AdminPwd,F_Version,F_isValid) values (1,'家庭1','12345678','1234','5678',1234,1);");
		string sql1("insert into T_BS_DeviceInfo (F_FamilyID,F_Name,F_TypeID,F_DeviceID,F_SID,F_Data,F_isValid) values (1,'设备1',1,1,1,20,1);");
		string sql2("insert into T_BS_DeviceInfo (F_FamilyID,F_Name,F_TypeID,F_DeviceID,F_SID,F_Data,F_isValid) values (1,'设备2',2,2,2,40,0);");
		string sql_err("insert into T_BS_DeviceInfo (F_FamilyID,F_Name,F_TypeID,F_DeviceID,F_SID,F_Data,F_isValid2) values (1,'设备2',2,2,2,40,0);");

		vector<string> sqls;
		sqls.push_back(sql0);
		sqls.push_back(sql1);
		sqls.push_back(sql2);
		sqls.push_back(sql_err);

		BOOST_CHECK_EQUAL(hdr.query_result(sqls, err), -1);
		cout << "insert multi-sql error:" << err << endl;
		// output:
		//	insert multi-sql error:Unknown column 'F_isValid2' in 'field list'

		// Check no element in database.
		{
			sqls.clear();
			sqls.push_back(string("select * from T_BS_FamilyInfo;"));
			sqls.push_back(string("select * from T_BS_DeviceInfo;"));
			db_result_set res;
			BOOST_CHECK_EQUAL(hdr.query_result(sqls, res, err), 0);

			foreach_(db_result& n, res.results()){
				BOOST_CHECK_EQUAL(n.rows().size(), 0u);
			}
		}

		sqls.clear();
		sqls.push_back(sql0);
		sqls.push_back(sql1);
		sqls.push_back(sql2);
		BOOST_CHECK_EQUAL(hdr.query_result(sqls, err), 0);

		// Check element in database.
		{
			sqls.clear();
			sqls.push_back(string("select F_FamilyID, F_Name,F_LogName,F_Pwd,F_AdminPwd,F_Version,F_isValid from T_BS_FamilyInfo;"));
			sqls.push_back(string("select F_FamilyID, F_Name,F_TypeID,F_DeviceID,F_SID,F_Data,F_isValid from T_BS_DeviceInfo;"));
			db_result_set res;
			BOOST_CHECK_EQUAL(hdr.query_result(sqls, res, err), 0);

			BOOST_CHECK_EQUAL(res.results()[0].rows().size(), 1u);
			BOOST_CHECK_EQUAL(res.results()[1].rows().size(), 2u);

			foreach_(db_row& n, res.results()[0].rows()){
				db_field* f_familyid = n[0];
				int db_familyid = atoi(f_familyid->value());

				db_field* f_name = n[1];
				string db_name = f_name->value();

				db_field* f_logname = n[2];
				string db_logname = f_logname->value();

				db_field* f_pwd = n[3];
				string db_pwd = f_pwd->value();

				db_field* f_adminpwd = n[4];
				string db_adminpwd = f_adminpwd->value();

				db_field* f_version = n[5];
				int db_version = atoi(f_version->value());

				db_field* f_isvalid = n[6];
				int db_isvalid = atoi(f_isvalid->value());

				BOOST_CHECK_EQUAL(db_familyid, 1);
				BOOST_CHECK(db_name.compare("家庭1") == 0);
				BOOST_CHECK(db_logname.compare("12345678") == 0);
				BOOST_CHECK(db_pwd.compare("1234") == 0);
				BOOST_CHECK(db_adminpwd.compare("5678") == 0);
				BOOST_CHECK_EQUAL(db_version, 1234);
				BOOST_CHECK_EQUAL(db_isvalid, 1);
			}

			int i =0;
			foreach_(db_row& n, res.results()[1].rows()){
				db_field* f_familyid = n[0];
				int db_familyid = atoi(f_familyid->value());

				db_field* f_name = n[1];
				string db_name = f_name->value();

				db_field* f_type = n[2];
				int db_type = atoi(f_type->value());

				db_field* f_deviceid = n[3];
				int db_deviceid = atoi(f_deviceid->value());

				db_field* f_sid = n[4];
				int db_sid = atoi(f_sid->value());

				db_field* f_data = n[5];
				int db_data = atoi(f_data->value());

				db_field* f_isvalid = n[6];
				int db_isvalid = atoi(f_isvalid->value());

				if(i == 0){
					BOOST_CHECK_EQUAL(db_familyid, 1);
					BOOST_CHECK(db_name.compare("设备1") == 0);
					BOOST_CHECK_EQUAL(db_type, 1);
					BOOST_CHECK_EQUAL(db_deviceid, 1);
					BOOST_CHECK_EQUAL(db_sid, 1);
					BOOST_CHECK_EQUAL(db_data, 20);
					BOOST_CHECK_EQUAL(db_isvalid, 1);
				}else{
					BOOST_CHECK_EQUAL(db_familyid, 1);
					BOOST_CHECK(db_name.compare("设备2") == 0);
					BOOST_CHECK_EQUAL(db_type, 2);
					BOOST_CHECK_EQUAL(db_deviceid, 2);
					BOOST_CHECK_EQUAL(db_sid, 2);
					BOOST_CHECK_EQUAL(db_data, 40);
					BOOST_CHECK_EQUAL(db_isvalid, 0);
				}
				i++;
			}

		}

		sqls.clear();
		sqls.push_back(string("delete from T_BS_FamilyInfo;"));
		sqls.push_back(string("delete from T_BS_DeviceInfo;"));
		BOOST_CHECK_EQUAL(hdr.query_result(sqls, err), 0);
	}
}


/*
 * 测试mysql_pool
 */

typedef bingo::singleton_v1<mysql::pool<connect_info>, u32> mypool;



struct thr_tss : public thread_tss_data{
	bingo::stringt t;
	mysql::handler<connect_info>* hdr;
};

struct thr_parser{

	// task_message_data use
	static const int message_size;

	// atomic_imp_one_to_one use
	static const u32 thread_n;
};
const int thr_parser::message_size = 1024;
const u32 thr_parser::thread_n = 2;


typedef task_message_data<thr_parser> thr_data_message;
typedef task_exit_data<thr_parser>    thr_exit_message;

typedef thread_task<
			atomic_imp_one_to_many< thr_data_message,
									thr_exit_message,
									thr_parser,
									thr_tss>,
			thr_data_message,
			thr_tss
		> thr_task_type;
typedef bingo::singleton_v2<thr_task_type,
		thr_task_type::thr_top_callback,
		thr_task_type::thr_init_callback> my_task;

mutex global_mu_;

void top(thr_tss* tss, thr_data_message*& data){
	BOOST_CHECK(tss->hdr != 0);

	string err;
	string sql("select version();");
	db_result result;

	cout << "thr:" << this_thread::get_id() << ",exec sql use hdr:" << tss->hdr << endl;;

	BOOST_CHECK_EQUAL(tss->hdr->query_result(sql, result, err), 0);	// use connector
}

void init(thr_tss* tss){

	tss->hdr = mypool::instance()->top(); // Get connector from pool

	cout << "thr:" << this_thread::get_id() << ",get hdr from pool:" << tss->hdr << endl;;
}

void pool_run(int n){

	int i = 0;
	while(i < 10){
		string err;
		string sql("select version();");

		u16 err_code = 0;
		thr_data_message* msg = new thr_data_message();
		msg->data.copy(sql.c_str(), sql.length(), err_code);
		if(my_task::instance()->put(msg, err_code) == -1){			// Input T into queue

			bingo::stringt t;
			SHOW_CURRENT_TIME("put error, err_code:" << err_code << ",data:" << t.stream_to_string(&msg->data.this_object()->message[0], 4))

			delete msg;
		}

		this_thread::sleep(seconds(10));
		i++;
	}

//	mypool::instance()->push(hdr);									// Back connector to pool
};



BOOST_AUTO_TEST_CASE(t_pool){

	mypool::create(2u);						// Make pool with two mysql connector

	this_thread::sleep(seconds(5));

	my_task::create(
			top,							// thread_task queue top callback
			init
				);

	thread t1(pool_run, 1);
	t1.join();

	this_thread::sleep(seconds(60*2));

	my_task::release();

	mypool::release();						// Free pool

	// output:
	//	thr:7553700,make hdr:0x7552db0					// make pool
	//	thr:7f54700,make hdr:0x7f53db0
	//
	//	thr:9573700,top hdr:0x7552db0					// assign connector to thread
	//	thr:9573700,get hdr from pool:0x7552db0
	//	thr:9573700,exec sql use hdr:0x7552db0
	//	thr:8b72700,top hdr:0x7f53db0
	//	thr:8b72700,get hdr from pool:0x7f53db0
	//
	//	thr:9573700,exec sql use hdr:0x7552db0			// use connector in every thread
	//	thr:8b72700,exec sql use hdr:0x7f53db0
	//	thr:9573700,exec sql use hdr:0x7552db0
	//	thr:8b72700,exec sql use hdr:0x7f53db0
	//	thr:9573700,exec sql use hdr:0x7552db0
	//	thr:8b72700,exec sql use hdr:0x7f53db0
	//	thr:9573700,exec sql use hdr:0x7552db0
	//	thr:8b72700,exec sql use hdr:0x7f53db0
	//	thr:9573700,exec sql use hdr:0x7552db0
	//
	//	mysql send heartjump success, time:2016-03-11T16:54:12	// connector heart-jump
	//	mysql send heartjump success, time:2016-03-11T16:54:12

	//	thr:7553700,io_service thread exit!				// free pool
	//	thr:7f54700,io_service thread exit!
}

BOOST_AUTO_TEST_SUITE_END()


