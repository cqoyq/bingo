/*
 * connector.h
 *
 *  Created on: 2016-2-17
 *      Author: root
 */

#ifndef PERSISTENT_MYSQL_CONNECTOR_H_
#define PERSISTENT_MYSQL_CONNECTOR_H_

#include "../db_connector.h"

#include "../../xthread/spinlock.h"
using bingo::xthread::spinlock;

#include <mysql.h>

namespace bingo { namespace persistent { namespace mysql  {

class connector {
public:
	connector(db_connector* conn_info);
	virtual ~connector();

	// Connect success return 0. otherwise -1, to check error.
	int connect(string& error);

	// Obtain mysql*.
	MYSQL*& get_connect();

	// Update timestamp_ to now.
	void update_to_now();

	// Check whether connect timeout.
	bool check_connect_timeout();

protected:
	db_connector* conn_info_;
	MYSQL* mysql_conn_;

	ptime timestamp_;
	enum{
		MAX_CONNECT_WAIT_IDLE_SECONDS = 60,
	};

};

} } }

#endif /* PERSISTENT_MYSQL_CONNECTOR_H_ */
