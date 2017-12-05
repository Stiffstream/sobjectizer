/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.9
 *
 * \file
 * \brief Helper class for accessing private functionality of environment-class.
 */

#pragma once

#include <so_5/rt/h/environment.hpp>

namespace so_5 {

namespace impl {

//
// internal_env_iface_t
//
/*
 * NOTE: the implementation of this class is in environment.cpp file.
 */
/*!
 * \since
 * v.5.5.9
 *
 * \brief A helper class for accessing the functionality of
 * environment-class which is specific for SObjectizer internals only.
 */
class internal_env_iface_t
	{
		//! Environment instance to work with.
		environment_t & m_env;

	public :
		//! Initializing constructor.
		internal_env_iface_t( environment_t & env )
			:	m_env( env )
			{}

		//! Create multi-producer/single-consumer mbox.
		mbox_t
		create_mpsc_mbox(
			//! The only consumer for the messages.
			agent_t * single_consumer,
			//! Pointer to the optional message limits storage.
			//! If this pointer is null then the limitless MPSC-mbox will be
			//! created. If this pointer is not null the the MPSC-mbox with limit
			//! control will be created.
			const so_5::message_limit::impl::info_storage_t * limits_storage );

		//! Notification about readiness to the deregistration.
		void
		ready_to_deregister_notify(
			//! Cooperation which is ready to be deregistered.
			coop_t * coop );

		//! Do the final actions of a cooperation deregistration.
		void
		final_deregister_coop(
			//! Cooperation name to be deregistered.
			const std::string & coop_name );

		//! Is message delivery tracing enabled?
		bool
		is_msg_tracing_enabled() const;

		//! Get access to message delivery tracer object.
		/*!
		 * \throw exception_t if (!is_msg_tracing_enabled()).
		 */
		so_5::msg_tracing::tracer_t &
		msg_tracer() const;

		//! Get default lock_factory for MPSC queues.
		/*!
		 * \since
		 * v.5.5.18
		 */
		so_5::disp::mpsc_queue_traits::lock_factory_t
		default_mpsc_queue_lock_factory() const;

		//! Get default lock_factory for MPMC queues.
		/*!
		 * \since
		 * v.5.5.18
		 */
		so_5::disp::mpmc_queue_traits::lock_factory_t
		default_mpmc_queue_lock_factory() const;
	};

/*!
 * \brief Helper function to be used for extraction of lock_factory for MPSC queues.
 *
 * \note Intended to be used in template code:
 * \code
template< typename Disp_Params >
auto
safe_get_lock_factory( so_5::environment_t & env, Disp_Params & params )
{
	auto lf = params.queue_params().lock_factory();
	if( !lf )
		lf = so_5::impl::default_lock_factory( env, lf );
	return lf;
}
 * \endcode
 * A call to default_lock_factory() will return appropriate lock_factory
 * for MPSC or MPMC queues.
 *
 * \since
 * v.5.5.18
 */
inline so_5::disp::mpsc_queue_traits::lock_factory_t
default_lock_factory(
	environment_t & env,
	const so_5::disp::mpsc_queue_traits::lock_factory_t & )
	{
		return internal_env_iface_t{ env }.default_mpsc_queue_lock_factory();
	}

/*!
 * \brief Helper function to be used for extraction of lock_factory for MPSC queues.
 *
 * \note Intended to be used in template code:
 * \code
template< typename Disp_Params >
auto
safe_get_lock_factory( so_5::environment_t & env, Disp_Params & params )
{
	auto lf = params.queue_params().lock_factory();
	if( !lf )
		lf = so_5::impl::default_lock_factory( env, lf );
	return lf;
}
 * \endcode
 * A call to default_lock_factory() will return appropriate lock_factory
 * for MPSC or MPMC queues.
 *
 * \since
 * v.5.5.18
 */
inline so_5::disp::mpmc_queue_traits::lock_factory_t
default_lock_factory(
	environment_t & env,
	const so_5::disp::mpmc_queue_traits::lock_factory_t & )
	{
		return internal_env_iface_t{ env }.default_mpmc_queue_lock_factory();
	}

} /* namespace impl */

} /* namespace so_5 */

