/*
	SObjectizer 5.
*/

/*!
 * \since v.5.5.8
 * \file
 * \brief Functions for creating and binding of the single thread dispatcher
 * with priority support.
 */

#pragma once

#include <string>

#include <so_5/h/declspec.hpp>

#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/disp_binder.hpp>

#include <so_5/h/priority.hpp>

#include <so_5/disp/mpsc_queue_traits/h/pub.hpp>

namespace so_5 {

namespace disp {

namespace prio_one_thread {

namespace strictly_ordered {

/*!
 * \since v.5.5.10
 * \brief Alias for namespace with traits of event queue.
 */
namespace queue_traits = so_5::disp::mpsc_queue_traits;

//
// disp_params_t
//
/*!
 * \since v.5.5.10
 * \brief Parameters for a dispatcher.
 */
class disp_params_t
	{
	public :
		//! Default constructor.
		disp_params_t() {}
		//! Copy constructor.
		disp_params_t( const disp_params_t & o )
			:	m_queue_params{ o.m_queue_params }
			{}
		//! Move constructor.
		disp_params_t( disp_params_t && o )
			:	m_queue_params{ std::move(o.m_queue_params) }
			{}

		friend inline void swap( disp_params_t & a, disp_params_t & b )
			{
				swap( a.m_queue_params, b.m_queue_params );
			}

		//! Copy operator.
		disp_params_t & operator=( const disp_params_t & o )
			{
				disp_params_t tmp{ o };
				swap( *this, tmp );
				return *this;
			}
		//! Move operator.
		disp_params_t & operator=( disp_params_t && o )
			{
				disp_params_t tmp{ std::move(o) };
				swap( *this, tmp );
				return *this;
			}

		//! Setter for queue parameters.
		disp_params_t &
		set_queue_params( queue_traits::queue_params_t p )
			{
				m_queue_params = std::move(p);
				return *this;
			}

		//! Tuner for queue parameters.
		/*!
		 * Accepts lambda-function or functional object which tunes
		 * queue parameters.
			\code
			namespace prio_disp = so_5::disp::prio_one_thread::strictly_ordered;
			prio_disp::create_private_disp( env,
				"my_prio_disp",
				prio_disp::disp_params_t{}.tune_queue_params(
					[]( prio_disp::queue_traits::queue_params_t & p ) {
						p.lock_factory( prio_disp::queue_traits::simple_lock_factory() );
					} ) );
			\endcode
		 */
		template< typename L >
		disp_params_t &
		tune_queue_params( L tunner )
			{
				tunner( m_queue_params );
				return *this;
			}

		//! Getter for queue parameters.
		const queue_traits::queue_params_t &
		queue_params() const
			{
				return m_queue_params;
			}

	private :
		//! Queue parameters.
		queue_traits::queue_params_t m_queue_params;
	};

//
// params_t
//
/*!
 * \brief Old alias for disp_params for compatibility with previous versions.
 * \deprecated Use disp_params_t instead.
 */
using params_t = disp_params_t;

//
// private_dispatcher_t
//

/*!
 * \since v.5.5.8
 * \brief An interface for %strictly_ordered private dispatcher.
 */
class SO_5_TYPE private_dispatcher_t : public so_5::atomic_refcounted_t
	{
	public :
		virtual ~private_dispatcher_t();

		//! Create a binder for that private dispatcher.
		virtual so_5::rt::disp_binder_unique_ptr_t
		binder() = 0;
	};

/*!
 * \since v.5.5.8
 * \brief A handle for the %strictly_ordered private dispatcher.
 */
using private_dispatcher_handle_t =
	so_5::intrusive_ptr_t< private_dispatcher_t >;

/*!
 * \since v.5.5.10
 * \brief Create an instance of dispatcher to be used as named dispatcher.
 */
SO_5_FUNC so_5::rt::dispatcher_unique_ptr_t
create_disp(
	//! Parameters for dispatcher.
	disp_params_t params );

//! Create a dispatcher.
inline so_5::rt::dispatcher_unique_ptr_t
create_disp()
	{
		return create_disp( disp_params_t{} );
	}

/*!
 * \since v.5.5.10
 * \brief Create a private %strictly_ordered dispatcher.
 *
 * \par Usage sample
\code
auto common_thread_disp = so_5::disp::prio_one_thread::strictly_ordered::create_private_disp(
	env,
	"request_processor",
	so_5::disp::prio_one_thread::strictly_ordered::disp_params_t{}.tune_queue_params(
		[]( so_5::disp::prio_one_thread::strictly_ordered::queue_traits::queue_params_t & p ) {
			p.lock_factory( so_5::disp::prio_one_thread::strictly_ordered::queue_traits::simple_lock_factory() );
		} ) );
auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private strictly_ordered dispatcher.
	common_thread_disp->binder() );
\endcode
 */
SO_5_FUNC private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	so_5::rt::environment_t & env,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string & data_sources_name_base,
	//! Parameters for the dispatcher.
	disp_params_t params );

/*!
 * \since v.5.5.8
 * \brief Create a private %strictly_ordered dispatcher.
 *
 * \par Usage sample
\code
auto common_thread_disp = so_5::disp::prio_one_thread::strictly_ordered::create_private_disp(
	env,
	"request_processor" );
auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private strictly_ordered dispatcher.
	common_thread_disp->binder() );
\endcode
 */
inline private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	so_5::rt::environment_t & env,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string & data_sources_name_base )
	{
		return create_private_disp( env, data_sources_name_base, disp_params_t{} );
	}

/*!
 * \since v.5.5.8
 * \brief Create a private %strictly_ordered dispatcher.
 *
 * \par Usage sample
\code
auto common_thread_disp = so_5::disp::prio_one_thread::strictly_ordered::create_private_disp( env );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private strictly_ordered dispatcher.
	common_thread_disp->binder() );
\endcode
 */
inline private_dispatcher_handle_t
create_private_disp( so_5::rt::environment_t & env )
	{
		return create_private_disp( env, std::string() );
	}

//! Create a dispatcher binder object.
SO_5_FUNC so_5::rt::disp_binder_unique_ptr_t
create_disp_binder(
	//! Name of the dispatcher to be bound to.
	const std::string & disp_name );

} /* namespace strictly_ordered */

} /* namespace prio_one_thread */

} /* namespace disp */

} /* namespace so_5 */

