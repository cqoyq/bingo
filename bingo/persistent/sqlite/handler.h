/*
 * handler.h
 *
 *  Created on: 2016-3-10
 *      Author: root
 */

#ifndef PERSISTENT_SQLITE_HANDLER_H_
#define PERSISTENT_SQLITE_HANDLER_H_

#include "../db_connector.h"
#include "../db_field.h"
#include "../db_result.h"
#include "../Ihandler.h"

#include "../../xthread/spinlock.h"
using bingo::xthread::spinlock;

#include <sqlite3.h>

namespace bingo { namespace persistent { namespace sqlite  {

class handler : public Ihandler {
public:
	handler(){
		db_ = 0;
	};
	virtual ~handler(){
		if(db_) sqlite3_close(db_);
	};

	// Open sqlite connect, success return 0.
	int create(const char *filename, string& err_what){

		if(sqlite3_open_v2(filename,
				&db_,
				SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX,
				0) == SQLITE_OK){

			return 0;
		}else{

#ifdef BINGO_PERSISTENT_SQLITE_DEBUG
			int err_code = sqlite3_errcode(db_);
			test_output("call create(), err_code:" << err_code)
#endif
			err_what = sqlite3_errmsg(db_);

			sqlite3_close(db_);
			db_ = 0;

			return -1;
		}
	}

public:
	/*
	 * Query single result, the sql is only single sql.
	 */
	int query_result(string& sql, db_result& result, string& err_what){
		spinlock::scoped_lock lock(slock_);

		BOOST_ASSERT(db_ != 0);

		const char* tail = 0;
		sqlite3_stmt* stmt = 0;
		if(sqlite3_prepare_v2(db_, sql.c_str(), sql.length(), &stmt, &tail) == SQLITE_OK){

			// Exec sql.
			int res = sqlite3_step(stmt);
			if(res == SQLITE_ROW){

				make_row(stmt, result);

				close_stmt(stmt);
				return 0;

			}else if(res == SQLITE_DONE){
				// No reset.

				close_stmt(stmt);
				return 0;

			}else{
				return get_err_and_close_stmt(stmt, err_what);
			}

		}else{

			return get_err_and_close_stmt(stmt, err_what);
		}
	}

	/*
	 * Query multi-result.
	 */
	int query_result(vector<string>& sqls, db_result_set& results, string& err_what){
		spinlock::scoped_lock lock(slock_);

		BOOST_ASSERT(db_ != 0);

		const char* tail = 0;
		sqlite3_stmt* stmt = 0;

		int r = 0;
		for (size_t n = 0;  n< sqls.size(); n++){

			if(strcmp(sqls[n].c_str(),"") == 0) continue;

			if(sqlite3_prepare_v2(db_, sqls[n].c_str(), sqls[n].length(), &stmt, &tail) == SQLITE_OK){

				db_result* result = new db_result();
				results.add_result(result);

				// Exec sql.
				int res = sqlite3_step(stmt);
				if(res == SQLITE_ROW){

					make_row(stmt, *result);
					close_stmt(stmt);

				}else if(res == SQLITE_DONE){
					// No reset.
					close_stmt(stmt);

				}else{

					get_err(err_what);
					r = -1;

					close_stmt(stmt);

					break;
				}

			}else{

				get_err(err_what);
				r = -1;

				close_stmt(stmt);

				break;
			}
		}

		return r;

	}

	/*
	 * Execute sql with no result,
	 */
	int query_result(string& sql, string& err_what){
		spinlock::scoped_lock lock(slock_);

		BOOST_ASSERT(db_ != 0);

		const char* tail = 0;
		sqlite3_stmt* stmt = 0;

		if(sqlite3_prepare_v2(db_, sql.c_str(), sql.length(), &stmt, &tail) == SQLITE_OK){

			// Exec sql.
			int res = sqlite3_step(stmt);
			if(res == SQLITE_ROW){

				close_stmt(stmt);
				return 0;

			}else if(res == SQLITE_DONE){
				// No reset.

				close_stmt(stmt);
				return 0;

			}else{
				return get_err_and_close_stmt(stmt, err_what);
			}

		}else{

			return get_err_and_close_stmt(stmt, err_what);
		}
	}

	/*
	 * Execute multi-sql with no result,
	 */
	int query_result(vector<string>& sqls, string& err_what){
		spinlock::scoped_lock lock(slock_);

		BOOST_ASSERT(db_ != 0);


		// Start transaction.
		if(begin_tansaction(err_what) == -1) return -1;

		int r = 0;
		const char* tail = 0;
		sqlite3_stmt* stmt = 0;

		for (size_t n = 0;  n< sqls.size(); n++){

			if(strcmp(sqls[n].c_str(),"") == 0) continue;

			if(sqlite3_prepare_v2(db_, sqls[n].c_str(), sqls[n].length(), &stmt, &tail) == SQLITE_OK){

				// Exec sql.
				int res = sqlite3_step(stmt);
				if(res == SQLITE_ROW){

					close_stmt(stmt);

				}else if(res == SQLITE_DONE){
					// No reset.
					close_stmt(stmt);

				}else{

					get_err(err_what);
					r = -1;

					close_stmt(stmt);

					break;
				}

			}else{

				get_err(err_what);
				r = -1;

				close_stmt(stmt);

				break;
			}
		}

		if(r == 0)
			commit_tansaction(err_what);
		else
			rollback_tansaction(err_what);

		return r;
	}

private:

	int begin_tansaction(string& err_what){
		string start_transaction = "begin transaction;";

		const char* tail = 0;
		sqlite3_stmt* stmt = 0;
		if(sqlite3_prepare_v2(db_, start_transaction.c_str(), start_transaction.length(), &stmt, &tail) == SQLITE_OK){

			// Exec sql.
			if(sqlite3_step(stmt) == SQLITE_DONE){
				close_stmt(stmt);
				return 0;
			}else{

				return get_err_and_close_stmt(stmt, err_what);
			}

		}else{
			return get_err_and_close_stmt(stmt, err_what);
		}
	}

	int commit_tansaction(string& err_what){
		string start_transaction = "commit transaction;";

		const char* tail = 0;
		sqlite3_stmt* stmt = 0;
		if(sqlite3_prepare_v2(db_, start_transaction.c_str(), start_transaction.length(), &stmt, &tail) == SQLITE_OK){

			// Exec sql.
			if(sqlite3_step(stmt) == SQLITE_DONE){
				close_stmt(stmt);
				return 0;
			}else{

				return get_err_and_close_stmt(stmt, err_what);
			}

		}else{
			return get_err_and_close_stmt(stmt, err_what);
		}
	}

	int rollback_tansaction(string& err_what){
		string start_transaction = "rollback transaction;";

		const char* tail = 0;
		sqlite3_stmt* stmt = 0;
		if(sqlite3_prepare_v2(db_, start_transaction.c_str(), start_transaction.length(), &stmt, &tail) == SQLITE_OK){

			// Exec sql.
			if(sqlite3_step(stmt) == SQLITE_DONE){
				close_stmt(stmt);
				return 0;
			}else{

				return get_err_and_close_stmt(stmt, err_what);
			}

		}else{
			return get_err_and_close_stmt(stmt, err_what);
		}
	}

	void get_err(string& err_what){

#ifdef BINGO_PERSISTENT_SQLITE_DEBUG
		int err_code = sqlite3_errcode(db_);
		test_output("call get_err_and_close(), err_code:" << err_code)
#endif

		err_what = sqlite3_errmsg(db_);
	}

	int get_err_and_close_stmt(sqlite3_stmt*& stmt, string& err_what){

#ifdef BINGO_PERSISTENT_SQLITE_DEBUG
		int err_code = sqlite3_errcode(db_);
		test_output("call get_err_and_close(), err_code:" << err_code)
#endif
		err_what = sqlite3_errmsg(db_);

		sqlite3_finalize(stmt);

		return -1;
	}

	void close_stmt(sqlite3_stmt*& stmt){

		sqlite3_finalize(stmt);
	}

	void make_row(sqlite3_stmt*& stmt, db_result& result){
		int res = 0;
		do{
			db_row* row_data = new db_row();

			int column_num = sqlite3_column_count(stmt);
			for (int i = 0; i < column_num; ++i) {

				string field_name = sqlite3_column_name(stmt, i);
				string field_value;

				switch(sqlite3_column_type(stmt, i)){
				case SQLITE_INTEGER:
					field_value = lexical_cast<string>(sqlite3_column_int(stmt, i));
					break;
				case SQLITE_FLOAT:
					field_value = lexical_cast<string>(sqlite3_column_double(stmt, i));
					break;
//					case SQLITE_BLOB:
//						field_value = lexical_cast<string>(sqlite3_column_blob(stmt, i));
//						break;
				case SQLITE3_TEXT:
					field_value = lexical_cast<string>(sqlite3_column_text(stmt, i));
					break;
				default:
					field_value = lexical_cast<string>(sqlite3_column_text(stmt, i));
				}

				db_field* field_data = new db_field(field_name, field_value);
				row_data->add_field(field_data);

			}

			result.add_row(row_data);

			res = sqlite3_step(stmt);

		}while(res == SQLITE_ROW);
	}

private:
	sqlite3 *db_;

	spinlock slock_;
};

} } }


#endif /* PERSISTENT_SQLITE_HANDLER_H_ */
