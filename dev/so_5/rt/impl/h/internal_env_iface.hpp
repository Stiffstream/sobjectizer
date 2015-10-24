/*
 * SObjectizer-5
 */

/*!
 * \since v.5.5.9
 * \file
 * \brief Helper class for accessing private functionality of environment-class.
 */

#pragma once

#include <so_5/rt/h/environment.hpp>

namespace so_5 {

namespace rt {

namespace impl {

//
// internal_env_iface_t
//
/*
 * NOTE: the implementation of this class is in environment.cpp file.
 */
/*!
 * \since v.5.5.9
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
			:	m_env{ env }
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
			const so_5::rt::message_limit::impl::info_storage_t * limits_storage );

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
	};

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */

