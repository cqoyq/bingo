/*
 * db_connector.cpp
 *
 *  Created on: 2016-2-16
 *      Author: root
 */

#include "db_connector.h"

using bingo::persistent::db_connector;

db_connector::db_connector() {
	// TODO Auto-generated constructor stub
	port_ = 0;
}

db_connector::db_connector(const char* ip, const char* user, const char* pwd, const char* dbname, u32 port){

	ip_.append(ip);
	user_.append(user);
	pwd_.append(pwd);
	dbname_.append(dbname);
	port_ = port;
}

db_connector::~db_connector() {
	// TODO Auto-generated destructor stub
}

