/*
 * handler.h
 *
 *  Created on: 2016-2-17
 *      Author: root
 */

#ifndef PERSISTENT_HANDLER_H_
#define PERSISTENT_HANDLER_H_

namespace bingo { namespace persistent {

class Ihandler{
public:
	virtual ~Ihandler(){};

	/*
	 * Query single result, the sql is only single sql.
	 */
	virtual int query_result(string& sql, db_result& result, string& err_what) = 0;

	/*
	 * Query multi-result.
	 */
	virtual int query_result(vector<string>& sqls, db_result_set& results, string& err_what) = 0;

	/*
	 * Execute sql with no result,
	 */
	virtual int query_result(string& sql, string& err_what) = 0;

	/*
	 * Execute multi-sql with no result,
	 */
	virtual int query_result(vector<string>& sqls, string& err_what) = 0;
};

} }


#endif /* HANDLER_H_ */
