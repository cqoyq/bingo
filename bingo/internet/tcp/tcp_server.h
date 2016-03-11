/*
 * tcp_server.h
 *
 *  Created on: 2016-1-19
 *      Author: root
 */

#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

#include <iostream>
using namespace std;

#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
using namespace boost;
using namespace boost::asio;

#include <boost/static_assert.hpp>

#include "../../tss.h"

namespace bingo { namespace internet { namespace tcp {

template<typename CONNECTION,
		 typename MANAGER,
		 typename PARSER,
		 typename TSS = thread_tss_data
		 >
class tcp_server{
private:
	BOOST_STATIC_ASSERT(static_check_class_inherit(TSS, thread_tss_data));
	BOOST_STATIC_ASSERT(static_check_u32(PARSER::max_wait_for_heartjump_seconds));
	BOOST_STATIC_ASSERT(static_check_u32(PARSER::max_wait_for_authentication_pass_seconds));
public:

	typedef boost::shared_ptr<CONNECTION> pointer;

	// Accept callback
	typedef function<void(const system::error_code& /*ec*/)> 		accept_error_callback;
	typedef function<int(pointer /*ptr*/, u16& /*err_code*/)> 		accept_success_callback;

	// Connection callback
	typedef function<void(pointer /*p*/, u16& /*err_code*/)> 		catch_error_callback;
	typedef function<void(pointer /*p*/, int& /*ec_value*/)> 		close_complete_callback;

	typedef function<int(pointer /*p*/, char*& /*rev_data*/, size_t& /*rev_data_size*/, size_t& /*remain_size*/, u16& /*err_code*/)> 	read_pk_header_complete_callback;
	typedef function<int(pointer /*p*/, char*& /*rev_data*/, size_t& /*rev_data_size*/, u16& /*err_code*/)> 							read_pk_full_complete_callback;
	typedef function<void(pointer /*p*/, char*& /*snd_p*/, size_t& /*snd_size*/, const boost::system::error_code& /*ec*/)> 				write_pk_full_complete_callback;

	typedef function<int(pointer /*p*/, char*& /*snd_p*/, size_t& /*snd_size*/, u16& /*err_code*/)> active_send_in_ioservice_callback;


	tcp_server(boost::asio::io_service& io_service, string& ipv4, u16& port,
			accept_success_callback				f1 = 0,
			accept_error_callback				f2 = 0,

			catch_error_callback 				f11 = 0,
			read_pk_header_complete_callback 	f12 = 0,
			read_pk_full_complete_callback 		f13 = 0,
			write_pk_full_complete_callback 	f14 = 0,
			active_send_in_ioservice_callback 	f15 = 0,
			close_complete_callback				f16 = 0
			)
		: f1_(f1),
		  f2_(f2),
		  f11_(f11), f12_(f12), f13_(f13),
		  f14_(f14), f15_(f15), f16_(f16),
		  check_heartjump_timer_(io_service),
		  check_authentication_timer_(io_service),
		  acceptor_(io_service,
				  ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(ipv4), port)
		  	  	  ){

		this_thread_tss.reset(new TSS());	// initialize the thread's storage

		start_accept();

		// Start to inspect heartjump
		check_heartjump();

		// Start to inspect authentication pass
		check_authentication();
	}


	void start_accept(){
#ifdef BINGO_TCP_SERVER_DEBUG
		std::stringstream stream;
		stream << this_thread::get_id();
		string id = stream.str();
		test_output("thr:" << id << ",call start_accept()")
#endif

		// Make new tcp_connection object.
		pointer new_connection =
				CONNECTION::create(acceptor_.get_io_service(),
					  f11_, f12_, f13_,
					  f14_, f15_, f16_);

		// Start to wait for connect.
		acceptor_.async_accept(new_connection->socket(),
				boost::bind(&tcp_server::accept_handler, this, new_connection,
				boost::asio::placeholders::error));
	}

	void accept_handler(pointer new_connection,
			const boost::system::error_code& ec){
		if (!ec)
		{

#ifdef BINGO_TCP_SERVER_DEBUG
			std::stringstream stream;
			stream << this_thread::get_id();
			string id = stream.str();
			test_output("thr:" << id << ",call accept_handler()")
#endif

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

			// Call accept_handle_error_func()
			if(f2_) f2_(ec);
		}

		// Start another one.
		start_accept();
	}

private:

	void check_authentication(){
		if(PARSER::max_wait_for_authentication_pass_seconds > 0){
			boost::posix_time::seconds s(PARSER::max_wait_for_authentication_pass_seconds);
			boost::posix_time::time_duration td = s;
			check_authentication_timer_.expires_from_now(td);
			check_authentication_timer_.async_wait(bind(&tcp_server::authentication_handler,
					this,
					boost::asio::placeholders::error));
		}
	}

	void authentication_handler(const system::error_code& ec){
		if(!ec){
#ifdef BINGO_TCP_SERVER_DEBUG
			test_output_with_time("start: check_authentication_pass")
#endif
			MANAGER::instance()->check_authentication_pass();
			check_authentication();
		}
	}

	void check_heartjump(){
		if(PARSER::max_wait_for_heartjump_seconds > 0){
			boost::posix_time::seconds s(PARSER::max_wait_for_heartjump_seconds);
			boost::posix_time::time_duration td = s;
			check_heartjump_timer_.expires_from_now(td);
			check_heartjump_timer_.async_wait(bind(&tcp_server::heartjump_handler,
					this,
					boost::asio::placeholders::error));
		}
	}

	void heartjump_handler(const system::error_code& ec){

		if(!ec){
#ifdef BINGO_TCP_SERVER_DEBUG
			test_output_with_time("start: check_heartjump")
#endif
			MANAGER::instance()->check_heartjump();
			check_heartjump();
		}
	}





protected:

	ip::tcp::acceptor acceptor_;

	accept_success_callback 			f1_;
	accept_error_callback   			f2_;

	catch_error_callback 				f11_;
	read_pk_header_complete_callback 	f12_;
	read_pk_full_complete_callback 		f13_;
	write_pk_full_complete_callback 	f14_;
	active_send_in_ioservice_callback 	f15_;
	close_complete_callback				f16_;

	deadline_timer check_heartjump_timer_;
	deadline_timer check_authentication_timer_;
};



} } }


#endif /* TCP_SERVER_H_ */
