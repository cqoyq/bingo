/*
 * handler.h
 *
 *  Created on: 2016-2-17
 *      Author: root
 */

#ifndef PERSISTENT_MYSQL_HANDLER_H_
#define PERSISTENT_MYSQL_HANDLER_H_

#include <vector>
#include <queue>
#include <string>
using namespace std;

#include "../../xthread/spinlock.h"
using bingo::xthread::spinlock;

#include "../db_connector.h"
#include "../db_field.h"
#include "../db_result.h"
#include "../Ihandler.h"
#include "connector.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
using namespace boost;
using namespace boost::asio;

#include <boost/assert.hpp>

namespace bingo { namespace persistent { namespace mysql  {

template<typename PARSER>
class handler : public Ihandler {
public:
	class heartjump {
	public:
		heartjump(io_service& io, handler* hdr):
			timer_(io),
			hdr_(hdr){
		}
		virtual ~heartjump(){}

		// Start to timer.
		void schedule_timer(){
			seconds s(MAX_TIME_INTERVAL_SECONDS);
			time_duration td = s;
			timer_.expires_from_now(td);
			timer_.async_wait(bind(&heartjump::time_out_handler, this, placeholders::error));
		}

	protected:

		// Time out handle method.
		void time_out_handler(const system::error_code& ec){
			if(ec){

#ifdef BINGO_PERSISTENT_MYSQL_DEBUG
				test_output("mysql heartjump's time_out_handler error:" << ec.message());
#endif

			}else{

				// Check whether connect timeout.
				if(hdr_->check_connect_timeout()){

					string sql("select version();"), error;

					db_result result;
					int res = hdr_->query_result(sql, result, error);
#ifdef BINGO_PERSISTENT_MYSQL_DEBUG
					if(res == -1){
						test_output("mysql send heartjump error:" << error);
					}else{
						test_output_with_time("mysql send heartjump success");
					}
#endif
				}

				// Reset timer.
				schedule_timer();

			}
		}

		// Call schedule_timer() to Start Timer.
		deadline_timer timer_;

		enum{
			MAX_TIME_INTERVAL_SECONDS = 60,
		};

		// Pointer to connector_pool.
		handler* hdr_;
	};
public:
	handler(){
		conn_ = 0;
		heartjump_ = 0;
	}
	virtual ~handler(){

		if(conn_ != 0) 		delete conn_;
		if(heartjump_ != 0) delete heartjump_;

		mysql_library_end();
	}

	int create(string& error){

		conn_info_.user		(PARSER::user);
		conn_info_.pwd		(PARSER::pwd);
		conn_info_.ip		(PARSER::ip);
		conn_info_.dbname	(PARSER::dbname);
		conn_info_.port		(PARSER::port);

		conn_ = new connector(&conn_info_);

		// Connect to database.
		if(conn_->connect(error) == -1){

			delete conn_;
			conn_ = 0;

			return -1;
		}

		return 0;
	}

	// Start heart-jump io_service.
	void start_heartjump(){

		// Make heart jump.
		heartjump_ = new heartjump(ios_, this);
		heartjump_->schedule_timer();

		ios_.run();
	}

	// Call this to stop heart-jump io_service.
	void stop_heartjump(){

		spinlock::scoped_lock lock(slock_);

		ios_.stop();
	}

	// Check whether connect timeout.
	bool check_connect_timeout(){

		spinlock::scoped_lock lock(slock_);

		return conn_->check_connect_timeout();
	}




	/*
	 * Query single result, the sql is only single sql.
	 */
	int query_result(string& sql, db_result& result, string& err_what){

		spinlock::scoped_lock lock(slock_);

		BOOST_ASSERT(conn_ != 0);
		MYSQL* mysql_conn = conn_->get_connect();

		int res = 0;
		// Execute sql statement.
		mysql_query(mysql_conn, sql.c_str());
		const char* er = mysql_error(mysql_conn);
		if((*er) != 0x00){

			err_what.clear();
			err_what.append("mysql query fail, error:");
			err_what.append(er);

			reconnect(er, conn_);

			res = -1;
		}else{

			// Starting to handle return of result.
			MYSQL_RES *res;
			MYSQL_ROW row;

			// Get MYSQL_RES.
			res = mysql_store_result(mysql_conn);

			// Get row number and column number in MYSQL_RES.
			u64 row_num 	= mysql_num_rows(res);
			u64 column_num = mysql_num_fields(res);

			// Get column data in MYSQL_RES.
			MYSQL_FIELD *columns;
			columns = mysql_fetch_fields(res);
			make_columns(columns, column_num, result);

			// Get row data in MYSQL_RES.
			while((row = mysql_fetch_row(res))!= NULL) {

				u64 *FieldLength = mysql_fetch_lengths(res);

				make_rows(row, column_num, row_num, FieldLength, result);
			}

			mysql_free_result(res);

			conn_->update_to_now();
		}

		return res;
	}

	/*
	 * Query multi-result.
	 */
	int query_result(vector<string>& sqls, db_result_set& results, string& err_what){

		spinlock::scoped_lock lock(slock_);

		BOOST_ASSERT(conn_ != 0);
		MYSQL* mysql_conn = conn_->get_connect();

		int res = 0;
		for (size_t n = 0;  n< sqls.size(); n++){

			if(strcmp(sqls[n].c_str(),"") == 0) continue;

			// Execute sql statement.
			mysql_query(mysql_conn, sqls[n].c_str());
			const char* er = mysql_error(mysql_conn);

			if((*er) != 0x00){
				err_what.clear();
				err_what.append("mysql query fail, error:");
				err_what.append(er);

				reconnect(er, conn_);

				res = -1;
				break;
			}


			// Starting to handle return of result.
			MYSQL_RES *res;
			MYSQL_ROW row;

			db_result* result = new db_result();

			// Add result to vector.
			results.add_result(result);


			// Get MYSQL_RES.
			res = mysql_store_result(mysql_conn);

			if(res){

				// Get row number and column number in MYSQL_RES.
				u64 row_num 	= mysql_num_rows(res);
				u64 column_num = mysql_num_fields(res);

				// Get column data in MYSQL_RES.
				MYSQL_FIELD *columns;
				columns = mysql_fetch_fields(res);
				make_columns(columns, column_num, *result);

				// Get row data in MYSQL_RES.
				while((row = mysql_fetch_row(res))!= NULL) {

					u64 *FieldLength = mysql_fetch_lengths(res);

					make_rows(row, column_num, row_num, FieldLength, *result);
				}

			}

			mysql_free_result(res);

			conn_->update_to_now();

		}

		return res;
	}

	/*
	 * Execute sql with no result,
	 */
	int query_result(string& sql, string& err_what){

		spinlock::scoped_lock lock(slock_);

		BOOST_ASSERT(conn_ != 0);
		MYSQL* mysql_conn = conn_->get_connect();

		int res = 0;
		// Execute sql statement.
		mysql_query(mysql_conn, sql.c_str());
		const char* er = mysql_error(mysql_conn);
		if((*er) != 0x00){

			err_what.append(er);

			reconnect(er, conn_);

			res = -1;

		}else{

			conn_->update_to_now();
		}

		return res;
	}

	/*
	 * Execute multi-sql with no result,
	 */
	int query_result(vector<string>& sqls, string& err_what){

		spinlock::scoped_lock lock(slock_);

		BOOST_ASSERT(conn_ != 0);
		MYSQL* mysql_conn = conn_->get_connect();

		// Start trasaction.
		mysql_query(mysql_conn, "start transaction;");

		int res = 0;
		for (size_t i = 0; i < sqls.size(); ++i) {

			// Execute sql statement.
			mysql_query(mysql_conn, sqls[i].c_str());
			const char* er = mysql_error(mysql_conn);
			if((*er) != 0x00){

				err_what.append(er);

				// Roll back transaction.
				mysql_query(mysql_conn, "rollback;");

				reconnect(er, conn_);

				res = -1;

				break;
			}
		}

		if(res == 0){
			// Commit transaction.
			mysql_query(mysql_conn, "commit;");

			conn_->update_to_now();
		}


		return res;
	}

private:

	// Connect again.
	void reconnect(const char* err, connector*& conn){
		if(strcmp(err, "MySQL server has gone away") == 0){

			// Close old conn.
			mysql_close(conn->get_connect());

			string error;
			conn->connect(error);
		}
	}

	void make_columns(MYSQL_FIELD *columns, size_t& column_number, db_result& result){
		for(u64 i = 0; i < column_number; i++)
		{
			db_column* col = new db_column();

			col->name = columns[i].name;
			col->length = columns[i].length;

			switch(columns[i].type){
			// tinyint\short\int\long
			case MYSQL_TYPE_TINY:
				if(columns[i].flags & UNSIGNED_FLAG)
					col->type = DB_FIELD_TYPE_UTINY;
				else
					col->type = DB_FIELD_TYPE_TINY;
				break;
			case MYSQL_TYPE_SHORT:
				if(columns[i].flags & UNSIGNED_FLAG)
					col->type = DB_FIELD_TYPE_USHORT;
				else
					col->type = DB_FIELD_TYPE_SHORT;
				break;
			case MYSQL_TYPE_LONG:
				if(columns[i].flags & UNSIGNED_FLAG)
					col->type = DB_FIELD_TYPE_UINT;
				else
					col->type = DB_FIELD_TYPE_INT;
				break;
			case MYSQL_TYPE_LONGLONG:
				if(columns[i].flags & UNSIGNED_FLAG)
					col->type = DB_FIELD_TYPE_ULONG;
				else
					col->type = DB_FIELD_TYPE_LONG;
				break;

			// decimal\float\double
			case MYSQL_TYPE_DECIMAL:
				col->type = DB_FIELD_TYPE_DECIMAL;
				break;
			case MYSQL_TYPE_FLOAT:
				col->type = DB_FIELD_TYPE_FLOAT;
				break;
			case MYSQL_TYPE_DOUBLE:
				col->type = DB_FIELD_TYPE_DOUBLE;
				break;

			// timestamp\date\time\datetime
			case MYSQL_TYPE_TIMESTAMP:
				col->type = DB_FIELD_TYPE_TIMESTAMP;
				break;
			case MYSQL_TYPE_DATE:
				col->type = DB_FIELD_TYPE_DATE;
				break;
			case MYSQL_TYPE_TIME:
				col->type = DB_FIELD_TYPE_TIME;
				break;
			case MYSQL_TYPE_DATETIME:
				col->type = DB_FIELD_TYPE_DATETIME;
				break;

			// blob\var_string\string
			case MYSQL_TYPE_BLOB:
				col->type = DB_FIELD_TYPE_BLOB;
				break;
			case MYSQL_TYPE_VAR_STRING:
				col->type = DB_FIELD_TYPE_VAR_STRING;
				break;
			case MYSQL_TYPE_STRING:
				col->type = DB_FIELD_TYPE_STRING;
				break;

			// bit
			case MYSQL_TYPE_BIT:
				col->type = DB_FIELD_TYPE_BIT;
				break;

			default:
				col->type = DB_FIELD_TYPE_UNKNOWN;
				break;
			}

			result.add_column(col);
		}
	}

	void make_rows(MYSQL_ROW& row,
			size_t& column_number, size_t& row_number, size_t*& field_lengths,
			db_result& result){
		if(row_number == 0) return;

		db_row* row_data = new db_row();

		for (u64 i = 0;  i < column_number; i++) {

			// Get field length.
			u64 db_size  = (u64)field_lengths[i];
			char* db_value = row[i];

			char value[db_size + 1];
			memset(value, 0x00, db_size + 1);
			memcpy(value, db_value, db_size);
			string s_value = value;

			string name = result.columns()[i].name;
			db_field* field_data = new db_field(name, s_value);

			row_data->add_field(field_data);
		}

		result.add_row(row_data);
	}

private:
	heartjump* heartjump_;
	io_service ios_;


	connector* conn_;
	db_connector conn_info_;


	spinlock slock_;
};

} } }

#endif /* PERSISTENT_MYSQL_HANDLER_H_ */
