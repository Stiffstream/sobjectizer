/*
	SObjectizer 5
*/

#include <so_5/disp/active_obj/h/pub.hpp>
#include <so_5/disp/active_obj/impl/h/disp.hpp> 
#include <so_5/disp/active_obj/impl/h/disp_binder.hpp> 

namespace so_5
{

namespace disp
{

namespace active_obj
{

SO_5_FUNC so_5::rt::dispatcher_unique_ptr_t
create_disp()
{
	return so_5::rt::dispatcher_unique_ptr_t(
		new impl::dispatcher_t );
}

SO_5_FUNC so_5::rt::disp_binder_unique_ptr_t
create_disp_binder( const std::string & disp_name )
{
	return so_5::rt::disp_binder_unique_ptr_t( 
		new impl::disp_binder_t( disp_name ) );
}

} /* namespace active_obj */

} /* namespace disp */

} /* namespace so_5 */
