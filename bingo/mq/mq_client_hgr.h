/*
 * mq_client_hgr.h
 *
 *  Created on: 2016-3-4
 *      Author: root
 */

#ifndef MQ_CLIENT_HGR_H_
#define MQ_CLIENT_HGR_H_

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

struct mq_client_tss_data : public thread_tss_data{
	bingo::stringt t;
};

struct mq_client_parse{
	static int retry_delay_seconds_when_connec;							// When Client connect fail, don't reconnect if the value is 0.
	static int max_retry_delay_seconds_when_connec;

	static const size_t header_size = MAX_SIZE_OF_MQ_PACKAGE_HEADER;	// Parse size of package's header.
	static int max_interval_seconds_when_send_heartjump;				// Client don't send heartjump if the value is 0.
};

typedef tcp_cet_connection<
			mq_package,						// Message
			mq_client_parse,				// Define static header_size
			mq_heartjump_package,			// Type of Heartjump package
			mq_client_tss_data				// The thread's storage
 	 	 > mq_client_connection_type;

class mq_cet_handler_manager{
public:
	mq_cet_handler_manager();
	virtual ~mq_cet_handler_manager();

	void insert(mq_client_connection_type* p);
	void erase();

	// Send data.
	int send_data(const char* data, size_t data_size, u16& err_code);

private:
	spinlock mu_;
	mq_client_connection_type* handler_pointer_;
};

typedef bingo::singleton_v0<mq_cet_handler_manager> mq_client_mgr_type;

typedef tcp_client<
			mq_client_connection_type,
			mq_client_parse,
			mq_package,
			mq_client_tss_data
		> mq_client_type;

} }

#endif /* MQ_CLIENT_HGR_H_ */
