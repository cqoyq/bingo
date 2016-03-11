/*
 * tcp_client.h
 *
 *  Created on: 2016-1-22
 *      Author: root
 */

#ifndef TCP_CLIENT_H_
#define TCP_CLIENT_H_

#include <iostream>
using namespace std;

#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
using namespace boost::asio;

#include "../../tss.h"

namespace bingo { namespace internet { namespace tcp {


template<typename CONNECTION,
		 typename PARSER,
		 typename TCP_MESSAGE_PACKAGE,
		 typename TSS = thread_tss_data>
class tcp_client{
private:

	BOOST_STATIC_ASSERT(static_check_class_inherit(TSS, thread_tss_data));
	BOOST_STATIC_ASSERT(static_check_u32(PARSER::retry_delay_seconds_when_connec));
	BOOST_STATIC_ASSERT(static_check_u32(PARSER::max_retry_delay_seconds_when_connec));
public:
	typedef boost::shared_ptr<CONNECTION> pointer;
	typedef mem_guard<TCP_MESSAGE_PACKAGE> package;

	typedef function<int(pointer /*ptr*/, u16& /*err_code*/)> 				connet_success_callback;
	typedef function<void(pointer /*ptr*/, int& /*retry_delay_seconds*/)> 	connet_error_callback;

	// Connection callback
	typedef function<void(pointer /*p*/, u16& /*err_code*/)> 	catch_error_callback;
	typedef function<void(pointer /*p*/, int& /*ec_value*/)> 	close_complete_callback;

	typedef function<int(pointer /*p*/, char*& /*rev_data*/, size_t& /*rev_data_size*/, size_t& /*remain_size*/, u16& /*err_code*/)> 	read_pk_header_complete_callback;
	typedef function<int(pointer /*p*/, char*& /*rev_data*/, size_t& /*rev_data_size*/, u16& /*err_code*/)> 							read_pk_full_complete_callback;
	typedef function<void(pointer /*p*/, char*& /*snd_p*/, size_t& /*snd_size*/, const boost::system::error_code& /*ec*/)> 				write_pk_full_complete_callback;

	typedef function<int(pointer /*p*/, char*& /*snd_p*/, size_t& /*snd_size*/, u16& /*err_code*/)> 				active_send_in_ioservice_callback;

	typedef function<int(package*& /*pk*/, u16& /*err_code*/)> make_send_first_package_callback;

	tcp_client(boost::asio::io_service& io_service, string& ipv4, u16& port,
			connet_success_callback				f1 = 0,
			connet_error_callback 				f2 = 0,

			catch_error_callback 				f11 = 0,
			read_pk_header_complete_callback 	f12 = 0,
			read_pk_full_complete_callback 		f13 = 0,
			write_pk_full_complete_callback 	f14 = 0,
			active_send_in_ioservice_callback 	f15 = 0,
			close_complete_callback				f16 = 0,
			make_send_first_package_callback	f17 = 0
			):  ios_(io_service), ipv4_(ipv4), port_(port),
				f1_(f1), f2_(f2),
				f11_(f11), f12_(f12), f13_(f13),
				f14_(f14), f15_(f15), f16_(f16), f17_(f17),
				timer_(io_service),
				retry_delay_(PARSER::retry_delay_seconds_when_connec){

		this_thread_tss.reset(new TSS());	// initialize the thread's storage

		start_connect();
	}

	void start_connect(){

		// Make new tcp_connection object.
		pointer new_connection =
				CONNECTION::create(ios_,
						boost::bind(&tcp_client::reconnect, this),
						f11_, f12_, f13_,
						f14_, f15_, f16_,
						f17_);

		// Start to connect.
		new_connection->socket().async_connect(
				ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(ipv4_), port_),
				boost::bind(&tcp_client::conn_handler, this, new_connection,
				boost::asio::placeholders::error));
	}

private:

	// Call the function when connect in handler again.
	void reconnect(){
		retry_delay_ = PARSER::retry_delay_seconds_when_connec;
		start_connect();
	}

	void conn_handler(pointer new_connection,
			const boost::system::error_code& ec){
		if (!ec){

			// Call accept_handle_success_func()
			if(f1_){

				u16 err_code = 0;
				if(f1_(new_connection, err_code) == 0)
					// Start to aync-read.
					new_connection->start();
				else{

					new_connection->catch_error(err_code);

					// Active close socket.
					new_connection->close_socket();
				}

			}else{

				// Start to aync-read.
				new_connection->start();
			}

		}else{

			if(PARSER::retry_delay_seconds_when_connec == 0)
				return;

			// Begin to reconnect.
			retry_delay_ *= 2;
			if (retry_delay_ > PARSER::max_retry_delay_seconds_when_connec)
				retry_delay_ = PARSER::max_retry_delay_seconds_when_connec;

			if(f2_) f2_(new_connection, retry_delay_);

			// Start to reconnet.
			schedule_timer(retry_delay_);

			return;
		}
	}

	void schedule_timer(int& expire_second){

		boost::posix_time::seconds s(expire_second);
		boost::posix_time::time_duration td = s;
		timer_.expires_from_now(td);
		timer_.async_wait(bind(&tcp_client::time_out_handler, this, boost::asio::placeholders::error));
	}

	void time_out_handler(const system::error_code& ec){

		if(!ec)
			start_connect();
	}

protected:
	io_service &ios_;
	pointer connection_;

	string ipv4_;
	u16    port_;

	int retry_delay_;				// reconnect delay seconds.
	deadline_timer timer_;			// reconnect timer.

	connet_success_callback				f1_;
	connet_error_callback 				f2_;

	catch_error_callback 				f11_;
	read_pk_header_complete_callback 	f12_;
	read_pk_full_complete_callback 		f13_;
	write_pk_full_complete_callback 	f14_;
	active_send_in_ioservice_callback 	f15_;
	close_complete_callback				f16_;
	make_send_first_package_callback	f17_;
};

} } }

#endif /* TCP_CLIENT_H_ */
