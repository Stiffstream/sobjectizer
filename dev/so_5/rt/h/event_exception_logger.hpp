/*
	SObjectizer 5.
*/

/*!
	\file
	\brief An interface for the exception logging definition.
*/

#if !defined( _SO_5__RT__EVENT_EXCEPTION_LOGGER_HPP_ )
#define _SO_5__RT__EVENT_EXCEPTION_LOGGER_HPP_

#include <exception>
#include <string>
#include <memory>

#include <so_5/h/declspec.hpp>

namespace so_5
{

namespace rt
{

//
// event_exception_logger_t
//

class event_exception_logger_t;

//! Typedef for the event_exception_logger autopointer.
typedef std::unique_ptr< event_exception_logger_t >
	event_exception_logger_unique_ptr_t;

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
	public:
		virtual ~event_exception_logger_t();

		/*!
		 * \brief Log the exception caught.
		*/
		virtual void
		log_exception(
			//! Exception caught.
			const std::exception & event_exception,
			//! Name of cooperation to which agent is belong.
			const std::string & coop_name ) = 0;

		/*!
		 * \brief Installation hook.
		 *
		 * A new exception logger should decide what happened with
		 * the old logger.
		 *
		 * Default implementation simply erases the old logger.
		 */
		virtual void
		on_install(
			//! Old logger.
			event_exception_logger_unique_ptr_t previous_logger );
};

//
// create_std_event_exception_logger
//

//! Create the default exception logger.
CPP_UTIL_2_EXPORT_FUNC_SPEC( event_exception_logger_unique_ptr_t )
create_std_event_exception_logger();

} /* namespace rt */

} /* namespace so_5 */

#endif
