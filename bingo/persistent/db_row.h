/*
 * db_row.h
 *
 *  Created on: 2016-3-10
 *      Author: root
 */

#ifndef PERSISTENT_DB_ROW_H_
#define PERSISTENT_DB_ROW_H_

#include "db_field.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
using namespace boost;

namespace bingo { namespace persistent {

class db_row {
private:
	struct find_first_same_name{
		find_first_same_name(const string& name):name_(name){}

		bool operator()(db_field& n){
			return (strcmp(n.name(), name_.c_str()) == 0)?true: false;
		}
	private:
		string name_;
	};

public:
	db_row(){};
	virtual ~db_row(){}

	void add_field(db_field*& field){
		row_.push_back(field);
	}

	boost::ptr_vector<db_field>& fields(){
		return row_;
	}

	db_field* operator[](const size_t i){
		if(row_.size() == 0)
			return 0;

		if(i > (row_.size() - 1))
			return 0;
		else
			return &row_[i];
	}

	db_field* operator[](const string name){
		set_iterator iter = boost::find_if(row_, find_first_same_name(name));

		if(iter != row_.end()){

			return &(*iter);
		}else{

			return 0;
		}
	}

private:
	boost::ptr_vector<db_field> row_;
	typedef boost::ptr_vector<db_field>::iterator set_iterator;
};

} }


#endif /* PERSISTENT_DB_ROW_H_ */
