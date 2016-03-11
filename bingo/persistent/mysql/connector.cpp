/*
 * connector.cpp
 *
 *  Created on: 2016-2-17
 *      Author: root
 */

#include "connector.h"

#include <mysql.h>

using bingo::persistent::mysql::connector;

connector::connector(db_connector* conn_info){
	conn_info_ = conn_info;
	mysql_conn_ = 0;

	timestamp_ = boost::posix_time::microsec_clock::local_time();
}

connector::~connector(){

	if(mysql_conn_!=0){

		mysql_close(mysql_conn_);

		mysql_thread_end();
	}
}

int connector::connect(string& error){

	// Init mysql connector.
	mysql_conn_ = mysql_init(NULL);

	my_bool reconnect = 0;
	mysql_options(mysql_conn_, MYSQL_SECURE_AUTH, &reconnect);

	// Connect mysql's server.
	if (!mysql_real_connect(mysql_conn_,
			conn_info_->ip(), conn_info_->user(), conn_info_->pwd(), conn_info_->dbname(), conn_info_->port(),
			NULL, 0)) {

		error.clear();
		error.append("database connect fail");

		// free Mysql.
		mysql_close(mysql_conn_);

		mysql_thread_end();

		mysql_conn_ = 0;

		return -1;
	}

	mysql_set_character_set(mysql_conn_, "utf8");

	return 0;
}

MYSQL*& connector::get_connect(){
	return mysql_conn_;
}

void connector::update_to_now(){

	timestamp_ = boost::posix_time::microsec_clock::local_time();
}

bool connector::check_connect_timeout(){

	ptime now = boost::posix_time::microsec_clock::local_time();
	ptime p1 = now - seconds(MAX_CONNECT_WAIT_IDLE_SECONDS);

	if(timestamp_ < p1){
		return true;
	}else{
		return false;
	}
}

