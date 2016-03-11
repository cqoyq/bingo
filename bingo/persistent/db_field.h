/*
 * db_field.h
 *
 *  Created on: 2016-2-16
 *      Author: root
 */

#ifndef PERSISTENT_DB_FIELD_H_
#define PERSISTENT_DB_FIELD_H_

#include "../string.h"
using namespace bingo;

namespace bingo { namespace persistent {

enum db_field_type {
	DB_FIELD_TYPE_TINY			= 0x01,		// 1
	DB_FIELD_TYPE_UTINY,					// 2
	DB_FIELD_TYPE_SHORT,					// 3
	DB_FIELD_TYPE_USHORT,					// 4
	DB_FIELD_TYPE_MEDIUMINT,				// 5
	DB_FIELD_TYPE_UMEDIUMINT,				// 6
	DB_FIELD_TYPE_INT,						// 7
	DB_FIELD_TYPE_UINT,						// 8
	DB_FIELD_TYPE_LONG,						// 9
	DB_FIELD_TYPE_ULONG,					// 10

	DB_FIELD_TYPE_DECIMAL,					// 11
	DB_FIELD_TYPE_FLOAT,					// 12
	DB_FIELD_TYPE_DOUBLE,					// 13

	DB_FIELD_TYPE_TIMESTAMP,				// 14
	DB_FIELD_TYPE_DATE,						// 15
	DB_FIELD_TYPE_TIME,						// 16
	DB_FIELD_TYPE_DATETIME,					// 17

	DB_FIELD_TYPE_BLOB,						// 18
	DB_FIELD_TYPE_VAR_STRING,				// 19
	DB_FIELD_TYPE_STRING,					// 20

	DB_FIELD_TYPE_BIT,						// 21
	DB_FIELD_TYPE_UNKNOWN,					// 22
};

class db_field {
public:
	db_field(string& name, string& value) :
		name_(name),
		value_(value){}
	virtual ~db_field(){}

	const char* name(){
		return name_.c_str();
	}

	const char* value(){
		return value_.c_str();
	}
private:
	string name_;
	string value_;
};

} }

#endif /* PERSISTENT_DB_FIELD_H_ */
