/*
 * mq_server_hgr.h
 *
 *  Created on: 2016-3-2
 *      Author: root
 */

#ifndef MQ_SERVER_HGR_H_
#define MQ_SERVER_HGR_H_

#include <string>
using namespace std;

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
using namespace boost;

#include "../xthread/spinlock.h"
using bingo::xthread::spinlock;

#include "../internet/tcp/all.h"
using namespace bingo;
using namespace bingo::internet::tcp;

#include "mq_package.h"

namespace bingo { namespace mq {

// Server thread local storage.
struct mq_server_tss_data : public thread_tss_data{
	bingo::stringt t;
};

// Server parse detail.
struct mq_server_parse{
	static const size_t header_size = MAX_SIZE_OF_MQ_PACKAGE_HEADER;			// Parse size of package's header.
	static const int max_wait_for_heartjump_seconds = 10;						// If the value is 0, then server don't check heartjump.
	static const int max_wait_for_authentication_pass_seconds = 15;				// If the value is 0, then server don't check authentication pass.
};

typedef tcp_svr_connection<mq_package,				// Message
						   mq_server_parse,			// Define static header_size
						   mq_server_tss_data		// The thread's storage
	 	 	> mq_server_connection_type;

// Server handler data.
struct mq_svr_handler_data{
	string name;										// Named tcp_connection.
	mq_server_connection_type* handler_pointer;			// Pointer to tcp_connection.

	mq_svr_handler_data(mq_server_connection_type*& p):handler_pointer(p){}
};

// Server handler manager.
class mq_svr_handler_manager : public IServerhandler{
public:
	mq_svr_handler_manager();
	virtual ~mq_svr_handler_manager();

	void insert(mq_server_connection_type* p);
	int erase(mq_server_connection_type* p, u16& err_code);
	int set_handler_name(mq_server_connection_type* p, string& name, u16& err_code);

	// Send data by handler's name.
	void send_data(string& name, const char* data, size_t data_size);

	void check_heartjump();
	void check_authentication_pass();
private:
	spinlock mu_;
	boost::ptr_vector<mq_svr_handler_data> sets_;
	typedef boost::ptr_vector<mq_svr_handler_data>::iterator set_iterator;
};

typedef bingo::singleton_v0<mq_svr_handler_manager> mq_server_mgr_type;

typedef  tcp_server<
		mq_server_connection_type,
		mq_server_mgr_type,
		mq_server_parse,
		mq_package,
		mq_server_tss_data
			> mq_server_type;

} }


#endif /* MQ_SERVER_HGR_H_ */
