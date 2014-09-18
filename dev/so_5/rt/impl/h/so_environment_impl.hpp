/*
	SObjectizer 5.
*/

/*!
	\file
	\brief An implementation of the SObjectizer Environment.
*/

#if !defined( _SO_5__RT__IMPL__SO_ENVIRONMENT_IMPL_HPP_ )
#define _SO_5__RT__IMPL__SO_ENVIRONMENT_IMPL_HPP_

#include <so_5/rt/impl/h/layer_core.hpp>
#include <so_5/rt/h/so_environment.hpp>

#include <so_5/rt/impl/h/mbox_core.hpp>
#include <so_5/rt/impl/h/agent_core.hpp>
#include <so_5/rt/impl/h/disp_core.hpp>
#include <so_5/rt/impl/h/layer_core.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

//
// so_environment_impl_t
//

//! An implementation of the SObjectizer Environment.
class so_environment_impl_t
{
	public:
		explicit so_environment_impl_t(
			so_environment_params_t && so_environment_params,
			so_environment_t & public_so_environment );

		~so_environment_impl_t();

		/*!
		 * \name Method for work with mboxes.
		 * \{
		 */
		inline mbox_ref_t
		create_local_mbox()
		{
			return m_mbox_core->create_local_mbox();
		}

		inline mbox_ref_t
		create_local_mbox(
			//! Mbox name.
			const nonempty_name_t & mbox_name )
		{
			return m_mbox_core->create_local_mbox( mbox_name );
		}

		inline mbox_ref_t
		create_mpsc_mbox(
			//! The only consumer for the messages.
			agent_t * single_consumer,
			//! Event queue proxy for the consumer.
			event_queue_proxy_ref_t event_queue )
		{
			return m_mbox_core->create_mpsc_mbox(
					single_consumer,
					std::move( event_queue ) );
		}
		/*!
		 * \}
		 */

		/*!
		 * \name Methods for work with dispatchers.
		 * \{
		 */

		//! Get default dispatcher.
		inline dispatcher_t &
		query_default_dispatcher()
		{
			return m_disp_core.query_default_dispatcher();
		}

		//! Get named dispatcher.
		inline dispatcher_ref_t
		query_named_dispatcher(
			const std::string & disp_name )
		{
			return m_disp_core.query_named_dispatcher( disp_name );
		}

		//! Set up an exception logger.
		inline void
		install_exception_logger(
			event_exception_logger_unique_ptr_t logger )
		{
			m_disp_core.install_exception_logger( std::move( logger ) );
		}

		//! Add dispatcher if not exists.
		/*!
		 * \since v.5.4.0
		 */
		dispatcher_ref_t
		add_dispatcher_if_not_exists(
			const std::string & disp_name,
			std::function< dispatcher_unique_ptr_t() > disp_factory )
		{
			return m_disp_core.add_dispatcher_if_not_exists(
					disp_name, disp_factory );
		}

		/*!
		 * \since v.5.2.3.
		 * \brief Call event exception logger for logging an exception.
		 */
		inline void
		call_exception_logger(
			//! Exception caught.
			const std::exception & event_exception,
			//! A cooperation to which agent is belong.
			const std::string & coop_name )
		{
			m_disp_core.call_exception_logger( event_exception, coop_name );
		}
		/*!
		 * \}
		 */

		/*!
		 * \name Methods for work with cooperations.
		 * \{
		 */

		//! Register an agent cooperation.
		void
		register_coop(
			//! Cooperation to be registered.
			agent_coop_unique_ptr_t agent_coop )
		{
			m_agent_core.register_coop( std::move( agent_coop ) );
		}

		//! Deregister an agent cooperation.
		void
		deregister_coop(
			//! Cooperation to be deregistered.
			const nonempty_name_t & name,
			//! Deregistration reason.
			int reason )
		{
			m_agent_core.deregister_coop(
					name,
					coop_dereg_reason_t( reason ) );
		}

		//! Notification about readiness to the deregistration.
		inline void
		ready_to_deregister_notify(
			agent_coop_t * coop )
		{
			m_agent_core.ready_to_deregister_notify( coop );
		}

		//! Do the final actions of a cooperation deregistration.
		inline void
		final_deregister_coop(
			//! Cooperation name to be deregistered.
			const std::string & coop_name )
		{
			bool any_cooperation_alive = 
					m_agent_core.final_deregister_coop( coop_name );

			if( !any_cooperation_alive && !m_autoshutdown_disabled )
				stop();
		}
		/*!
		 * \}
		 */

		/*!
		 * \name Methods for work with timer events.
		 * \{
		 */

		//! Schedule timer event.
		so_5::timer_id_t
		schedule_timer(
			//! Message type.
			const std::type_index & type_wrapper,
			//! Message to be sent.
			const message_ref_t & msg,
			//! Mbox receiver.
			const mbox_ref_t & mbox,
			//! A delay for the first sent.
			unsigned int delay_msec,
			//! Timeout for the periodic delivery.
			//! Must be 0 for delayed messages.
			unsigned int period_msec );

		//! Schedule a single-shot timer event.
		void
		single_timer(
			//! Message type.
			const std::type_index & type_wrapper,
			//! Message to be sent.
			const message_ref_t & msg,
			//! Mbox receiver.
			const mbox_ref_t & mbox,
			//! A delay for the delivery.
			unsigned int delay_msec );

		/*!
		 * \}
		 */

		/*!
		 * \name Methods for work with layers.
		 * \{
		 */
		so_layer_t *
		query_layer(
			const std::type_index & type ) const;

		//! Add an extra layer.
		/*!
			\see layer_core_t::add_extra_layer().
		*/
		void
		add_extra_layer(
			const std::type_index & type,
			const so_layer_ref_t & layer );
		/*!
		 * \}
		 */

		/*!
		 * \name Start, initialization, shutting down.
		 * \{
		 */
		void
		run(
			so_environment_t & env );

		void
		stop();
		/*!
		 * \}
		 */

		//! Get reference to the SObjectizer Environment.
		so_environment_t &
		query_public_so_environment();

		/*!
		 * \since v.5.3.0
		 * \brief An exception reaction for the whole SO Environment.
		 */
		exception_reaction_t
		exception_reaction() const;

		/*!
		 * \since v.5.5.0
		 * \brief Get the error_logger object.
		 */
		error_logger_t &
		error_logger() const;

	private:
		/*!
		 * \since v.5.5.0
		 * \brief Error logger object for this environment.
		 *
		 * \attention Must be the first attribute of the object!
		 * It must be created and initilized first and destroyed last.
		 */
		error_logger_shptr_t m_error_logger;

		//! An utility for mboxes.
		mbox_core_ref_t m_mbox_core;

		//! An utility for agents/cooperations.
		agent_core_t m_agent_core;

		//! An utility for dispatchers.
		disp_core_t m_disp_core;

		//! An utility for layers.
		layer_core_t m_layer_core;

		//! Reference to the SObjectizer Environment.
		so_environment_t & m_public_so_environment;

		//! Timer.
		so_5::timer_thread_unique_ptr_t m_timer_thread;

		/*!
		 * \since v.5.3.0
		 * \brief An exception reaction for the whole SO Environment.
		 */
		const exception_reaction_t m_exception_reaction;

		/*!
		 * \since v.5.4.0
		 * \brief Is autoshutdown when there is no more cooperation disabled?
		 *
		 * \see so_environment_params_t::disable_autoshutdown()
		 */
		const bool m_autoshutdown_disabled;

		/*!
		 * \since v.5.2.0
		 * \brief Run layers and call next run stage.
		 */
		void
		run_layers_and_go_further(
			so_environment_t & env );

		/*!
		 * \since v.5.2.0
		 * \brief Run dispatchers and call next run stage.
		 */
		void
		run_dispatcher_and_go_further(
			so_environment_t & env );

		/*!
		 * \since v.5.2.0
		 * \brief Run timer and call next run stage.
		 */
		void
		run_timer_and_go_further(
			so_environment_t & env );

		/*!
		 * \since v.5.2.0
		 * \brief Run agent core and call next run stage.
		 */
		void
		run_agent_core_and_go_further(
			so_environment_t & env );

		/*!
		 * \since v.5.2.0
		 * \brief Run customer's initialization routine and wait for
		 * start of deregistration procedure.
		 */
		void
		run_user_supplied_init_and_wait_for_stop(
			so_environment_t & env );

		/*!
		 * \since v.5.2.0
		 * \brief Templated implementation of one run stage.
		 */
		void
		do_run_stage(
			//! Short description of stage.
			const std::string & stage_name,
			//! Stage initialization code.
			std::function< void() > init_fn,
			//! Stage deinitialization code.
			std::function< void() > deinit_fn,
			//! Next stage method.
			std::function< void() > next_stage );
};

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */

#endif
