/*
 * tcp_svr_connection.h
 *
 *  Created on: 2016-1-19
 *      Author: root
 */

#ifndef TCP_SVR_CONNECTION_H_
#define TCP_SVR_CONNECTION_H_

#include <iostream>
using namespace std;

#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
using namespace boost;
using namespace boost::asio;
#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;

#include <boost/static_assert.hpp>

#include "../../tss.h"

namespace bingo { namespace internet { namespace tcp {

template<typename TCP_MESSAGE_PACKAGE,
		 typename PARSER,
		 typename TSS = thread_tss_data
		>
class tcp_svr_connection
  : public boost::enable_shared_from_this<tcp_svr_connection<TCP_MESSAGE_PACKAGE, PARSER, TSS> >{
private:

	BOOST_STATIC_ASSERT(static_check_class_inherit(TSS, thread_tss_data));
	BOOST_STATIC_ASSERT(static_check_u32(PARSER::max_wait_for_heartjump_seconds));
	BOOST_STATIC_ASSERT(static_check_u32(PARSER::max_wait_for_authentication_pass_seconds));
	BOOST_STATIC_ASSERT(static_check_size_t(PARSER::header_size));
	BOOST_STATIC_ASSERT(static_check_greater_than_zero(PARSER::header_size));
public:

	typedef mem_guard<TCP_MESSAGE_PACKAGE> package;
	typedef boost::shared_ptr<tcp_svr_connection> pointer;

	// Connection callback
	typedef function<void(pointer /*p*/, u16& /*err_code*/)> 		catch_error_callback;
	typedef function<void(pointer /*p*/, int& /*ec_value*/)> 		close_complete_callback;

	typedef function<int(pointer /*p*/, char*& /*rev_data*/, size_t& /*rev_data_size*/, size_t& /*remain_size*/, u16& /*err_code*/)> 	read_pk_header_complete_callback;
	typedef function<int(pointer /*p*/, char*& /*rev_data*/, size_t& /*rev_data_size*/, u16& /*err_code*/)> 							read_pk_full_complete_callback;
	typedef function<void(pointer /*p*/, char*& /*snd_p*/, size_t& /*snd_size*/, const boost::system::error_code& /*ec*/)> 				write_pk_full_complete_callback;

	typedef function<int(pointer /*p*/, char*& /*snd_p*/, size_t& /*snd_size*/, u16& /*err_code*/)> active_send_in_ioservice_callback;

	static pointer create(boost::asio::io_service& io_service,
			catch_error_callback& 				f1,
			read_pk_header_complete_callback& 	f2,
			read_pk_full_complete_callback& 	f3,
			write_pk_full_complete_callback& 	f4,
			active_send_in_ioservice_callback& 	f5,
			close_complete_callback&			f6
			){

		return pointer(new tcp_svr_connection(io_service, f1, f2, f3, f4, f5, f6));
	}

	~tcp_svr_connection(){
#ifdef BINGO_TCP_SERVER_DEBUG
		test_output("handler:" << this << ",destory!");
#endif
	}

	TSS* this_tss(){
		return (TSS*)this_thread_tss.get();
	}

	ip::tcp::socket& socket(){
		return socket_;
	}

	void start(){

		// Set authentication start to time.
		set_authentication_pass_datetime();

		// Start async read data, read finish to call read_handler().
		boost::asio::async_read(socket_,
				boost::asio::buffer(rev_mgr_.current(), PARSER::header_size),
				boost::bind(&tcp_svr_connection::read_handler,
							this->shared_from_this(),
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred,
							PARSER::header_size));


	}

	void close_socket(){
		// This function is used to close the socket. Any asynchronous send, receive
		// or connect operations will be cancelled immediately, and will complete
		// with the boost::asio::error::operation_aborted error.
		socket_.close();
	}

	void catch_error(u16 err_code){
		if(f1_) f1_(this->shared_from_this(), err_code);
	}


	// Call this from tcp_handler_manager.
	bool check_heartjump_timeout(){
		ptime now = boost::posix_time::microsec_clock::local_time();
		ptime p1 = now - seconds(PARSER::max_wait_for_heartjump_seconds);

#ifdef BINGO_TCP_SERVER_DEBUG
		test_output("check heartjump time, p1_:" << to_iso_extended_string(p1_) <<
				",p1:" << to_iso_extended_string(p1) <<
				",p1_< p1:" << (p1_ < p1) <<
				",hdr:" << this);
#endif

		if(p1_ < p1){
			return true;
		}else{
			return false;
		}
	}

	// Call this to update p1_ when detect the heartjump package.
	void set_heartjump_datetime(){
		p1_ = boost::posix_time::microsec_clock::local_time();

#ifdef BINGO_TCP_SERVER_DEBUG
		test_output("set heartjump time:" << to_iso_extended_string(p1_) << ",hdr:" << this);
#endif
	}



	// Call this from tcp_handler_manager.
	bool check_authentication_pass(){

		if(!is_authentication_pass_){
			// Check whether authentication pass is timeout.
			ptime now = boost::posix_time::microsec_clock::local_time();
			ptime p2 = now - seconds(PARSER::max_wait_for_authentication_pass_seconds);

			if(p2_ < p2)
				return false;
			else
				return true;

		}else{
			return true;
		}
	}

	// Call this to update is_authentication_pass_ is true, and then update heartjump datetime.
	// when detect authentication is pass.
	void set_authentication_pass(){
		is_authentication_pass_ = true;

		// Set heart-jump start to time.
		set_heartjump_datetime();
	}



	void send_data_in_thread(char*& sdata, size_t& sdata_size){

		u16 err_code = 0;
		package* new_data = 0;

		// Malloc send buffer.
		if(malloc_snd_buffer(sdata, sdata_size, new_data, err_code) == 0){

			ios_.post(bind(&tcp_svr_connection::active_send, this->shared_from_this(), new_data));
		}else{

			catch_error(err_code);

			send_close_in_thread(err_code);

		}


	}

	void send_close_in_thread(u16 err_code){

		ios_.post(bind(&tcp_svr_connection::active_close, this->shared_from_this(), err_code));
	}

	void active_send(package*& pk){

		if(is_valid_){

			if (f5_){
				u16 err_code = 0;
				char* p =  pk->header();
				if(f5_(this->shared_from_this(), p, pk->length(), err_code) == -1){

					catch_error(err_code);

					free_snd_buffer(pk);

					return;
				}
			}
			boost::asio::async_write(socket_,
					buffer(pk->header(), pk->length()),
					boost::bind(&tcp_svr_connection::write_handler,
							this->shared_from_this(),
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred,
							pk));

		}else{

			free_snd_buffer(pk);
		}
	}

private:
	tcp_svr_connection(boost::asio::io_service& io_service,
			catch_error_callback& 				f1,
			read_pk_header_complete_callback& 	f2,
			read_pk_full_complete_callback& 	f3,
			write_pk_full_complete_callback& 	f4,
			active_send_in_ioservice_callback& 	f5,
			close_complete_callback&			f6)
		: ios_(io_service),
		  socket_(io_service),
		  is_valid_(true),
		  is_authentication_pass_ (false),
		  package_size_(sizeof(TCP_MESSAGE_PACKAGE)),
		  f1_(f1), f2_(f2), f3_(f3),
		  f4_(f4), f5_(f5), f6_(f6){

		set_heartjump_datetime();

		set_authentication_pass_datetime();
	}


	void read_handler(const boost::system::error_code& ec,
			size_t bytes_transferred,
			size_t bytes_requested){

		if(!is_valid_)
			return;

		// Check error.
		if(ec)
		{
			close(ec);
			return;
		}

		// Set mblk's reader data length, and move rd_ptr().
		u16 err_code = 0;
		if( rev_mgr_.change_length(bytes_transferred, err_code) == -1){

			catch_error(err_code);

			close_socket();
			close_completed(ec.value());

			return;
		}

		if(bytes_transferred < bytes_requested){

			// Continue read remain data.
			size_t remain = bytes_requested - bytes_transferred;

			boost::asio::async_read(socket_,
					buffer(rev_mgr_.current(), remain),
					bind(&tcp_svr_connection::read_handler,
							this->shared_from_this(),
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred,
							remain));

		}else if (rev_mgr_.length() == PARSER::header_size){

			size_t remain = 0;
			if(f2_) {
				u16 err_code = 0;
				char* p = rev_mgr_.header();
				if(f2_(this->shared_from_this(), p, rev_mgr_.length(), remain, err_code) == -1){

					catch_error(err_code);

					close_socket();
					close_completed(ec.value());

					return;
				}
			}

			// Check whether remain size is valid.
			if(!rev_mgr_.check_future_data_length(remain)){

				catch_error(error_tcp_package_header_is_wrong);

				close_socket();
				close_completed(ec.value());

				return;
			}

			// Continue read other data.
			boost::asio::async_read(socket_,
					buffer(rev_mgr_.current(), remain),
					bind(&tcp_svr_connection::read_handler,
							this->shared_from_this(),
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred,
							remain));


		}else{
			// finish to recevie Message block.

			if(f3_) {
				u16 err_code = 0;
				char* p = rev_mgr_.header();
				if(f3_(this->shared_from_this(), p, rev_mgr_.length(), err_code) == -1){

					catch_error(err_code);

					close_socket();
					close_completed(ec.value());

					return;
				}
			}

			rev_mgr_.clear();

			// Start new async read data.
			boost::asio::async_read(socket_,
						boost::asio::buffer(rev_mgr_.current(), PARSER::header_size),
						boost::bind(&tcp_svr_connection::read_handler,
									this->shared_from_this(),
									boost::asio::placeholders::error,
									boost::asio::placeholders::bytes_transferred,
									PARSER::header_size));
		}
	}

	void write_handler(const boost::system::error_code& ec, size_t bytes_transferred, package*& pk){

		if(f4_) {
			char* p =  pk->header();
			f4_(this->shared_from_this(), p, bytes_transferred, ec);
		}

		free_snd_buffer(pk);
	}




	void active_close(u16& err_code){

		if(is_valid_){
			catch_error(err_code);

			close_socket();
		}
	}



	void close(const boost::system::error_code& ec){

		using namespace boost::system::errc;
		if(ec.value() != operation_canceled){

			u16 err_code = error_tcp_server_close_socket_because_client;
			catch_error(err_code);

			// Passive close
			close_socket();
		}

		close_completed(ec.value());
	}

	void close_completed(int ec_value){

		is_valid_ = false;

		if(f6_) f6_(this->shared_from_this(), ec_value);
	}





	int malloc_snd_buffer(
			char*& ori_data,
			size_t& ori_data_size,
			package*& pk,
			u16& err_code){

		if(ori_data_size > package_size_){
			err_code = error_tcp_package_snd_exceed_max_size;
			return -1;
		}

		pk = new package();
		pk->copy(ori_data, ori_data_size, err_code);

		return 0;
	}

	void free_snd_buffer(package*& pk){

		delete pk;
		pk = 0;
	}



	// Call this to update p2_.
	void set_authentication_pass_datetime(){
		p2_ = boost::posix_time::microsec_clock::local_time();
	}

private:

	boost::asio::io_service& ios_;
	ip::tcp::socket socket_;

	bool is_valid_;

	package rev_mgr_;
	size_t package_size_;

	catch_error_callback 				f1_;
	read_pk_header_complete_callback 	f2_;
	read_pk_full_complete_callback 		f3_;
	write_pk_full_complete_callback 	f4_;
	active_send_in_ioservice_callback 	f5_;
	close_complete_callback				f6_;


	ptime p1_;						// Save heart jump datetime.

	ptime p2_;						// Save authentication pass datetime.
	bool is_authentication_pass_;   // When authentication pass, this is true, default is false.
};

} } }


#endif /* TCP_SVR_CONNECTION_H_ */
