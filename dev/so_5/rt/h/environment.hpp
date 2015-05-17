/*
	SObjectizer 5.
*/

/*!
	\file
	\brief SObjectizer Environment definition.
*/

#pragma once

#include <functional>
#include <chrono>
#include <memory>
#include <type_traits>

#include <so_5/h/compiler_features.hpp>
#include <so_5/h/declspec.hpp>
#include <so_5/h/exception.hpp>
#include <so_5/h/error_logger.hpp>
#include <so_5/h/compiler_features.hpp>

#include <so_5/rt/h/nonempty_name.hpp>
#include <so_5/rt/h/mbox.hpp>
#include <so_5/rt/h/message.hpp>
#include <so_5/rt/h/agent_coop.hpp>
#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/disp_binder.hpp>
#include <so_5/rt/h/so_layer.hpp>
#include <so_5/rt/h/coop_listener.hpp>
#include <so_5/rt/h/event_exception_logger.hpp>

#include <so_5/h/timers.hpp>

#include <so_5/rt/stats/h/controller.hpp>
#include <so_5/rt/stats/h/repository.hpp>

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace so_5
{

/*!
 * \since v.5.5.1
 * \brief Special type for autoname-cooperation implementation.
 */
struct autoname_indicator_t {};

/*!
 * \since v.5.5.1
 * \brief Special marker for indication of automatic name generation.
 */
inline autoname_indicator_t
autoname() { return autoname_indicator_t(); }

namespace rt
{

//
// environment_params_t
//

//! Parameters for the SObjectizer Environment initialization.
/*!
 * This class is used for setting SObjectizer Parameters.
 *
 * \see http://www.parashift.com/c++-faq/named-parameter-idiom.html
 */
class SO_5_TYPE environment_params_t
{
	public:
		/*!
		 * \brief Constructor.
		 *
		 * Sets default values for parameters.
		 */
		environment_params_t();
		/*!
		 * \since v.5.2.3
		 * \brief Move constructor.
		 */
		environment_params_t( environment_params_t && other );
		~environment_params_t();

		/*!
		 * \since v.5.2.3
		 * \brief Move operator.
		 */
		environment_params_t &
		operator=( environment_params_t && other );

		/*!
		 * \since v.5.2.3
		 * \brief Swap operation.
		 */
		void
		swap( environment_params_t & other );

		//! Add a named dispatcher.
		/*!
		 * By default the SObjectizer Environment has only one dispatcher
		 * with one working thread. A user can add his own dispatcher:
		 * named ones.
		 *
		 * \note If a dispatcher with \a name is already registered it
		 * will be replaced by a new dispatcher \a dispatcher.
		 */
		environment_params_t &
		add_named_dispatcher(
			//! Dispatcher name.
			const nonempty_name_t & name,
			//! Dispatcher.
			dispatcher_unique_ptr_t dispatcher );

		//! Set the timer_thread factory.
		/*!
		 * If \a factory is a null then the default timer thread
		 * will be used.
		 */
		environment_params_t &
		timer_thread(
			//! timer_thread factory to be used.
			so_5::timer_thread_factory_t factory );

		//! Add an additional layer to the SObjectizer Environment.
		/*!
		 * If this layer is already added it will be replaced by \a layer_ptr.
		 * 
		 * The method distinguishes layers from each other by the type SO_LAYER.
		*/
		template< class SO_LAYER >
		environment_params_t &
		add_layer(
			//! A layer to be added.
			std::unique_ptr< SO_LAYER > layer_ptr )
		{
			if( layer_ptr.get() )
			{
				so_layer_unique_ptr_t ptr( layer_ptr.release() );

				add_layer(
					std::type_index( typeid( SO_LAYER ) ),
					std::move( ptr ) );
			}

			return *this;
		}

		//! Add an additional layer to the SObjectizer Environment.
		/*!
		 * SObjectizer Environment takes control about \a layer_raw_ptr life time.
		 *
		 * If this layer is already added it will be replaced by \a layer_raw_ptr.
		 * 
		 * The method distinguishes layers from each other by the type SO_LAYER.
		*/
		template< class SO_LAYER >
		environment_params_t &
		add_layer(
			//! A layer to be added.
			SO_LAYER * layer_raw_ptr )
		{
			return add_layer( std::unique_ptr< SO_LAYER >( layer_raw_ptr ) );
		}

		//! Set cooperation listener object.
		environment_params_t &
		coop_listener(
			coop_listener_unique_ptr_t coop_listener );

		//! Set exception logger object.
		environment_params_t &
		event_exception_logger(
			event_exception_logger_unique_ptr_t logger );

		/*!
		 * \name Exception reaction flag management methods.
		 * \{
		 */
		/*!
		 * \since v.5.3.0
		 * \brief Get exception reaction flag value.
		 */
		inline exception_reaction_t
		exception_reaction() const
		{
			return m_exception_reaction;
		}

		/*!
		 * \since v.5.3.0
		 * \brief Set exception reaction flag value.
		 */
		environment_params_t &
		exception_reaction( exception_reaction_t value )
		{
			m_exception_reaction = value;
			return *this;
		}
		/*!
		 * \}
		 */

		/*!
		 * \since v.5.4.0
		 * \brief Do not shutdown SO Environment when it is becomes empty.
		 *
		 * \par Description
		 * Since v.5.4.0 SO Environment checks count of live cooperations
		 * after every cooperation deregistration. If there is no more
		 * live cooperations then SO Environment will be shutted down.
		 * If it is not appropriate then this method must be called.
		 * It disables autoshutdown of SO Environment. Event if there is
		 * no more live cooperations SO Environment will work until
		 * explisit call to environment_t::stop() method.
		 */
		environment_params_t &
		disable_autoshutdown()
		{
			m_autoshutdown_disabled = true;
			return *this;
		}

		/*!
		 * \since v.5.4.0
		 * \brief Is autoshutdown disabled?
		 *
		 * \see disable_autoshutdown()
		 */
		bool
		autoshutdown_disabled() const
		{
			return m_autoshutdown_disabled;
		}

		/*!
		 * \since v.5.5.0
		 * \brief Set error logger for the environment.
		 */
		environment_params_t &
		error_logger( const error_logger_shptr_t & logger )
		{
			m_error_logger = logger;
			return *this;
		}

		/*!
		 * \name Methods for internal use only.
		 * \{
		 */
		//! Get map of named dispatchers.
		named_dispatcher_map_t
		so5__giveout_named_dispatcher_map()
		{
			return std::move( m_named_dispatcher_map );
		}

		//! Get map of default SObjectizer's layers.
		const so_layer_map_t &
		so5__layers_map() const
		{
			return m_so_layers;
		}

		//! Get cooperation listener.
		coop_listener_unique_ptr_t
		so5__giveout_coop_listener()
		{
			return std::move( m_coop_listener );
		}

		//! Get exception logger.
		event_exception_logger_unique_ptr_t
		so5__giveout_event_exception_logger()
		{
			return std::move( m_event_exception_logger );
		}

		//! Get the timer_thread factory.
		so_5::timer_thread_factory_t
		so5__giveout_timer_thread_factory()
		{
			return std::move( m_timer_thread_factory );
		}

		//! Get error logger for the environment.
		const error_logger_shptr_t &
		so5__error_logger() const
		{
			return m_error_logger;
		}
		/*!
		 * \}
		 */

	private:
		//! Add an additional layer.
		/*!
		 * If this layer is already added it will be replaced by \a layer_ptr.
		 * 
		 * The method distinguishes layers from each other by the type SO_LAYER.
		 */
		void
		add_layer(
			//! Type identification for layer.
			const std::type_index & type,
			//! A layer to be added.
			so_layer_unique_ptr_t layer_ptr );

		//! Named dispatchers.
		named_dispatcher_map_t m_named_dispatcher_map;

		//! Timer thread factory.
		so_5::timer_thread_factory_t m_timer_thread_factory;

		//! Additional layers.
		so_layer_map_t m_so_layers;

		//! Cooperation listener.
		coop_listener_unique_ptr_t m_coop_listener;

		//! Exception logger.
		event_exception_logger_unique_ptr_t m_event_exception_logger;

		/*!
		 * \since v.5.3.0
		 * \brief Exception reaction flag for the whole SO Environment.
		 */
		exception_reaction_t m_exception_reaction;

		/*!
		 * \since v.5.4.0
		 * \brief Is autoshutdown when there is no more cooperation disabled?
		 *
		 * \see disable_autoshutdown()
		 */
		bool m_autoshutdown_disabled;

		/*!
		 * \since v.5.5.0
		 * \brief Error logger for the environment.
		 */
		error_logger_shptr_t m_error_logger;
};

//
// so_environment_params_t
//
/*!
 * \brief Old name for compatibility with previous versions.
 * \deprecated Obsolete in 5.5.0
 */
typedef environment_params_t so_environment_params_t;

//
// environment_t
//

//! SObjectizer Environment.
/*!
 * \section so_env__intro Basic information
 *
 * The SObjectizer Environment provides a basic infrastructure for
 * the SObjectizer Run-Time execution.
 *
 * The main method of starting SObjectizer Environment creates a
 * class derived from the environment_t and reimplementing the
 * environment_t::init() method.
 * This method should be used to define starting actions of
 * application. For example first application cooperations can
 * be registered here and starting messages can be sent to them.
 *
 * The SObjectizer Environment calls the environment_t::init() when
 * the SObjectizer Run-Time is successfully started. 
 * If something happened during the Run-Time startup then 
 * the method init() will not be called.
 *
 * The SObjectizer Run-Time is started by the environment_t::run().
 * This method blocks the caller thread until SObjectizer completely
 * finished its work.
 *
 * The SObjectizer Run-Time is finished by the environment_t::stop().
 * This method doesn't block the caller thread. Instead it sends a special
 * shutdown signal to the Run-Time. The SObjectizer Run-Time then 
 * informs agents about this and waits finish of agents work.
 * The SObjectizer Run-Time finishes if all agents are stopped and
 * all cooperations are deregistered.
 *
 * Methods of the SObjectizer Environment can be splitted into the
 * following groups:
 * - working with mboxes;
 * - working with dispatchers, exception loggers and handlers;
 * - working with cooperations;
 * - working with delayed and periodic messages;
 * - working with additional layers;
 * - initializing/running/stopping/waiting of the Run-Time.
 *
 * \section so_env__mbox_methods Methods for working with mboxes.
 *
 * SObjectizer Environment allows creation of named and anonymous mboxes.
 * Syncronization objects for these mboxes can be obtained from
 * common pools or assigned by a user during mbox creation.
 *
 * Mboxes are created by environment_t::create_local_mbox() methods.
 * All these methods return the mbox_t which is a smart reference 
 * to the mbox.
 *
 * An anonymous mbox is automatically destroyed when the last reference to it is
 * destroyed. So, to save the anonymous mbox, the mbox_ref from 
 * the create_local_mbox() should be stored somewhere.
 *
 * Named mbox must be destroyed manually by calling the 
 * environment_t::destroy_mbox() method. But physically the deletion of the 
 * named mbox postponed to the deletion of last reference to it. 
 * So if there is some reference to the named mbox it instance will live 
 * with this reference. But mbox itself will be removed from 
 * SObjectizer Environment lists.
 *
 *
 * \section so_env__coop_methods Methods for working with cooperations.
 *
 * Cooperations can be created by environment_t::create_coop() methods.
 *
 * The method environment_t::register_coop() should be used for the 
 * cooperation registration.
 *
 * Method environment_t::deregister_coop() should be used for the 
 * cooperation deregistration.
 *
 * \section so_env__delayed_message_methods Methods for 
 * sending delayed and periodic messages.
 *
 * Receiving of delayed and/or periodic messages are named as timer events.
 * 
 * The timer event can be created and destroyed. If the timer event for
 * a delayed message is destroyed before message timeout is expired then
 * message delivery will be canceled. For periodic messages destroying of
 * the timer event means that message delivery will be stopped.
 *
 * Timer events can be created by environment_t::schedule_timer()
 * methods. The one version of the schedule_timer() is intended for
 * messages with an actual data. The second one -- for the signals without
 * the message data.
 *
 * Methods schedule_timer() return a special reference for the timer event.
 * Timer event destroyed when this reference destroyed. So it is necessary
 * to store this reference somewhere. Also the timer event can be destroyed
 * by the so_5::timer_thread::timer_id_ref_t::release() method.
 *
 * A special method environment_t::single_timer() can be used in
 * case when a single shot timer event is necessary. With using this
 * method there is no need to store reference for the scheduled
 * single shot timer event.
 */
class SO_5_TYPE environment_t
{
		//! Auxiliary methods for getting reference to itself.
		/*!
		 * Could be used in constructors without compiler warnings.
		 */
		environment_t &
		self_ref();

	public:
		explicit environment_t(
			//! Initialization params.
			environment_params_t && so_environment_params );

		virtual ~environment_t();

		environment_t( const environment_t & ) = delete;
		environment_t &
		operator=( const environment_t & ) = delete;

		/*!
		 * \name Methods for working with mboxes.
		 * \{
		 */

		//! Create an anonymous mbox with the default mutex.
		/*!
		 *	\note always creates a new mbox.
		 */
		mbox_t
		create_local_mbox();

		//! Create named mbox with the default mutex.
		/*!
		 * If \a mbox_name is unique then a new mutex will be created.
		 * If not the reference to existing mutex will be returned.
		 */
		mbox_t
		create_local_mbox(
			//! Mbox name.
			const nonempty_name_t & mbox_name );
		/*!
		 * \}
		 */

		/*!
		 * \name Method for working with dispatchers.
		 * \{
		 */

		//! Access to the default dispatcher.
		dispatcher_t &
		query_default_dispatcher();

		//! Get named dispatcher.
		/*!
		 * \return A reference to the dispatcher with the name \a disp_name.
		 * Zero reference if a dispatcher with such name is not found.
		 */
		dispatcher_ref_t
		query_named_dispatcher(
			//! Dispatcher name.
			const std::string & disp_name );

		//! Set up an exception logger.
		void
		install_exception_logger(
			event_exception_logger_unique_ptr_t logger );

		/*!
		 * \since v.5.4.0
		 * \brief Add named dispatcher if it is not exists.
		 *
		 * \par Usage:
			\code
			so_5::rt::environment_t & env = ...;
			env.add_dispatcher_if_not_exists(
				"my_coop_dispatcher",
				[]() { so_5::disp::one_thread::create_disp(); } );
			\endcode
		 *
		 * \throw so_5::exception_t if dispatcher cannot be added.
		 */
		dispatcher_ref_t
		add_dispatcher_if_not_exists(
			//! Dispatcher name.
			const std::string & disp_name,
			//! Dispatcher factory.
			std::function< dispatcher_unique_ptr_t() > disp_factory );
		/*!
		 * \}
		 */

		/*!
		 * \name Methods for working with cooperations.
		 * \{
		 */

		//! Create a cooperation.
		/*!
		 * \return A new cooperation with \a name. This cooperation
		 * will use default dispatcher binders.
		 */
		agent_coop_unique_ptr_t
		create_coop(
			//! A new cooperation name.
			const nonempty_name_t & name );

		//! Create a cooperation with automatically generated name.
		/*!
		 * \since v.5.5.1
		 * \return A new cooperation with automatically generated name. This
		 * cooperation will use default dispatcher binders.
		 */
		agent_coop_unique_ptr_t
		create_coop(
			autoname_indicator_t indicator() );

		//! Create a cooperation.
		/*!
		 * A binder \a disp_binder will be used for binding cooperation
		 * agents to the dispatcher. This binder will be default binder for
		 * this cooperation.
		 *
			\code
			so_5::rt::agent_coop_unique_ptr_t coop = so_env.create_coop(
				so_5::rt::nonempty_name_t( "some_coop" ),
				so_5::disp::active_group::create_disp_binder(
					"active_group",
					"some_active_group" ) );

			// That agent will be bound to the dispatcher "active_group"
			// and will be member of an active group with name
			// "some_active_group".
			coop->add_agent(
				so_5::rt::agent_ref_t( new a_some_agent_t( env ) ) );
			\endcode
		 */
		agent_coop_unique_ptr_t
		create_coop(
			//! A new cooperation name.
			const nonempty_name_t & name,
			//! A default binder for this cooperation.
			disp_binder_unique_ptr_t disp_binder );

		//! Create a cooperation with automatically generated name.
		/*!
		 * \since v.5.5.1
		 * \return A cooperation with automatically generated name and
		 * \a disp_binder as the default dispatcher binder.
		 */
		agent_coop_unique_ptr_t
		create_coop(
			//! A new cooperation name.
			autoname_indicator_t indicator(),
			//! A default binder for this cooperation.
			disp_binder_unique_ptr_t disp_binder );

		//! Register a cooperation.
		/*!
		 * The cooperation registration includes following steps:
		 *
		 * - binding agents to the cooperation object;
		 * - checking uniques of the cooperation name. The cooperation will 
		 *   not be registered if its name isn't unique;
		 * - agent_t::so_define_agent() will be called for each agent
		 *   in the cooperation;
		 * - binding of each agent to the dispatcher.
		 *
		 * If all these actions are successful then the cooperation is
		 * marked as registered.
		 */
		void
		register_coop(
			//! Cooperation to be registered.
			agent_coop_unique_ptr_t agent_coop );

		/*!
		 * \since v.5.2.1
		 * \brief Register single agent as a cooperation.
		 *
		 * It is just a helper methods for convience.
		 *
		 * Usage sample:
		 * \code
		   std::unique_ptr< my_agent > a( new my_agent(...) );
		   so_env.register_agent_as_coop( "sample_coop", std::move(a) );
		 * \endcode
		 */
		template< class A >
		void
		register_agent_as_coop(
			const nonempty_name_t & coop_name,
			std::unique_ptr< A > agent )
		{
			auto coop = create_coop( coop_name );
			coop->add_agent( std::move( agent ) );
			register_coop( std::move( coop ) );
		}

		/*!
		 * \since v.5.5.1.
		 * \brief Register single agent as a cooperation with automatically
		 * generated name.
		 *
		 * It is just a helper methods for convience.
		 *
		 * Usage sample:
		 * \code
		   std::unique_ptr< my_agent > a( new my_agent(...) );
		   so_env.register_agent_as_coop( so_5::autoname, std::move(a) );
		 * \endcode
		 */
		template< class A >
		void
		register_agent_as_coop(
			autoname_indicator_t indicator(),
			std::unique_ptr< A > agent )
		{
			auto coop = create_coop( indicator );
			coop->add_agent( std::move( agent ) );
			register_coop( std::move( coop ) );
		}

		/*!
		 * \since v.5.2.1
		 * \brief Register single agent as a cooperation.
		 *
		 * It is just a helper methods for convience.
		 *
		 * Usage sample:
		 * \code
		   so_env.register_agent_as_coop(
		   	"sample_coop",
		   	new my_agent_t(...) );
		 * \endcode
		 */
		inline void
		register_agent_as_coop(
			const nonempty_name_t & coop_name,
			agent_t * agent )
		{
			register_agent_as_coop(
					coop_name,
					std::unique_ptr< agent_t >( agent ) );
		}

		/*!
		 * \since v.5.5.1
		 * \brief Register single agent as a cooperation with automatically
		 * generated name.
		 *
		 * It is just a helper methods for convience.
		 *
		 * Usage sample:
		 * \code
		   so_env.register_agent_as_coop( so_5::autoname, new my_agent_t(...) );
		 * \endcode
		 */
		inline void
		register_agent_as_coop(
			autoname_indicator_t indicator(),
			agent_t * agent )
		{
			register_agent_as_coop(
					indicator,
					std::unique_ptr< agent_t >( agent ) );
		}

		/*!
		 * \since v.5.2.1
		 * \brief Register single agent as a cooperation with specified
		 * dispatcher binder.
		 *
		 * It is just a helper methods for convience.
		 *
		 * Usage sample:
		 * \code
		   std::unique_ptr< my_agent > a( new my_agent(...) );
		   so_env.register_agent_as_coop(
		   		"sample_coop",
		   		std::move(a),
		   		so_5::disp::active_group::create_disp_binder(
		   				"active_group",
		   				"some_active_group" ) );
		 * \endcode
		 */
		template< class A >
		void
		register_agent_as_coop(
			const nonempty_name_t & coop_name,
			std::unique_ptr< A > agent,
			disp_binder_unique_ptr_t disp_binder )
		{
			auto coop = create_coop( coop_name, std::move( disp_binder ) );
			coop->add_agent( std::move( agent ) );
			register_coop( std::move( coop ) );
		}

		/*!
		 * \since v.5.5.1
		 * \brief Register single agent as a cooperation with specified
		 * dispatcher binder and automatically generated name.
		 *
		 * It is just a helper methods for convience.
		 *
		 * Usage sample:
		 * \code
		   std::unique_ptr< my_agent > a( new my_agent(...) );
		   so_env.register_agent_as_coop(
					so_5::autoname,
		   		std::move(a),
		   		so_5::disp::active_group::create_disp_binder(
		   				"active_group",
		   				"some_active_group" ) );
		 * \endcode
		 */
		template< class A >
		void
		register_agent_as_coop(
			autoname_indicator_t indicator(),
			std::unique_ptr< A > agent,
			disp_binder_unique_ptr_t disp_binder )
		{
			auto coop = create_coop( indicator, std::move( disp_binder ) );
			coop->add_agent( std::move( agent ) );
			register_coop( std::move( coop ) );
		}

		/*!
		 * \since v.5.2.1
		 * \brief Register single agent as a cooperation with specified
		 * dispatcher binder.
		 *
		 * It is just a helper methods for convience.
		 *
		 * Usage sample:
		 * \code
		   so_env.register_agent_as_coop(
		   	"sample_coop",
		   	new my_agent_t(...),
		   	so_5::disp::active_group::create_disp_binder(
		   			"active_group",
		   			"some_active_group" ) );
		 * \endcode
		 */
		inline void
		register_agent_as_coop(
			const nonempty_name_t & coop_name,
			agent_t * agent,
			disp_binder_unique_ptr_t disp_binder )
		{
			register_agent_as_coop(
					coop_name,
					std::unique_ptr< agent_t >( agent ),
					std::move( disp_binder ) );
		}

		/*!
		 * \since v.5.5.1
		 * \brief Register single agent as a cooperation with specified
		 * dispatcher binder and automatically generated name.
		 *
		 * It is just a helper methods for convience.
		 *
		 * Usage sample:
		 * \code
		   so_env.register_agent_as_coop(
				so_5::autoname,
		   	new my_agent_t(...),
		   	so_5::disp::active_group::create_disp_binder(
		   			"active_group",
		   			"some_active_group" ) );
		 * \endcode
		 */
		inline void
		register_agent_as_coop(
			autoname_indicator_t indicator(),
			agent_t * agent,
			disp_binder_unique_ptr_t disp_binder )
		{
			register_agent_as_coop(
					indicator,
					std::unique_ptr< agent_t >( agent ),
					std::move( disp_binder ) );
		}

		//! Deregister the cooperation.
		/*!
		 * Method searches the cooperation within registered cooperations and if
		 * it is found deregisters it.
		 *
		 * Deregistration can take some time.
		 *
		 * At first a special signal is sent to cooperation agents.
		 * By receiving these signal agents stop receiving new messages.
		 * When the local event queue for an agent becomes empty the 
		 * agent informs the cooperation about this. When the cooperation 
		 * receives all these signals from agents it informs 
		 * the SObjectizer Run-Time.
		 * Only after this the cooperation is deregistered on the special thread
		 * context.
		 *
		 * After the cooperation deregistration agents are unbound from
		 * dispatchers. And name of the cooperation is removed from
		 * the list of registered cooperations.
		 */
		void
		deregister_coop(
			//! Name of the cooperation to be registered.
			const nonempty_name_t & name,
			//! Deregistration reason.
			int reason );
		/*!
		 * \}
		 */

		/*!
		 * \name Methods for working with timer events.
		 * \{
		 */

		//! Schedule timer event.
		/*!
		 * \since v.5.5.0
		 */
		template< class MESSAGE >
		so_5::timer_id_t
		schedule_timer(
			//! Message to be sent after timeout.
			std::unique_ptr< MESSAGE > msg,
			//! Mbox to which message will be delivered.
			const mbox_t & mbox,
			//! Timeout before the first delivery.
			std::chrono::steady_clock::duration pause,
			//! Period of the delivery repetition for periodic messages.
			/*! 
				\note Value 0 indicates that it's not periodic message 
					(will be delivered one time).
			*/
			std::chrono::steady_clock::duration period )
		{
			ensure_message_with_actual_data( msg.get() );

			return schedule_timer(
				std::type_index( typeid( MESSAGE ) ),
				message_ref_t( msg.release() ),
				mbox,
				pause,
				period );
		}

		//! Schedule timer event.
		/*!
		 * \deprecated Obsolete in v.5.5.0. Use versions with
		 * std::chrono::steady_clock::duration parameters.
		 */
		template< class MESSAGE >
		so_5::timer_id_t
		schedule_timer(
			//! Message to be sent after timeout.
			std::unique_ptr< MESSAGE > msg,
			//! Mbox to which message will be delivered.
			const mbox_t & mbox,
			//! Timeout before the first delivery.
			unsigned int delay_msec,
			//! Period of the delivery repetition for periodic messages.
			/*! 
				\note Value 0 indicates that it's not periodic message 
					(will be delivered one time).
			*/
			unsigned int period_msec )
		{
			ensure_message_with_actual_data( msg.get() );

			return schedule_timer(
				std::type_index( typeid( MESSAGE ) ),
				message_ref_t( msg.release() ),
				mbox,
				std::chrono::milliseconds( delay_msec ),
				std::chrono::milliseconds( period_msec ) );
		}

		//! Schedule a timer event for a signal.
		/*!
		 * \since v.5.5.0
		 */
		template< class MESSAGE >
		so_5::timer_id_t
		schedule_timer(
			//! Mbox to which signal will be delivered.
			const mbox_t & mbox,
			//! Timeout before the first delivery.
			std::chrono::steady_clock::duration pause,
			//! Period of the delivery repetition for periodic messages.
			/*! 
				\note Value 0 indicates that it's not periodic message 
					(will be delivered one time).
			*/
			std::chrono::steady_clock::duration period )
		{
			ensure_signal< MESSAGE >();

			return schedule_timer(
				std::type_index( typeid( MESSAGE ) ),
				message_ref_t(),
				mbox,
				pause,
				period );
		}

		//! Schedule a timer event for a signal.
		/*!
		 * \deprecated Obsolete in v.5.5.0. Use versions with
		 * std::chrono::steady_clock::duration parameters.
		 */
		template< class MESSAGE >
		so_5::timer_id_t
		schedule_timer(
			//! Mbox to which signal will be delivered.
			const mbox_t & mbox,
			//! Timeout before the first delivery.
			unsigned int delay_msec,
			//! Period of the delivery repetition for periodic messages.
			/*! 
				\note Value 0 indicates that it's not periodic message 
					(will be delivered one time).
			*/
			unsigned int period_msec )
		{
			ensure_signal< MESSAGE >();

			return schedule_timer(
				std::type_index( typeid( MESSAGE ) ),
				message_ref_t(),
				mbox,
				std::chrono::milliseconds( delay_msec ),
				std::chrono::milliseconds( period_msec ) );
		}

		//! Schedule a single shot timer event.
		/*!
		 * \since v.5.5.0
		 */
		template< class MESSAGE >
		void
		single_timer(
			//! Message to be sent after timeout.
			std::unique_ptr< MESSAGE > msg,
			//! Mbox to which message will be delivered.
			const mbox_t & mbox,
			//! Timeout before delivery.
			std::chrono::steady_clock::duration pause )
		{
			ensure_message_with_actual_data( msg.get() );

			single_timer(
				std::type_index( typeid( MESSAGE ) ),
				message_ref_t( msg.release() ),
				mbox,
				pause );
		}

		//! Schedule a single shot timer event.
		/*!
		 * \deprecated Obsolete in v.5.5.0. Use versions with
		 * std::chrono::steady_clock::duration parameters.
		 */
		template< class MESSAGE >
		void
		single_timer(
			//! Message to be sent after timeout.
			std::unique_ptr< MESSAGE > msg,
			//! Mbox to which message will be delivered.
			const mbox_t & mbox,
			//! Timeout before delivery.
			unsigned int delay_msec )
		{
			ensure_message_with_actual_data( msg.get() );

			single_timer(
				std::type_index( typeid( MESSAGE ) ),
				message_ref_t( msg.release() ),
				mbox,
				std::chrono::milliseconds( delay_msec ) );
		}

		//! Schedule a single shot timer event for a signal.
		/*!
		 * \since v.5.5.0
		 */
		template< class MESSAGE >
		void
		single_timer(
			//! Mbox to which signal will be delivered.
			const mbox_t & mbox,
			//! Timeout before delivery.
			std::chrono::steady_clock::duration pause )
		{
			ensure_signal< MESSAGE >();

			single_timer(
				std::type_index( typeid( MESSAGE ) ),
				message_ref_t(),
				mbox,
				pause );
		}

		//! Schedule a single shot timer event for a signal.
		/*!
		 * \deprecated Obsolete in v.5.5.0. Use versions with
		 * std::chrono::steady_clock::duration parameters.
		 */
		template< class MESSAGE >
		void
		single_timer(
			//! Mbox to which signal will be delivered.
			const mbox_t & mbox,
			//! Timeout before delivery.
			unsigned int delay_msec )
		{
			ensure_signal< MESSAGE >();

			single_timer(
				std::type_index( typeid( MESSAGE ) ),
				message_ref_t(),
				mbox,
				std::chrono::milliseconds( delay_msec ) );
		}
		/*!
		 * \}
		 */

		/*!
		 * \name Methods for working with layers.
		 * \{
		 */

		//! Get access to the layer without raising exception if layer
		//! is not found.
		template< class SO_LAYER >
		SO_LAYER *
		query_layer_noexcept() const
		{
			static_assert( std::is_base_of< so_layer_t, SO_LAYER >::value,
					"SO_LAYER must be derived from so_layer_t class" );

			return dynamic_cast< SO_LAYER * >(
					query_layer( std::type_index( typeid( SO_LAYER ) ) ) );
		}

		//! Get access to the layer with exception if layer is not found.
		template< class SO_LAYER >
		SO_LAYER *
		query_layer() const
		{
			auto layer = query_layer_noexcept< SO_LAYER >();

			if( !layer )
				SO_5_THROW_EXCEPTION(
					rc_layer_does_not_exist,
					"layer does not exist" );

			return dynamic_cast< SO_LAYER * >( layer );
		}

		//! Add an additional layer.
		template< class SO_LAYER >
		void
		add_extra_layer(
			std::unique_ptr< SO_LAYER > layer_ptr )
		{
			add_extra_layer(
				std::type_index( typeid( SO_LAYER ) ),
				so_layer_ref_t( layer_ptr.release() ) );
		}

		/*!
		 * \since v.5.2.0.4
		 * \brief Add an additional layer via raw pointer.
		 */
		template< class SO_LAYER >
		void
		add_extra_layer(
			SO_LAYER * layer_raw_ptr )
		{
			add_extra_layer( std::unique_ptr< SO_LAYER >( layer_raw_ptr ) );
		}
		/*!
		 * \}
		 */

		/*!
		 * \name Methods for starting, initializing and stopping of the Run-Time.
		 * \{
		 */

		//! Run the SObjectizer Run-Time.
		void
		run();

		//! Initialization hook.
		/*!
		 * \attention A hang inside of this method will prevent the Run-Time
		 * from stopping. For example if a dialog with an application user
		 * is performed inside init() then SObjectizer cannot finish
		 * its work until this dialog is finished.
		 */
		virtual void
		init() = 0;

		//! Send a shutdown signal to the Run-Time.
		void
		stop();
		/*!
		 * \}
		 */

		/*!
		 * \since v.5.2.3.
		 * \brief Call event exception logger for logging an exception.
		 */
		void
		call_exception_logger(
			//! Exception caught.
			const std::exception & event_exception,
			//! A cooperation to which agent is belong.
			const std::string & coop_name );

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

		/*!
		 * \since v.5.5.4
		 * \brief Helper method for simplification of agents creation.
		 *
		 * \note Creates an instance of agent of type \a AGENT by using
		 * environment_t::make_agent() template function and adds it to
		 * the cooperation. Uses the fact that most agent types use reference
		 * to the environment object as the first argument.
		 *
		 * \return unique pointer to the new agent.
		 *
		 * \tparam AGENT type of agent to be created.
		 * \tparam ARGS type of parameters list for agent constructor.
		 *
		 * \par Usage sample:
		 \code
		 so_5::rt::environment_t & env = ...;
		 // For the case of constructor like my_agent(environmen_t&).
		 auto a1 = env.make_agent< my_agent >(); 
		 // For the case of constructor like your_agent(environment_t&, std::string).
		 auto a2 = env.make_agent< your_agent >( "hello" );
		 // For the case of constructor like their_agent(environment_t&, std::string, mbox_t).
		 auto a3 = env.make_agent< their_agent >( "bye", a2->so_direct_mbox() );
		 \endcode
		 */
		template< class AGENT, typename... ARGS >
		std::unique_ptr< AGENT >
		make_agent( ARGS &&... args )
		{
			return std::unique_ptr< AGENT >(
					new AGENT( *this, std::forward<ARGS>(args)... ) );
		}

		/*!
		 * \since v.5.5.4
		 * \brief Access to controller of run-time monitoring.
		 */
		stats::controller_t &
		stats_controller();

		/*!
		 * \since v.5.5.4
		 * \brief Access to repository of data sources for run-time monitoring.
		 */
		stats::repository_t &
		stats_repository();

		/*!
		 * \since v.5.5.5
		 * \brief Helper method for simplification of cooperation creation
		 * and registration.
		 *
		 * \par Usage samples:
			\code
			// For the case when name for new coop will be generated automatically.
			// And default dispatcher will be used for binding.
			env.introduce_coop( []( so_5::rt::agent_coop_t & coop ) {
				coop->make_agent< first_agent >(...);
				coop->make_agent< second_agent >(...);
			});

			// For the case when name is specified.
			// Default dispatcher will be used for binding.
			env.introduce_coop( "main-coop", []( so_5::rt::agent_coop_t & coop ) {
				coop->make_agent< first_agent >(...);
				coop->make_agent< second_agent >(...);
			});

			// For the case when name is automatically generated and
			// dispatcher binder is specified.
			env.introduce_coop(
				so_5::disp::active_obj::create_private_disp( env )->binder(),
				[]( so_5::rt::agent_coop_t & coop ) {
					coop->make_agent< first_agent >(...);
					coop->make_agent< second_agent >(...);
				} );

			// For the case when name is explicitly defined and
			// dispatcher binder is specified.
			env.introduce_coop(
				"main-coop",
				so_5::disp::active_obj::create_private_disp( env )->binder(),
				[]( so_5::rt::agent_coop_t & coop ) {
					coop->make_agent< first_agent >(...);
					coop->make_agent< second_agent >(...);
				} );
			\endcode
		 */
		template< typename... ARGS >
		void
		introduce_coop( ARGS &&... args );

		/*!
		 * \name Methods for internal use inside SObjectizer.
		 * \{
		 */
		//! Create multi-producer/single-consumer mbox.
		mbox_t
		so5__create_mpsc_mbox(
			//! The only consumer for the messages.
			agent_t * single_consumer,
			//! Pointer to the optional message limits storage.
			//! If this pointer is null then the limitless MPSC-mbox will be
			//! created. If this pointer is not null the the MPSC-mbox with limit
			//! control will be created.
			const so_5::rt::message_limit::impl::info_storage_t * limits_storage,
			//! Event queue proxy for the consumer.
			event_queue_proxy_ref_t event_queue );

		//! Notification about readiness to the deregistration.
		void
		so5__ready_to_deregister_notify(
			//! Cooperation which is ready to be deregistered.
			agent_coop_t * coop );

		//! Do the final actions of a cooperation deregistration.
		void
		so5__final_deregister_coop(
			//! Cooperation name to be deregistered.
			const std::string & coop_name );
		/*!
		 * \}
		 */

	private:
		//! Schedule timer event.
		so_5::timer_id_t
		schedule_timer(
			//! Message type.
			const std::type_index & type_wrapper,
			//! Message to be sent after timeout.
			const message_ref_t & msg,
			//! Mbox to which message will be delivered.
			const mbox_t & mbox,
			//! Timeout before the first delivery.
			std::chrono::steady_clock::duration pause,
			//! Period of the delivery repetition for periodic messages.
			/*! 
				\note Value 0 indicates that it's not periodic message 
					(will be delivered one time).
			*/
			std::chrono::steady_clock::duration period );

		//! Schedule a single shot timer event.
		void
		single_timer(
			//! Message type.
			const std::type_index & type_wrapper,
			//! Message to be sent after timeout.
			const message_ref_t & msg,
			//! Mbox to which message will be delivered.
			const mbox_t & mbox,
			//! Timeout before the first delivery.
			std::chrono::steady_clock::duration pause );

		//! Access to an additional layer.
		so_layer_t *
		query_layer(
			const std::type_index & type ) const;

		//! Add an additional layer.
		void
		add_extra_layer(
			const std::type_index & type,
			const so_layer_ref_t & layer );

		//! Remove an additional layer.
		void
		remove_extra_layer(
			const std::type_index & type );

		struct internals_t;

		//! SObjectizer Environment internals.
		std::unique_ptr< internals_t > m_impl;

		/*!
		 * \name Implementation details related to run/stop functionality.
		 * \{
		 */
		/*!
		 * \since v.5.5.4
		 * \brief Run controller for run-time monitoring
		 * and call next run stage.
		 */
		void
		impl__run_stats_controller_and_go_further();

		/*!
		 * \brief Run layers and call next run stage.
		 */
		void
		impl__run_layers_and_go_further();

		/*!
		 * \brief Run dispatchers and call next run stage.
		 */
		void
		impl__run_dispatcher_and_go_further();

		/*!
		 * \brief Run timer and call next run stage.
		 */
		void
		impl__run_timer_and_go_further();

		/*!
		 * \brief Run agent core and call next run stage.
		 */
		void
		impl__run_agent_core_and_go_further();

		/*!
		 * \brief Run customer's initialization routine and wait for
		 * start of deregistration procedure.
		 */
		void
		impl__run_user_supplied_init_and_wait_for_stop();

		/*!
		 * \brief Templated implementation of one run stage.
		 */
		void
		impl__do_run_stage(
			//! Short description of stage.
			const std::string & stage_name,
			//! Stage initialization code.
			std::function< void() > init_fn,
			//! Stage deinitialization code.
			std::function< void() > deinit_fn,
			//! Next stage method.
			std::function< void() > next_stage );
		/*!
		 * \}
		 */
};

//
// so_environment_t
//
/*!
 * \brief Old name for compatibility with previous versions.
 * \deprecated Obsolete in 5.5.0
 */
typedef environment_t so_environment_t;

namespace details
{

/*!
 * \since v.5.5.5
 * \brief Helper class for building and registering new cooperation.
 */
class introduce_coop_helper_t
{
public :
	//! Constructor for the case of creation a cooperation without parent.
	introduce_coop_helper_t( environment_t & env )
		:	m_env( env )
		,	m_parent_coop_name( nullptr )
	{}
	//! Constructor for the case of creation of child cooperation.
	introduce_coop_helper_t(
		environment_t & env,
		const std::string & parent_coop_name )
		:	m_env( env )
		,	m_parent_coop_name( &parent_coop_name )
	{}

	/*!
	 * For the case:
	 * - name autogenerated;
	 * - default dispatcher is used.
	 */
	template< typename L >
	void
	introduce( L && lambda )
	{
		build_and_register_coop(
				so_5::autoname,
				create_default_disp_binder(),
				std::forward< L >( lambda ) );
	}

	/*!
	 * For the case:
	 * - name autogenerated;
	 * - default dispatcher is used.
	 */
	template< typename L >
	void
	introduce( autoname_indicator_t(), L && lambda )
	{
		build_and_register_coop(
				so_5::autoname,
				create_default_disp_binder(),
				std::forward< L >( lambda ) );
	}

	/*!
	 * For the case:
	 * - name autogenerated;
	 * - dispatcher builder is specified.
	 */
	template< typename L >
	void
	introduce( disp_binder_unique_ptr_t binder, L && lambda )
	{
		build_and_register_coop(
				so_5::autoname,
				std::move( binder ),
				std::forward< L >( lambda ) );
	}

	/*!
	 * For the case:
	 * - name autogenerated;
	 * - dispatcher builder is specified.
	 */
	template< typename L >
	void
	introduce(
		autoname_indicator_t(),
		disp_binder_unique_ptr_t binder,
		L && lambda )
	{
		build_and_register_coop(
				so_5::autoname,
				std::move( binder ),
				std::forward< L >( lambda ) );
	}

	/*!
	 * For the case:
	 * - name explicitely specified;
	 * - default dispatcher is used.
	 */
	template< typename L >
	void
	introduce( const std::string & name, L && lambda )
	{
		build_and_register_coop(
				name,
				create_default_disp_binder(),
				std::forward< L >( lambda ) );
	}

	/*!
	 * For the case:
	 * - name explicitely specified;
	 * - dispatcher builder is specified.
	 */
	template< typename L >
	void
	introduce(
		const std::string & name,
		disp_binder_unique_ptr_t binder,
		L && lambda )
	{
		build_and_register_coop(
				name,
				std::move( binder ),
				std::forward< L >( lambda ) );
	}

private :
	//! Environment for creation of cooperation.
	environment_t & m_env;
	//! Optional name of parent cooperation.
	/*!
	 * Value nullptr means that there is no parent.
	 */
	const std::string * m_parent_coop_name;

	template< typename COOP_NAME, typename LAMBDA >
	void
	build_and_register_coop(
		COOP_NAME && name,
		disp_binder_unique_ptr_t binder,
		LAMBDA && lambda )
	{
		auto coop = m_env.create_coop( name, std::move( binder ) );
		if( m_parent_coop_name )
			coop->set_parent_coop_name( *m_parent_coop_name );
		lambda( *coop );
		m_env.register_coop( std::move( coop ) );
	}
};

} /* namespace details */

template< typename... ARGS >
void
environment_t::introduce_coop( ARGS &&... args )
{
	details::introduce_coop_helper_t helper{ *this };
	helper.introduce( std::forward< ARGS >( args )... );
}

/*!
 * \since v.5.5.3
 * \brief A simple way for creating child cooperation.
 *
 * \par Usage sample
	\code
	class owner : public so_5::rt::agent_t
	{
	public :
		...
		virtual void
		so_evt_start() override
		{
			auto child = so_5::rt::create_child_coop( *this, so_5::autoname );
			child->add_agent( new worker( so_environment() ) );
			...
			so_environment().register_coop( std::move( child ) );
		}
	};
	\endcode
 */
template< typename... ARGS >
agent_coop_unique_ptr_t
create_child_coop(
	//! Owner of the cooperation.
	agent_t & owner,
	//! Arguments for the environment_t::create_coop() method.
	ARGS&&... args )
{
	auto coop = owner.so_environment().create_coop(
			std::forward< ARGS >(args)... );
	coop->set_parent_coop_name( owner.so_coop_name() );

	return coop;
}

/*!
 * \since v.5.5.5
 * \brief A simple way for creating and registering child cooperation.
 *
 * \par Usage sample
	\code
	class owner : public so_5::rt::agent_t
	{
	public :
		...
		virtual void
		so_evt_start() override
		{
			so_5::rt::build_child_coop( *this, []( so_5::rt::agent_coop_t & coop ) {
				coop.make_agent< worker >();
			} );
		}
	};
	\endcode

 * \note This function is just a tiny wrapper around
 * so_5::rt::environment_t::introduce_coop() helper method. For more
 * examples with usage of introduce_coop() please see description of
 * that method.
 */
template< typename... ARGS >
void
introduce_child_coop(
	//! Owner of the cooperation.
	agent_t & owner,
	//! Arguments for the environment_t::introduce_coop() method.
	ARGS&&... args )
{
	details::introduce_coop_helper_t{
			owner.so_environment(),
			owner.so_coop_name() }.introduce( std::forward< ARGS >(args)... );
}

} /* namespace rt */

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

