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
#include <so_5/h/msg_tracing.hpp>

#include <so_5/h/custom_mbox.hpp>

#include <so_5/h/stop_guard.hpp>

#include <so_5/rt/h/nonempty_name.hpp>
#include <so_5/rt/h/mbox.hpp>
#include <so_5/rt/h/mchain.hpp>
#include <so_5/rt/h/message.hpp>
#include <so_5/rt/h/agent_coop.hpp>
#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/disp_binder.hpp>
#include <so_5/rt/h/so_layer.hpp>
#include <so_5/rt/h/coop_listener.hpp>
#include <so_5/rt/h/event_exception_logger.hpp>
#include <so_5/rt/h/event_queue_hook.hpp>

#include <so_5/h/timers.hpp>

#include <so_5/rt/stats/h/controller.hpp>
#include <so_5/rt/stats/h/repository.hpp>

#include <so_5/rt/h/queue_locks_defaults_manager.hpp>

#include <so_5/rt/h/environment_infrastructure.hpp>

#include <so_5/disp/one_thread/h/params.hpp>


#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace so_5
{

/*!
 * \brief Special type for autoname-cooperation implementation.
 *
 * \since
 * v.5.5.1
 */
struct autoname_indicator_t {};

/*!
 * \brief Special marker for indication of automatic name generation.
 *
 * \since
 * v.5.5.1
 */
inline autoname_indicator_t
autoname() { return autoname_indicator_t(); }

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
		 * \brief Move constructor.
		 *
		 * \since
		 * v.5.2.3
		 */
		environment_params_t( environment_params_t && other );
		~environment_params_t();

		/*!
		 * \brief Move operator.
		 *
		 * \since
		 * v.5.2.3
		 */
		environment_params_t &
		operator=( environment_params_t && other );

		/*!
		 * \brief Swap operation.
		 *
		 * \since
		 * v.5.2.3
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
			nonempty_name_t name,
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
		template< class SO_Layer >
		environment_params_t &
		add_layer(
			//! A layer to be added.
			std::unique_ptr< SO_Layer > layer_ptr )
		{
			if( layer_ptr.get() )
			{
				layer_unique_ptr_t ptr( layer_ptr.release() );

				add_layer(
					std::type_index( typeid( SO_Layer ) ),
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
		template< class SO_Layer >
		environment_params_t &
		add_layer(
			//! A layer to be added.
			SO_Layer * layer_raw_ptr )
		{
			return add_layer( std::unique_ptr< SO_Layer >( layer_raw_ptr ) );
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
		 * \since
		 * v.5.3.0
		 * \brief Get exception reaction flag value.
		 */
		inline exception_reaction_t
		exception_reaction() const
		{
			return m_exception_reaction;
		}

		/*!
		 * \since
		 * v.5.3.0
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
		 * \since
		 * v.5.4.0
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
		 * \since
		 * v.5.4.0
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
		 * \since
		 * v.5.5.0
		 * \brief Set error logger for the environment.
		 */
		environment_params_t &
		error_logger( const error_logger_shptr_t & logger )
		{
			m_error_logger = logger;
			return *this;
		}

		/*!
		 * \brief Set message delivery tracer for the environment.
		 *
		 * Usage example:
		 * \code
		 * so_5::launch( [](so_5::environment_t & env) { ... },
		 * 	[](so_5::environment_params_t & params) {
		 * 		params.message_delivery_tracer( so_5::msg_tracing::std_cout_tracer() );
		 * 		...
		 * 	} );
		 * \endcode
		 *
		 * \since
		 * v.5.5.9
		 */
		environment_params_t &
		message_delivery_tracer( so_5::msg_tracing::tracer_unique_ptr_t tracer )
		{
			m_message_delivery_tracer = std::move( tracer );
			return *this;
		}

		/*!
		 * \brief Set message tracer filter for the environment.
		 *
		 * \since
		 * v.5.5.22
		 */
		environment_params_t &
		message_delivery_tracer_filter(
			so_5::msg_tracing::filter_shptr_t filter )
		{
			m_message_delivery_tracer_filter = std::move( filter );
			return *this;
		}

		/*!
		 * \since
		 * v.5.5.10
		 * \brief Set parameters for the default dispatcher.
		 *
		 * \par Usage example:
			\code
			so_5::launch( []( so_5::environment_t & env ) { ... },
				[]( so_5::environment_params_t & env_params ) {
					using namespace so_5::disp::one_thread;
					// Event queue for the default dispatcher must use mutex as lock.
					env_params.default_disp_params( disp_params_t{}.tune_queue_params(
						[]( queue_traits::queue_params_t & queue_params ) {
							queue_params.lock_factory( queue_traits::simple_lock_factory() );
						} ) );
				} );
			\endcode
		 */
		environment_params_t &
		default_disp_params( so_5::disp::one_thread::disp_params_t params )
		{
			m_default_disp_params = std::move(params);
			return *this;
		}

		/*!
		 * \since
		 * v.5.5.10
		 * \brief Get the parameters for the default dispatcher.
		 */
		const so_5::disp::one_thread::disp_params_t &
		default_disp_params() const
		{
			return m_default_disp_params;
		}

		/*!
		 * \brief Set activity tracking flag for the whole SObjectizer Environment.
		 * \since
		 * v.5.5.18
		 */
		environment_params_t &
		work_thread_activity_tracking(
			work_thread_activity_tracking_t flag )
		{
			m_work_thread_activity_tracking = flag;
			return *this;
		}

		/*!
		 * \brief Get activity tracking flag for the whole SObjectizer Environment.
		 *
		 * \since
		 * v.5.5.18
		 */
		work_thread_activity_tracking_t
		work_thread_activity_tracking() const
		{
			return m_work_thread_activity_tracking;
		}

		//! Helper for turning work thread activity tracking on.
		/*!
		 * \since
		 * v.5.5.18
		 */
		environment_params_t &
		turn_work_thread_activity_tracking_on()
			{
				return work_thread_activity_tracking(
						work_thread_activity_tracking_t::on );
			}

		//! Helper for turning work thread activity tracking off.
		/*!
		 * \since
		 * v.5.5.18
		 */
		environment_params_t &
		turn_work_thread_activity_tracking_off()
			{
				return work_thread_activity_tracking(
						work_thread_activity_tracking_t::off );
			}

		//! Set manager for queue locks defaults.
		/*!
		 * \since
		 * v.5.5.18
		 */
		environment_params_t &
		queue_locks_defaults_manager(
			queue_locks_defaults_manager_unique_ptr_t manager )
			{
				m_queue_locks_defaults_manager = std::move(manager);
				return *this;
			}

		//! Get the current environment infrastructure factory.
		/*!
		 * \since
		 * v.5.5.19
		 */
		const environment_infrastructure_factory_t &
		infrastructure_factory() const
			{
				return m_infrastructure_factory;
			}

		//! Set new environment infrastructure factory.
		/*!
		 * \since
		 * v.5.5.19
		 */
		environment_params_t &
		infrastructure_factory(
//FIXME: it is better to use something like gsl::not_null<environment_infrastructure_factory_t>!
			environment_infrastructure_factory_t factory )
			{
				m_infrastructure_factory = std::move(factory);
				return *this;
			}

		/*!
		 * \brief Set event_queue_hook object.
		 *
		 * Since v.5.5.24 it is possible to use special event_queue_hook
		 * object. If it is used it should be set for SObjectizer
		 * Environment before the Environment will be started. This method
		 * allows to specify event_queue_hook object for a new Environment
		 * object.
		 *
		 * Usage example:
		 * \code
		 * so_5::launch(
		 * 	[](so_5::environment_t & env) {...}, // Some stating actions.
		 * 	[](so_5::environment_params_t & params) {
		 * 		// Set my own event_queue hook object.
		 * 		so_5::make_event_queue_hook<my_hook>(
		 * 			// Object is created dynamically and should be
		 * 			// destroyed the normal way.
		 * 			so_5::event_queue_hook_t::default_deleter,
		 * 			arg1, arg3, arg3 // and all other arguments for my_hook's constructor.
		 * 		);
		 * 	});
		 * \endcode
		 *
		 * \note
		 * The previous event_queue_hook object (if it was set earlier)
		 * will just be dropped.
		 *
		 * \since
		 * v.5.5.24
		 */
		void
		event_queue_hook(
			event_queue_hook_unique_ptr_t hook )
			{
				m_event_queue_hook = std::move(hook);
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
		const layer_map_t &
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
		 * \brief Get message delivery tracer for the environment.
		 *
		 * \since
		 * v.5.5.9
		 */
		so_5::msg_tracing::tracer_unique_ptr_t
		so5__giveout_message_delivery_tracer()
		{
			return std::move( m_message_delivery_tracer );
		}

		/*!
		 * \brief Get message delivery tracer filter for the environment.
		 *
		 * \since
		 * v.5.5.22
		 */
		so_5::msg_tracing::filter_shptr_t
		so5__giveout_message_delivery_tracer_filter()
		{
			return std::move( m_message_delivery_tracer_filter );
		}

		//! Take out queue locks defaults manager.
		/*!
		 * \since
		 * v.5.5.18
		 */
		queue_locks_defaults_manager_unique_ptr_t
		so5__giveout_queue_locks_defaults_manager()
			{
				return std::move( m_queue_locks_defaults_manager );
			}

		//! Take out event_queue_hook object.
		/*!
		 * \since
		 * v.5.5.24
		 */
		event_queue_hook_unique_ptr_t
		so5__giveout_event_queue_hook()
			{
				return std::move( m_event_queue_hook );
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
			layer_unique_ptr_t layer_ptr );

		//! Named dispatchers.
		named_dispatcher_map_t m_named_dispatcher_map;

		//! Timer thread factory.
		so_5::timer_thread_factory_t m_timer_thread_factory;

		//! Additional layers.
		layer_map_t m_so_layers;

		//! Cooperation listener.
		coop_listener_unique_ptr_t m_coop_listener;

		//! Exception logger.
		event_exception_logger_unique_ptr_t m_event_exception_logger;

		/*!
		 * \brief Exception reaction flag for the whole SO Environment.
		 *
		 * \since
		 * v.5.3.0
		 */
		exception_reaction_t m_exception_reaction;

		/*!
		 * \brief Is autoshutdown when there is no more cooperation disabled?
		 *
		 * \see disable_autoshutdown()
		 *
		 * \since
		 * v.5.4.0
		 */
		bool m_autoshutdown_disabled;

		/*!
		 * \brief Error logger for the environment.
		 * \since
		 * v.5.5.0
		 */
		error_logger_shptr_t m_error_logger;

		/*!
		 * \brief Tracer for message delivery.
		 * \since
		 * v.5.5.9
		 */
		so_5::msg_tracing::tracer_unique_ptr_t m_message_delivery_tracer;

		/*!
		 * \brief Message delivery tracer filter to be used with environment.
		 * \since
		 * v.5.5.22
		 */
		so_5::msg_tracing::filter_shptr_t m_message_delivery_tracer_filter;

		/*!
		 * \brief Parameters for the default dispatcher.
		 * \since
		 * v.5.5.10
		 */
		so_5::disp::one_thread::disp_params_t m_default_disp_params;

		/*!
		 * \brief Work thread activity tracking for the whole Environment.
		 * \since
		 * v.5.5.18
		 */
		work_thread_activity_tracking_t m_work_thread_activity_tracking;

		/*!
		 * \brief Manager for defaults of queue locks.
		 *
		 * \since
		 * v.5.5.18
		 */
		queue_locks_defaults_manager_unique_ptr_t m_queue_locks_defaults_manager;

		/*!
		 * \brief A factory for environment infrastructure entity.
		 *
		 * \since
		 * v.5.5.19
		 */
		environment_infrastructure_factory_t m_infrastructure_factory;

		/*!
		 * \brief An event_queue_hook object.
		 *
		 * \note
		 * It can be a nullptr. It means that no event_queue_hook should
		 * be used.
		 *
		 * \since
		 * v.5.5.24
		 */
		event_queue_hook_unique_ptr_t m_event_queue_hook;
};

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
 * Mboxes are created by environment_t::create_mbox() methods.
 * All these methods return the mbox_t which is a smart reference 
 * to the mbox.
 *
 * An anonymous mbox is automatically destroyed when the last reference to it is
 * destroyed. So, to save the anonymous mbox, the mbox_ref from 
 * the create_mbox() should be stored somewhere.
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
	friend class so_5::impl::internal_env_iface_t;

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
		create_mbox();

		//! Create named mbox.
		/*!
		 * If \a mbox_name is unique then a new mbox will be created.
		 * If not the reference to existing mbox will be returned.
		 */
		mbox_t
		create_mbox(
			//! Mbox name.
			nonempty_name_t mbox_name );

		/*!
		 * \deprecated Will be removed in v.5.6.0. Use create_mbox() instead.
		 */
		inline mbox_t
		create_local_mbox()
			{
				return create_mbox();
			}

		/*!
		 * \deprecated Will be removed in v.5.6.0. Use create_mbox() instead.
		 */
		inline mbox_t
		create_local_mbox(
			nonempty_name_t mbox_name )
			{
				return create_mbox( std::move(mbox_name) );
			}
		/*!
		 * \}
		 */

		/*!
		 * \name Method for working with message chains.
		 * \{
		 */

		/*!
		 * \since
		 * v.5.5.13
		 *
		 * \brief Create message chain.
		 *
		 * \par Usage examples:
			\code
			so_5::environment_t & env = ...;
			// Create mchain with size-unlimited queue.
			auto ch1 = env.create_mchain(
				so_5::make_unlimited_mchain_params() );
			// Create mchain with size-limited queue without a timeout
			// on attempt to push another message to full mchain...
			auto ch2 = env.create_mchain(
				so_5::make_limited_without_waiting_mchain_params(
					// ...maximum size of the chain.
					100,
					// ...memory for chain will be allocated and deallocated dynamically...
					so_5::mchain_props::memory_usage_t::dynamic,
					// ...an exception will be thrown on overflow.
					so_5::mchain_props::overflow_reaction_t::throw_exception ) );
			// Create mchain with size-limited queue with a timeout for 200ms
			// on attempt to push another message to full mchain...
			auto ch3 = env.create_mchain(
				so_5::make_limited_with_waiting_mchain_params(
					// ...maximum size of the chain.
					100,
					// ...memory for chain will be preallocated...
					so_5::mchain_props::memory_usage_t::preallocated,
					// ...an oldest message from mchain will be removed on overflow...
					so_5::mchain_props::overflow_reaction_t::remove_oldest,
					// ...timeout for waiting on attempt to push a message into full mchain.
					std::chrono::milliseconds(200) ) );
			// Create size-unlimited mchain with custom notificator for
			// 'not_empty' situations.
			auto ch4 = env.create_mchain(
				so_5::make_unlimited_mchain_params().not_empty_notificator(
					[&] { some_widget.send_notify(); } ) );
			\endcode
		 */
		mchain_t
		create_mchain(
			//! Parameters for a new bag.
			const mchain_params_t & params );
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
		 * \since
		 * v.5.4.0
		 *
		 * \brief Add named dispatcher if it is not exists.
		 *
		 * \par Usage:
			\code
			so_5::environment_t & env = ...;
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
		coop_unique_ptr_t
		create_coop(
			//! A new cooperation name.
			nonempty_name_t name );

		//! Create a cooperation with automatically generated name.
		/*!
		 * \since
		 * v.5.5.1
		 *
		 * \return A new cooperation with automatically generated name. This
		 * cooperation will use default dispatcher binders.
		 */
		coop_unique_ptr_t
		create_coop(
			autoname_indicator_t indicator() );

		//! Create a cooperation.
		/*!
		 * A binder \a disp_binder will be used for binding cooperation
		 * agents to the dispatcher. This binder will be default binder for
		 * this cooperation.
		 *
			\code
			so_5::coop_unique_ptr_t coop = so_env.create_coop(
				"some_coop",
				so_5::disp::active_group::create_disp_binder(
					"active_group",
					"some_active_group" ) );

			// That agent will be bound to the dispatcher "active_group"
			// and will be member of an active group with name
			// "some_active_group".
			coop->make_agent< a_some_agent_t >();
			\endcode
		 */
		coop_unique_ptr_t
		create_coop(
			//! A new cooperation name.
			nonempty_name_t name,
			//! A default binder for this cooperation.
			disp_binder_unique_ptr_t disp_binder );

		//! Create a cooperation with automatically generated name.
		/*!
		 * \since
		 * v.5.5.1
		 *
		 * \return A cooperation with automatically generated name and
		 * \a disp_binder as the default dispatcher binder.
		 */
		coop_unique_ptr_t
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
			coop_unique_ptr_t agent_coop );

		/*!
		 * \brief Register single agent as a cooperation.
		 *
		 * \since
		 * v.5.2.1
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
			nonempty_name_t coop_name,
			std::unique_ptr< A > agent )
		{
			auto coop = create_coop( std::move(coop_name) );
			coop->add_agent( std::move( agent ) );
			register_coop( std::move( coop ) );
		}

		/*!
		 * \brief Register single agent as a cooperation with automatically
		 * generated name.
		 *
		 * \since
		 * v.5.5.1.
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
		 * \brief Register single agent as a cooperation.
		 *
		 * \since
		 * v.5.2.1
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
			nonempty_name_t coop_name,
			agent_t * agent )
		{
			register_agent_as_coop(
					std::move(coop_name),
					std::unique_ptr< agent_t >( agent ) );
		}

		/*!
		 * \brief Register single agent as a cooperation with automatically
		 * generated name.
		 *
		 * \since
		 * v.5.5.1
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
		 * \brief Register single agent as a cooperation with specified
		 * dispatcher binder.
		 *
		 * \since
		 * v.5.2.1
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
			nonempty_name_t coop_name,
			std::unique_ptr< A > agent,
			disp_binder_unique_ptr_t disp_binder )
		{
			auto coop = create_coop(
					std::move( coop_name ), std::move( disp_binder ) );
			coop->add_agent( std::move( agent ) );
			register_coop( std::move( coop ) );
		}

		/*!
		 * \brief Register single agent as a cooperation with specified
		 * dispatcher binder and automatically generated name.
		 *
		 * \since
		 * v.5.5.1
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
		 * \brief Register single agent as a cooperation with specified
		 * dispatcher binder.
		 *
		 * \since
		 * v.5.2.1
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
			nonempty_name_t coop_name,
			agent_t * agent,
			disp_binder_unique_ptr_t disp_binder )
		{
			register_agent_as_coop(
					std::move( coop_name ),
					std::unique_ptr< agent_t >( agent ),
					std::move( disp_binder ) );
		}

		/*!
		 * \brief Register single agent as a cooperation with specified
		 * dispatcher binder and automatically generated name.
		 *
		 * \since
		 * v.5.5.1
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
			nonempty_name_t name,
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
		 * \attention
		 * Values of \a pause and \a period should be non-negative.
		 *
		 * \since
		 * v.5.5.19
		 */
		template< class Message >
		so_5::timer_id_t
		schedule_timer(
			//! Message type for searching subscribers.
			std::type_index subscription_type,
			//! Message to be sent after timeout.
			std::unique_ptr< Message > msg,
			//! Message mutability.
			message_mutability_t mutability,
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
			ensure_classical_message< Message >();
			ensure_message_with_actual_data( msg.get() );
			change_message_mutability( *msg, mutability );

			return schedule_timer(
				subscription_type,
				message_ref_t( msg.release() ),
				mbox,
				pause,
				period );
		}

		//! Schedule timer event.
		/*!
		 * \attention
		 * Values of \a pause and \a period should be non-negative.
		 *
		 * \since
		 * v.5.5.0
		 */
		template< class Message >
		so_5::timer_id_t
		schedule_timer(
			//! Message to be sent after timeout.
			std::unique_ptr< Message > msg,
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
			ensure_classical_message< Message >();
			ensure_message_with_actual_data( msg.get() );

			return schedule_timer(
				message_payload_type< Message >::subscription_type_index(),
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
		template< class Message >
		so_5::timer_id_t
		schedule_timer(
			//! Message to be sent after timeout.
			std::unique_ptr< Message > msg,
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
			ensure_classical_message< Message >();
			ensure_message_with_actual_data( msg.get() );

			return schedule_timer(
				message_payload_type< Message >::subscription_type_index(),
				message_ref_t( msg.release() ),
				mbox,
				std::chrono::milliseconds( delay_msec ),
				std::chrono::milliseconds( period_msec ) );
		}

		//! Schedule a timer event for a signal.
		/*!
		 * \attention
		 * Values of \a pause and \a period should be non-negative.
		 *
		 * \since
		 * v.5.5.0
		 */
		template< class Message >
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
			ensure_signal< Message >();

			return schedule_timer(
				message_payload_type< Message >::subscription_type_index(),
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
		template< class Message >
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
			ensure_signal< Message >();

			return schedule_timer(
				message_payload_type< Message >::subscription_type_index(),
				message_ref_t(),
				mbox,
				std::chrono::milliseconds( delay_msec ),
				std::chrono::milliseconds( period_msec ) );
		}

		//! Schedule a single shot timer event.
		/*!
		 * \attention
		 * Value of \a pause should be non-negative.
		 *
		 * \since
		 * v.5.5.0
		 */
		template< class Message >
		void
		single_timer(
			//! Message to be sent after timeout.
			std::unique_ptr< Message > msg,
			//! Mbox to which message will be delivered.
			const mbox_t & mbox,
			//! Timeout before delivery.
			std::chrono::steady_clock::duration pause )
		{
			ensure_message_with_actual_data( msg.get() );

			single_timer(
				message_payload_type< Message >::subscription_type_index(),
				message_ref_t( msg.release() ),
				mbox,
				pause );
		}

		//! Schedule a single shot timer event.
		/*!
		 * Intended to be used for delaying mutable messages.
		 *
		 * \since
		 * v.5.5.19
		 */
		template< class Message >
		void
		single_timer(
			//! Type to be used for searching subscribers.
			std::type_index subscription_type,
			//! Message to be sent after timeout.
			std::unique_ptr< Message > msg,
			//! Mutability flag for that message.
			message_mutability_t mutability,
			//! Mbox to which message will be delivered.
			const mbox_t & mbox,
			//! Timeout before delivery.
			std::chrono::steady_clock::duration pause )
		{
			ensure_message_with_actual_data( msg.get() );

			change_message_mutability( *msg, mutability );

			single_timer(
				subscription_type,
				message_ref_t( msg.release() ),
				mbox,
				pause );
		}

		//! Schedule a single shot timer event.
		/*!
		 * \deprecated Obsolete in v.5.5.0. Use versions with
		 * std::chrono::steady_clock::duration parameters.
		 */
		template< class Message >
		void
		single_timer(
			//! Message to be sent after timeout.
			std::unique_ptr< Message > msg,
			//! Mbox to which message will be delivered.
			const mbox_t & mbox,
			//! Timeout before delivery.
			unsigned int delay_msec )
		{
			ensure_classical_message< Message >();
			ensure_message_with_actual_data( msg.get() );

			single_timer(
				message_payload_type< Message >::subscription_type_index(),
				message_ref_t( msg.release() ),
				mbox,
				std::chrono::milliseconds( delay_msec ) );
		}

		//! Schedule a single shot timer event for a signal.
		/*!
		 * \since
		 * v.5.5.0
		 */
		template< class Message >
		void
		single_timer(
			//! Mbox to which signal will be delivered.
			const mbox_t & mbox,
			//! Timeout before delivery.
			std::chrono::steady_clock::duration pause )
		{
			ensure_signal< Message >();

			single_timer(
				message_payload_type< Message >::subscription_type_index(),
				message_ref_t(),
				mbox,
				pause );
		}

		//! Schedule a single shot timer event for a signal.
		/*!
		 * Intended to be used with mutable_msg<signal>.
		 *
		 * \since
		 * v.5.5.0
		 */
		template< class Message >
		void
		single_timer(
			//! Type to be used for searching subscribers.
			std::type_index subscription_type,
			//! Mbox to which signal will be delivered.
			const mbox_t & mbox,
			//! Timeout before delivery.
			std::chrono::steady_clock::duration pause )
		{
			ensure_signal< Message >();

			single_timer(
				subscription_type,
				message_ref_t(),
				mbox,
				pause );
		}

		//! Schedule a single shot timer event for a signal.
		/*!
		 * \deprecated Obsolete in v.5.5.0. Use versions with
		 * std::chrono::steady_clock::duration parameters.
		 */
		template< class Message >
		void
		single_timer(
			//! Mbox to which signal will be delivered.
			const mbox_t & mbox,
			//! Timeout before delivery.
			unsigned int delay_msec )
		{
			ensure_signal< Message >();

			single_timer(
				message_payload_type< Message >::subscription_type_index(),
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
		template< class SO_Layer >
		SO_Layer *
		query_layer_noexcept() const
		{
			static_assert( std::is_base_of< layer_t, SO_Layer >::value,
					"SO_Layer must be derived from so_layer_t class" );

			return dynamic_cast< SO_Layer * >(
					query_layer( std::type_index( typeid( SO_Layer ) ) ) );
		}

		//! Get access to the layer with exception if layer is not found.
		template< class SO_Layer >
		SO_Layer *
		query_layer() const
		{
			auto layer = query_layer_noexcept< SO_Layer >();

			if( !layer )
				SO_5_THROW_EXCEPTION(
					rc_layer_does_not_exist,
					"layer does not exist" );

			return layer;
		}

		//! Add an additional layer.
		template< class SO_Layer >
		void
		add_extra_layer(
			std::unique_ptr< SO_Layer > layer_ptr )
		{
			add_extra_layer(
				std::type_index( typeid( SO_Layer ) ),
				layer_ref_t( layer_ptr.release() ) );
		}

		/*!
		 * \brief Add an additional layer via raw pointer.
		 *
		 * \since
		 * v.5.2.0.4
		 */
		template< class SO_Layer >
		void
		add_extra_layer(
			SO_Layer * layer_raw_ptr )
		{
			add_extra_layer( std::unique_ptr< SO_Layer >( layer_raw_ptr ) );
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
		 * \brief Call event exception logger for logging an exception.
		 *
		 * \since
		 * v.5.2.3.
		 */
		void
		call_exception_logger(
			//! Exception caught.
			const std::exception & event_exception,
			//! A cooperation to which agent is belong.
			const std::string & coop_name );

		/*!
		 * \brief An exception reaction for the whole SO Environment.
		 *
		 * \since
		 * v.5.3.0
		 */
		exception_reaction_t
		exception_reaction() const;

		/*!
		 * \brief Get the error_logger object.
		 *
		 * \since
		 * v.5.5.0
		 */
		error_logger_t &
		error_logger() const;

		/*!
		 * \brief Helper method for simplification of agents creation.
		 *
		 * \since
		 * v.5.5.4
		 *
		 * \note Creates an instance of agent of type \a Agent by using
		 * environment_t::make_agent() template function and adds it to
		 * the cooperation. Uses the fact that most agent types use reference
		 * to the environment object as the first argument.
		 *
		 * \return unique pointer to the new agent.
		 *
		 * \tparam Agent type of agent to be created.
		 * \tparam Args type of parameters list for agent constructor.
		 *
		 * \par Usage sample:
		 \code
		 so_5::environment_t & env = ...;
		 // For the case of constructor like my_agent(environmen_t&).
		 auto a1 = env.make_agent< my_agent >(); 
		 // For the case of constructor like your_agent(environment_t&, std::string).
		 auto a2 = env.make_agent< your_agent >( "hello" );
		 // For the case of constructor like their_agent(environment_t&, std::string, mbox_t).
		 auto a3 = env.make_agent< their_agent >( "bye", a2->so_direct_mbox() );
		 \endcode
		 */
		template< class Agent, typename... Args >
		std::unique_ptr< Agent >
		make_agent( Args &&... args )
		{
			return std::unique_ptr< Agent >(
					new Agent( *this, std::forward<Args>(args)... ) );
		}

		/*!
		 * \brief Access to controller of run-time monitoring.
		 *
		 * \since
		 * v.5.5.4
		 */
		stats::controller_t &
		stats_controller();

		/*!
		 * \brief Access to repository of data sources for run-time monitoring.
		 *
		 * \since
		 * v.5.5.4
		 */
		stats::repository_t &
		stats_repository();

		/*!
		 * \brief Helper method for simplification of cooperation creation
		 * and registration.
		 *
		 * \since
		 * v.5.5.5
		 *
		 * \par Usage samples:
			\code
			// For the case when name for new coop will be generated automatically.
			// And default dispatcher will be used for binding.
			env.introduce_coop( []( so_5::coop_t & coop ) {
				coop.make_agent< first_agent >(...);
				coop.make_agent< second_agent >(...);
			});

			// For the case when name is specified.
			// Default dispatcher will be used for binding.
			env.introduce_coop( "main-coop", []( so_5::coop_t & coop ) {
				coop.make_agent< first_agent >(...);
				coop.make_agent< second_agent >(...);
			});

			// For the case when name is automatically generated and
			// dispatcher binder is specified.
			env.introduce_coop(
				so_5::disp::active_obj::create_private_disp( env )->binder(),
				[]( so_5::coop_t & coop ) {
					coop.make_agent< first_agent >(...);
					coop.make_agent< second_agent >(...);
				} );

			// For the case when name is explicitly defined and
			// dispatcher binder is specified.
			env.introduce_coop(
				"main-coop",
				so_5::disp::active_obj::create_private_disp( env )->binder(),
				[]( so_5::coop_t & coop ) {
					coop.make_agent< first_agent >(...);
					coop.make_agent< second_agent >(...);
				} );
			\endcode
		 */
		template< typename... Args >
		void
		introduce_coop( Args &&... args );

		/*!
		 * \brief Get activity tracking flag for the whole SObjectizer Environment.
		 *
		 * \since
		 * v.5.5.18
		 */
		work_thread_activity_tracking_t
		work_thread_activity_tracking() const;

		/*!
		 * \brief Get binding to the default dispatcher.
		 *
		 * \note
		 * This method is part of environment_t for possibility to
		 * write custom implementations of environment_infrastructure_t.
		 * Because of that this method can be changed or removed in 
		 * future versions of SObjectizer.
		 *
		 * \since
		 * v.5.5.19
		 */
		disp_binder_unique_ptr_t
		so_make_default_disp_binder();

		/*!
		 * \brief Get autoshutdown_disabled flag.
		 *
		 * Autoshutdown feature is on by default. It can be turned off
		 * in environment_params_t. This methods returns <i>true</i> if
		 * autoshutdown is turned off.
		 *
		 * \since
		 * v.5.5.19
		 */
		bool
		autoshutdown_disabled() const;

		//! Schedule timer event.
		/*!
		 * \attention
		 * Values of \a pause and \a period should be non-negative.
		 *
		 * \note
		 * Before v.5.5.19 this method was private.
		 */
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
		/*!
		 * \attention
		 * Value of \a pause should be non-negative.
		 *
		 * \note
		 * Before v.5.5.19 this method was private.
		 */
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

		//! Create a custom mbox.
		/*!
		 * \tparam Lambda type of actual lambda with all creation actions.
		 * The Lambda must be lambda-function or functional objects with
		 * the following format:
		 * \code
		 * so_5::mbox_t lambda(const so_5::mbox_creation_data_t &);
		 * \endcode
		 *
		 * \since
		 * v.5.5.19.2
		 */
		template< typename Lambda >
		mbox_t
		make_custom_mbox( Lambda && lambda )
			{
				using namespace custom_mbox_details;

				creator_template_t< Lambda > creator( std::forward<Lambda>(lambda) );
				return do_make_custom_mbox( creator );
			}

		/*!
		 * \name Methods for working with stop_guards.
		 * \{
		 */
		//! Set up a new stop_guard.
		/*!
		 * Usage examples:
		 * \code
		 * // Add a stop_guard.
		 * // Note: an exception can be thrown if stop is in progress
		 * class my_stop_guard
		 * 	: public so_5::stop_guard_t
		 * 	, public std::enable_shared_from_this< my_stop_guard >
		 * {...};
		 *
		 * class my_agent : public so_5::agent_t
		 * {
		 * 	...
		 * 	void on_some_event()
		 * 	{
		 * 		// We need a stop_guard here.
		 * 		m_my_guard = std::make_shared< my_stop_guard >(...);
		 * 		so_environment().setup_stop_guard( m_my_guard );
		 * 	}
		 * private :
		 * 	so_5::stop_guard_shptr_t m_my_guard;
		 * };
		 *
		 * //
		 * // Add a stop_guard without throwing an exception if stop is in progress
		 * //
		 * class my_stop_guard
		 * 	: public so_5::stop_guard_t
		 * 	, public std::enable_shared_from_this< my_stop_guard >
		 * {...};
		 *
		 * class my_agent : public so_5::agent_t
		 * {
		 * 	...
		 * 	void on_some_event()
		 * 	{
		 * 		// We need a stop_guard here.
		 * 		m_my_guard = std::make_shared< my_stop_guard >(...);
		 * 		const auto r = so_environment().setup_stop_guard(
		 * 				m_my_guard,
		 * 				so_5::stop_guard_t::what_if_stop_in_progress_t::return_negative_result );
		 *			if( so_5::stop_guard_t::setup_result_t::stop_already_in_progress  == r )
		 *				... // handle error here.
		 * 	}
		 * private :
		 * 	so_5::stop_guard_shptr_t m_my_guard;
		 * };
		 * \endcode
		 *
		 * \note
		 * Uniqueness of stop_guard is not checked. It means that
		 * it is possible to add the same stop_guard several times.
		 * But it seems to be useless.
		 *
		 * \since
		 * v.5.5.19.2
		 */
		stop_guard_t::setup_result_t
		setup_stop_guard(
			//! Stop guard to be set.
			//! Should not be nullptr.
			stop_guard_shptr_t guard,
			//! What to do is the stop operation is already in progress?
			stop_guard_t::what_if_stop_in_progress_t reaction_on_stop_in_progress
				= stop_guard_t::what_if_stop_in_progress_t::throw_exception );

		//! Remove stop_guard and complete the stop operation if necessary.
		/*!
		 * Every stop_guard which was added to the environment must be
		 * explicitely removed from the environment. It is done by this method.
		 * If there is no more stop_guard and the stop operation is in progress
		 * then the environment will complete the stop operation.
		 *
		 * Usage examples:
		 * \code
		 * // Note: an exception can be thrown if stop is in progress
		 * class my_stop_guard
		 * 	: public so_5::stop_guard_t
		 * 	, public std::enable_shared_from_this< my_stop_guard >
		 * {...};
		 *
		 * class my_agent : public so_5::agent_t
		 * {
		 * 	...
		 * 	void on_some_event()
		 * 	{
		 * 		// We need a stop_guard here.
		 * 		m_my_guard = std::make_shared< my_stop_guard >(...);
		 * 		so_environment().setup_stop_guard( m_my_guard );
		 * 	}
		 *
		 * 	void on_work_finished_signal()
		 * 	{
		 * 		// Stop_guard must be removed now.
		 * 		so_environment().remove_stop_guard( m_my_guard );
		 * 	}
		 * private :
		 * 	so_5::stop_guard_shptr_t m_my_guard;
		 * };
		 * \endcode
		 * \since
		 * v.5.5.19.2
		 */
		void
		remove_stop_guard(
			//! Stop guard to be removed.
			stop_guard_shptr_t guard );
		/*!
		 * \}
		 */

		/*!
		 * \name Methods for working with msg_tracing's filters.
		 * \{
		 */
		/*!
		 * \brief Change the current msg_tracing's filter to a new one.
		 *
		 * Usage example:
		 * \code
		 * so_5::launch([](so_5::environment_t & env) {...},
		 * 	[](so_5::environment_params_t & params) {
		 * 		// Turn message delivery tracing on.
		 * 		params.message_delivery_tracer(
		 * 			so_5::msg_tracing::std_cout_tracer());
		 * 		// Disable all trace messages.
		 * 		// It is expected that trace filter will be changed in the future.
		 * 		params.message_delivery_tracer_filter(
		 * 			so_5::msg_tracing::make_disable_all_filter());
		 * 		...
		 * 	} );
		 * ...
		 * void some_agent_t::turn_msg_tracing_on() {
		 * 	// Remove trace filter. As result all trace messages will be printed.
		 * 	so_environment().change_message_delivery_tracer_filter(
		 * 		so_5::msg_tracing::no_filter());
		 * 	...
		 * }
		 * \endcode
		 *
		 * \note
		 * It is possible that there are active calls to
		 * so_5::msg_tracing::filter_t::filter() methods at the time of
		 * invocation of change_message_delivery_tracer_filter(). In this
		 * case all active calls will be completed with the previous
		 * filter. This could lead to mixture of messages in the trace:
		 * some of them will be enabled by old filter and some of them
		 * will be enabled by new filter. And it is possible that
		 * messages enabled by new filter will precede messages enabled
		 * by old filter.
		 *
		 * \throw exception_t if message delivery tracing is disabled.
		 *
		 * \since
		 * v.5.5.22
		 */
		void
		change_message_delivery_tracer_filter(
			//! A new filter to be used.
			//! It can be an empty pointer. In this case all trace messages
			//! will be passed to tracer object.
			so_5::msg_tracing::filter_shptr_t filter );
		/*!
		 * \}
		 */

	private:
		//! Access to an additional layer.
		layer_t *
		query_layer(
			const std::type_index & type ) const;

		//! Add an additional layer.
		void
		add_extra_layer(
			const std::type_index & type,
			const layer_ref_t & layer );

		//! Remove an additional layer.
		void
		remove_extra_layer(
			const std::type_index & type );

		//! Actual creation of a custom mbox.
		/*!
		 * \since
		 * v.5.5.19.2
		 */
		mbox_t
		do_make_custom_mbox(
			custom_mbox_details::creator_iface_t & creator );

		struct internals_t;

		//! SObjectizer Environment internals.
		std::unique_ptr< internals_t > m_impl;

		/*!
		 * \name Implementation details related to run/stop functionality.
		 * \{
		 */
		/*!
		 * \brief Run controller for run-time monitoring
		 * and call next run stage.
		 *
		 * \since
		 * v.5.5.4
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
		 * \brief Launch environment infrastructure and wait for finish.
		 *
		 * \since
		 * v.5.5.19
		 */
		void
		impl__run_infrastructure();
		/*!
		 * \}
		 */
};

namespace details
{

/*!
 * \brief Helper class for building and registering new cooperation.
 *
 * \since
 * v.5.5.5
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

	template< typename Coop_Name, typename Lambda >
	void
	build_and_register_coop(
		Coop_Name && name,
		disp_binder_unique_ptr_t binder,
		Lambda && lambda )
	{
		auto coop = m_env.create_coop( name, std::move( binder ) );
		if( m_parent_coop_name )
			coop->set_parent_coop_name( *m_parent_coop_name );
		lambda( *coop );
		m_env.register_coop( std::move( coop ) );
	}
};

} /* namespace details */

template< typename... Args >
void
environment_t::introduce_coop( Args &&... args )
{
	details::introduce_coop_helper_t helper{ *this };
	helper.introduce( std::forward< Args >( args )... );
}

/*!
 * \brief A simple way for creating child cooperation.
 *
 * \since
 * v.5.5.3
 *
 * \par Usage sample
	\code
	class owner : public so_5::agent_t
	{
	public :
		...
		virtual void
		so_evt_start() override
		{
			auto child = so_5::create_child_coop( *this, so_5::autoname );
			child->make_agent< worker >();
			...
			so_environment().register_coop( std::move( child ) );
		}
	};
	\endcode
 */
template< typename... Args >
coop_unique_ptr_t
create_child_coop(
	//! Owner of the cooperation.
	agent_t & owner,
	//! Arguments for the environment_t::create_coop() method.
	Args&&... args )
{
	auto coop = owner.so_environment().create_coop(
			std::forward< Args >(args)... );
	coop->set_parent_coop_name( owner.so_coop_name() );

	return coop;
}

/*!
 * \brief A simple way for creating child cooperation when there is
 * a reference to the parent cooperation object.
 *
 * \since
 * v.5.5.8
 *
 * \par Usage sample
	\code
	env.introduce_coop( []( so_5::coop_t & coop ) {
		coop.define_agent().on_start( [&coop] {
			auto child = so_5::create_child_coop( coop, so_5::autoname );
			child->make_agent< worker >();
			...
			coop.environment().register_coop( std::move( child ) );
		} );
		...
	} );
	\endcode
 */
template< typename... Args >
coop_unique_ptr_t
create_child_coop(
	//! Parent cooperation.
	const coop_t & parent,
	//! Arguments for the environment_t::create_coop() method.
	Args&&... args )
{
	auto coop = parent.environment().create_coop(
			std::forward< Args >(args)... );
	coop->set_parent_coop_name( parent.query_coop_name() );

	return coop;
}

/*!
 * \brief A simple way for creating and registering child cooperation.
 *
 * \since
 * v.5.5.5
 *
 * \par Usage sample
	\code
	class owner : public so_5::agent_t
	{
	public :
		...
		virtual void
		so_evt_start() override
		{
			so_5::introduce_child_coop( *this, []( so_5::coop_t & coop ) {
				coop.make_agent< worker >();
			} );
		}
	};
	\endcode

 * \note This function is just a tiny wrapper around
 * so_5::environment_t::introduce_coop() helper method. For more
 * examples with usage of introduce_coop() please see description of
 * that method.
 */
template< typename... Args >
void
introduce_child_coop(
	//! Owner of the cooperation.
	agent_t & owner,
	//! Arguments for the environment_t::introduce_coop() method.
	Args&&... args )
{
	details::introduce_coop_helper_t{
			owner.so_environment(),
			owner.so_coop_name() }.introduce( std::forward< Args >(args)... );
}

/*!
 * \brief A simple way for creating and registering child cooperation
 * when there is a reference to parent coop.
 *
 * \since
 * v.5.5.8
 *
 * \par Usage sample
	\code
	env.introduce_coop( []( so_5::coop_t & parent ) {
		coop.define_agent().on_start( [&parent] {
			so_5::introduce_child_coop( parent,
				[]( so_5::coop_t & child ) {
					child.make_agent< worker >();
					...
				} );
			...
		} );
		...
	} );
	\endcode

 * \note This function is just a tiny wrapper around
 * so_5::environment_t::introduce_coop() helper method. For more
 * examples with usage of introduce_coop() please see description of
 * that method.
 */
template< typename... Args >
void
introduce_child_coop(
	//! Parent cooperation.
	const coop_t & parent,
	//! Arguments for the environment_t::introduce_coop() method.
	Args&&... args )
{
	details::introduce_coop_helper_t{
			parent.environment(),
			parent.query_coop_name() }.introduce( std::forward< Args >(args)... );
}

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::environment_params_t
 * instead.
 */
using environment_params_t = so_5::environment_params_t;

/*!
 * \brief Old name for compatibility with previous versions.
 * \deprecated Obsolete in 5.5.0. Will be removed in v.5.6.0.
 */
typedef so_5::environment_params_t so_environment_params_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::environment_t
 * instead.
 */
using environment_t = so_5::environment_t;

/*!
 * \brief Old name for compatibility with previous versions.
 * \deprecated Obsolete in 5.5.0. Will be removed in v.5.6.0
 */
typedef so_5::environment_t so_environment_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::create_child_coop()
 * instead.
 */
template< typename... Args >
coop_unique_ptr_t
create_child_coop( Args&&... args )
{
	return so_5::create_child_coop( std::forward<Args>(args)... );
}

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::introduce_child_coop()
 * instead.
 */
template< typename... Args >
void
introduce_child_coop( Args&&... args )
{
	so_5::introduce_child_coop( std::forward<Args>(args)... );
}

} /* namespace rt */

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

