/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief Basic part of coop_repository functionality.
 *
 * \since
 * v.5.5.19
 */
#include <so_5/impl/coop_repository_basis.hpp>

#include <so_5/environment.hpp>

#include <so_5/details/rollback_on_exception.hpp>
#include <so_5/details/abort_on_fatal_error.hpp>

#include <cstdlib>
#include <algorithm>

namespace so_5
{

namespace impl
{

coop_repository_basis_t::coop_repository_basis_t(
	outliving_reference_t< environment_t > environment,
	coop_listener_unique_ptr_t /*coop_listener*/ )
	:	m_env{ environment }
{
}

SO_5_NODISCARD
coop_unique_ptr_t
coop_repository_basis_t::make_coop(
	coop_handle_t parent,
	disp_binder_shptr_t default_binder )
{
//FIXME: implement this!
(void)parent;
(void)default_binder;
return {};
}

#if 0
namespace
{
	/*!
	 * \since
	 * v.5.2.3
	 *
	 * \brief Special guard to increment and decrement cooperation
	 * usage counters.
	 */
	class coop_usage_counter_guard_t
	{
		public :
			coop_usage_counter_guard_t( coop_t & coop )
				:	m_coop( coop )
			{
				coop_t::so_increment_usage_count( coop );
			}
			~coop_usage_counter_guard_t()
			{
				coop_t::so_decrement_usage_count( m_coop );
			}

		private :
			coop_t & m_coop;
	};

} /* namespace anonymous */
#endif

coop_handle_t
coop_repository_basis_t::register_coop(
	coop_unique_ptr_t /*coop_ptr*/ )
{
//FIXME: implement this!
return {};
}

void
coop_repository_basis_t::deregister_coop(
	coop_handle_t coop,
	coop_dereg_reason_t dereg_reason )
{
//FIXME: implement this!
(void)coop;
(void)dereg_reason;
}

coop_repository_basis_t::final_deregistration_resul_t
coop_repository_basis_t::final_deregister_coop(
	coop_shptr_t coop )
{
//FIXME: implement this!
(void)coop;

	return final_deregistration_resul_t{false, true};
}

std::size_t
coop_repository_basis_t::deregister_all_coop() noexcept
{
//FIXME: implement this!
return 0u;
}

coop_repository_basis_t::initiate_deregistration_result_t
coop_repository_basis_t::initiate_deregistration()
{
	initiate_deregistration_result_t result =
			initiate_deregistration_result_t::already_in_progress;

//FIXME: implement this!
	return result;
}

environment_t &
coop_repository_basis_t::environment()
{
	return m_env.get();
}

environment_infrastructure_t::coop_repository_stats_t
coop_repository_basis_t::query_stats()
{
//FIXME: implement this!
	return {
			0u,
			0u,
			0u,
			0u
		};
}

} /* namespace impl */

} /* namespace so_5 */

