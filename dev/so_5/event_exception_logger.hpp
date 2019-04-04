/*
	SObjectizer 5.
*/

/*!
	\file
	\brief An interface for the exception logging definition.
*/

#pragma once

#include <so_5/declspec.hpp>
#include <so_5/compiler_features.hpp>

#include <so_5/coop_handle.hpp>

#include <exception>
#include <string>
#include <memory>

namespace so_5
{

//
// event_exception_logger_t
//

class event_exception_logger_t;

//! Typedef for the event_exception_logger autopointer.
using event_exception_logger_unique_ptr_t =
	std::unique_ptr< event_exception_logger_t >;

//! An interface for the exception logging.
/*!
 * Exception logger should handle two actions:
 *
 * 1. Installation of a logger into the SObjectizer Environment.
 *    The on_install() method could be reimplemented for this.
 * 2. Caught of an exception. The log_exception() method
 *    should be reimplemented for this.
 */
class SO_5_TYPE event_exception_logger_t
{
		// Note: clang-3.9 requires this on Windows platform.
		event_exception_logger_t( const event_exception_logger_t & ) = delete;
		event_exception_logger_t( event_exception_logger_t && ) = delete;
		event_exception_logger_t & operator=( const event_exception_logger_t & ) = delete;
		event_exception_logger_t & operator=( event_exception_logger_t && ) = delete;

	public:
		event_exception_logger_t() = default;
		virtual ~event_exception_logger_t() noexcept = default;

		/*!
		 * \brief Log the exception caught.
		 *
		 * \attention
		 * This method is marked as noexcept in v.5.6.0
		 */
		virtual void
		log_exception(
			//! Exception caught.
			const std::exception & event_exception,
			//! Cooperation to which agent is belong.
			const coop_handle_t & coop ) noexcept = 0;

		/*!
		 * \brief Installation hook.
		 *
		 * A new exception logger should decide what happened with
		 * the old logger.
		 *
		 * Default implementation simply erases the old logger.
		 *
		 * \attention
		 * This method is marked as noexcept in v.5.6.0
		 */
		virtual void
		on_install(
			//! Old logger.
			event_exception_logger_unique_ptr_t previous_logger ) noexcept;

};

//
// create_std_event_exception_logger
//

//! Create the default exception logger.
SO_5_FUNC event_exception_logger_unique_ptr_t
create_std_event_exception_logger();

} /* namespace so_5 */

