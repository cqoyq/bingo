/*
 * db_result.h
 *
 *  Created on: 2016-2-16
 *      Author: root
 */

#ifndef PERSISTENT_DB_RESULT_H_
#define PERSISTENT_DB_RESULT_H_

#include "db_row.h"
#include "db_column.h"

#include <boost/ptr_container/ptr_vector.hpp>
using namespace boost;

namespace bingo { namespace persistent {

class db_result {
public:
	db_result(){}
	virtual ~db_result(){}

	void add_column(db_column*& column){
		columns_.push_back(column);
	}

	boost::ptr_vector<db_column>& columns(){
		return columns_;
	}

	void add_row(db_row*& row){
		rows_.push_back(row);
	}

	boost::ptr_vector<db_row>& rows(){
		return rows_;
	}

private:
	boost::ptr_vector<db_row> rows_;
	boost::ptr_vector<db_column> columns_;
};

class db_result_set {
public:
	db_result_set(){};
	virtual ~db_result_set(){}

	void add_result(db_result*& res){
		res_.push_back(res);
	}

	boost::ptr_vector<db_result>& results(){
		return res_;
	}

	db_result* operator[](const size_t i){

		if(res_.size() == 0)
			return 0;

		if(i > (res_.size() - 1))
			return 0;
		else
			return &res_[i];
	}

private:
	boost::ptr_vector<db_result> res_;
};

} }

#endif /* PERSISTENT_DB_RESULT_H_ */
