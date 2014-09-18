/*
	SObjectizer 5.
*/

#include <so_5/disp/one_thread/h/pub.hpp>
#include <so_5/disp/one_thread/impl/h/disp.hpp>
#include <so_5/disp/one_thread/impl/h/disp_binder.hpp>

namespace so_5
{

namespace disp
{

namespace one_thread
{

SO_5_EXPORT_FUNC_SPEC( so_5::rt::dispatcher_unique_ptr_t )
create_disp()
{
	return so_5::rt::dispatcher_unique_ptr_t(
		new impl::dispatcher_t );
}

SO_5_EXPORT_FUNC_SPEC( so_5::rt::disp_binder_unique_ptr_t )
create_disp_binder( const std::string & disp_name )
{
	return so_5::rt::disp_binder_unique_ptr_t(
		new impl::disp_binder_t( disp_name ) );
}

} /* namespace one_thread */

} /* namespace disp */

namespace rt
{

SO_5_EXPORT_FUNC_SPEC( disp_binder_unique_ptr_t )
create_default_disp_binder()
{
	// Dispatcher with empty name means default dispatcher.
	return so_5::disp::one_thread::create_disp_binder( std::string() );
}

} /* namespace rt */

} /* namespace so_5 */

