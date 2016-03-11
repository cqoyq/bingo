/*
 * db_connector.h
 *
 *  Created on: 2016-2-16
 *      Author: root
 */

#ifndef PERSISTENT_DB_CONNECTOR_H_
#define PERSISTENT_DB_CONNECTOR_H_

#include <string>
using namespace std;

#include "../pb.h"

namespace bingo { namespace persistent {

class db_connector {
public:
	db_connector();
	db_connector(const char* ip, const char* user, const char* pwd, const char* dbname, u32 port);
	virtual ~db_connector();

	const char* ip(){
		return ip_.c_str();
	}

	void ip(const char* ip){
		ip_.append(ip);
	}

	const char* user(){
		return user_.c_str();
	}

	void user(const char* user){
		user_.append(user);
	}

	const char* pwd(){
		return pwd_.c_str();
	}

	void pwd(const char* pwd){
		pwd_.append(pwd);
	}

	const char* dbname(){
		return dbname_.c_str();
	}

	void dbname(const char* dbname){
		dbname_.append(dbname);
	}

	u32 port(){
		return port_;
	}

	void port(u32 port){
		port_ = port;
	}

protected:
	string 	ip_;
	string 	user_;
	string 	pwd_;
	string 	dbname_;
	u32 	port_;
};

} }

#endif /* PERSISTENT_DB_CONNECTOR_H_ */
