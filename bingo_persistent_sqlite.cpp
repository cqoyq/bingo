/*
 * bingo_persistent_sqlite.cpp
 *
 *  Created on: 2016-3-11
 *      Author: root
 */

#include <boost/test/unit_test.hpp>

#include <iostream>
using namespace std;

#include "cost_time.h"

#define BINGO_PERSISTENT_SQLITE_DEBUG
#include "bingo/persistent/sqlite/all.h"
using namespace bingo;
using namespace bingo::persistent;


BOOST_AUTO_TEST_SUITE(bingo_persistent_sqlite)

/*
 * 测试sql操作
 */
BOOST_AUTO_TEST_CASE(t_sql){

	string err_what;
	sqlite::handler hdr;

	// Check open fail.
	BOOST_CHECK(hdr.create("./sqlite_test.db111", err_what) == -1);
	cout << "open file:" << "./sqlite_test.db111" << " fail, err:" << err_what << endl;
	// output:
	//	open file:./sqlite_test.db111 fail, err:unable to open database file


	// Open sqlite succes.
	BOOST_CHECK(hdr.create("./sqlite_test.db", err_what) == 0);

	string sql = "select * from t_peole34;";

	// Check sql err
	{
		db_result res;
		BOOST_CHECK(hdr.query_result(sql, res, err_what) == -1);
		cout << "select sql fail, err:" << err_what << endl;
		// output:
		//	exe sql fail, err:no such table: t_peole34

		sql.clear();
		sql.append("select auto_1 from t_peole;");
		BOOST_CHECK(hdr.query_result(sql, res, err_what) == -1);
		cout << "select sql fail, err:" << err_what << endl;
		// output:
		//	exe sql fail, err:no such column: auto_1

		sql.clear();
		sql.append("insert into t_peole (auto_id) values (1);");
		BOOST_CHECK(hdr.query_result(sql, err_what) == -1);
		cout << "insert sql fail, err:" << err_what << endl;
		// output:
		//	insert sql fail, err:table t_peole has no column named auto_id

		// Check no element in t_peole
		{
			sql.clear();
			sql.append("select * from t_peole;");
			db_result res;
			BOOST_CHECK(hdr.query_result(sql, res, err_what) == 0);
			BOOST_CHECK(res.rows().size() == 0);
		}

		sql.clear();
		sql.append("insert into t_peole (name, remark) values ('bac','80');");
		BOOST_CHECK(hdr.query_result(sql, err_what) == 0);

		// Check element in t_peole
		{
			sql.clear();
			sql.append("select name,remark from t_peole;");
			db_result res;
			BOOST_CHECK(hdr.query_result(sql, res, err_what) == 0);
			BOOST_CHECK(res.rows().size() == 1);
			BOOST_CHECK(strcmp(res.rows()[0][0]->value(),"bac") == 0);
			BOOST_CHECK(strcmp(res.rows()[0][1]->value(),"80") == 0);

			sql.clear();
			sql.append("delete from t_peole;");
			BOOST_CHECK(hdr.query_result(sql, err_what) == 0);
		}

		vector<string> sqls;
		{
			string sql1 = "insert into t_peole (name, remark) values ('cqoyq','999');";
			string sql2 = "insert into t_peole (name, remark) values ('chan','8888');";
			string sql3 = "insert into t_address (ipaddress) values ('127.0.0.1');";
			string sql_err = "insert into t_address ('ipaddress9') values ('127.0.0.1');";

			sqls.push_back(sql1);
			sqls.push_back(sql2);
			sqls.push_back(sql3);
			sqls.push_back(sql_err);

			BOOST_CHECK(hdr.query_result(sqls, err_what) == -1);
			cout << "multi-sql insert sql fail, err:" << err_what << endl;
			// output:
			//	multi-sql insert sql fail, err:table t_address has no column named ipaddress9

			// Check no element in t_peole and t_address
			{
				string sql1 = "select * from t_peole;";
				string sql2 = "select * from t_address;";

				sqls.clear();
				sqls.push_back(sql1);
				sqls.push_back(sql2);
				db_result_set res_set;
				BOOST_CHECK(hdr.query_result(sqls, res_set, err_what) == 0);
				BOOST_CHECK(res_set[0]->rows().size() == 0);
				BOOST_CHECK(res_set[1]->rows().size() == 0);
			}
		}

		{
			string sql1 = "insert into t_peole (name, remark) values ('cqoyq','999');";
			string sql2 = "insert into t_peole (name, remark) values ('chan','8888');";
			string sql3 = "insert into t_address (ipaddress) values ('127.0.0.1');";

			sqls.clear();
			sqls.push_back(sql1);
			sqls.push_back(sql2);
			sqls.push_back(sql3);
			BOOST_CHECK(hdr.query_result(sqls, err_what) == 0);
			{
				string sql1 = "select name, remark from t_peole;";
				string sql2 = "select ipaddress from t_address;";

				sqls.clear();
				sqls.push_back(sql1);
				sqls.push_back(sql2);
				db_result_set res_set;
				BOOST_CHECK(hdr.query_result(sqls, res_set, err_what) == 0);
				BOOST_CHECK(res_set[0]->rows().size() == 2);
				BOOST_CHECK(res_set[1]->rows().size() == 1);
				BOOST_CHECK(strcmp(res_set[0]->rows()[0][0]->value(),"cqoyq") == 0);
				BOOST_CHECK(strcmp(res_set[0]->rows()[1][0]->value(),"chan") == 0);
				BOOST_CHECK(strcmp(res_set[1]->rows()[0][0]->value(),"127.0.0.1") == 0);

				cout << "res_set0 data:" << endl;
				int i = 0;
				foreach_(db_row& row, res_set[0]->rows()){
					cout << "row index:" << i << ",column index:0, name:" << row[0]->name() << ",value:" << row[0]->value() << endl;
					i++;
				}

				cout << "res_set1 data:" << endl;
				i = 0;
				foreach_(db_row& row, res_set[1]->rows()){
					cout << "row index:" << i << ",column index:0, name:" << row[0]->name() << ",value:" << row[0]->value() << endl;
					i++;
				}
				// output:
				//	res_set0 data:
				//	row index:0,column index:0, name:name,value:cqoyq
				//	row index:1,column index:0, name:name,value:chan

				//	res_set1 data:
				//	row index:0,column index:0, name:ipaddress,value:127.0.0.1
			}

			// Clear data.
			{
				string sql1 = "delete from t_peole;";
				string sql2 = "delete from t_address;";

				sqls.clear();
				sqls.push_back(sql1);
				sqls.push_back(sql2);
				BOOST_CHECK(hdr.query_result(sqls, err_what) == 0);
			}

			{
				string sql1 = "select * from t_peole;";
				string sql2 = "select * from t_address;";

				sqls.clear();
				sqls.push_back(sql1);
				sqls.push_back(sql2);
				db_result_set res_set;
				BOOST_CHECK(hdr.query_result(sqls, res_set, err_what) == 0);
				BOOST_CHECK(res_set[0]->rows().size() == 0);
				BOOST_CHECK(res_set[1]->rows().size() == 0);
			}

		}

	}

	// Select single sql.
	{
		sql.clear();
		sql.append("select * from t_peole;");
		db_result res;
		BOOST_CHECK(hdr.query_result(sql, res, err_what) == 0);
		BOOST_CHECK(res.rows().size() == 0);
	}

	// Select multi-sql.
	{
		string sql1 = "select * from t_peole;";
		string sql2 = "select * from t_address;";
		string sql_error = "select * from t_ad90;";

		vector<string> sqls;
		sqls.push_back(sql1);
		sqls.push_back(sql2);
		sqls.push_back(sql_error);

		// Check error.
		db_result_set res_set;
		BOOST_CHECK(hdr.query_result(sqls, res_set, err_what) == -1);
		cout << "select multi-sql error:" << err_what << endl;
		// output:
		//	select multi-sql error:no such table: t_ad90

		res_set.results().clear();
		sqls.clear();
		sqls.push_back(sql1);
		sqls.push_back(sql2);
		BOOST_CHECK(hdr.query_result(sqls, res_set, err_what) == 0);
		BOOST_CHECK(res_set[0]->rows().size() == 0);
		BOOST_CHECK(res_set[1]->rows().size() == 0);
	}

}

BOOST_AUTO_TEST_SUITE_END()

