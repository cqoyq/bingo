/*
 * db_column.h
 *
 *  Created on: 2016-3-10
 *      Author: root
 */

#ifndef PERSISTENT_DB_COLUMN_H_
#define PERSISTENT_DB_COLUMN_H_

#include "../string.h"
#include "db_field.h"

#include <boost/ptr_container/ptr_vector.hpp>
using namespace boost;

namespace bingo { namespace persistent {

struct db_column{
	string name;
	db_field_type type;
	size_t length;
};

} }


#endif /* PERSISTENT_DB_COLUMN_H_ */
