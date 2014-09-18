/*
	SObjectizer 5.
*/

#include <iostream>

#include <so_5/rt/h/event_exception_logger.hpp>

namespace so_5
{

namespace rt
{

//
// event_exception_logger_t
//

event_exception_logger_t::~event_exception_logger_t()
{}

void
event_exception_logger_t::on_install(
	event_exception_logger_unique_ptr_t previous_logger )
{
	// Default implementation deletes old logger.
	previous_logger.reset();
}

namespace /* ananymous */
{

//
// std_event_exception_logger_t
//

//! Standard exception logger implementation.
class std_event_exception_logger_t
	:
		public event_exception_logger_t
{
	public:
		virtual ~std_event_exception_logger_t()
		{}

		virtual void
		log_exception(
			const std::exception & event_exception,
			const std::string & coop_name )
		{
			std::cerr
				<< "SObjectizer event exception caught: "
				<< event_exception.what()
				<< "; cooperation: '" << coop_name << "'"
				<< std::endl;
		}
};

} /* anonymous namespace */

//
// create_std_event_exception_logger
//

CPP_UTIL_2_EXPORT_FUNC_SPEC( event_exception_logger_unique_ptr_t )
create_std_event_exception_logger()
{
	return event_exception_logger_unique_ptr_t(
		new std_event_exception_logger_t );
}

} /* namespace rt */

} /* namespace so_5 */
