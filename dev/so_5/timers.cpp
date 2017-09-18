/*
	SObjectizer 5.
*/

/*!
	\file
	\since
	v.5.5.0

	\brief Timers and tools for working with timers.
*/

#include <so_5/details/h/abort_on_fatal_error.hpp>

#include <so_5/rt/impl/h/mbox_iface_for_timers.hpp>

#include <so_5/h/stdcpp.hpp>

#include <so_5/h/timers.hpp>

#include <timertt/all.hpp>

namespace so_5
{

//
// timer_t
//
timer_t::~timer_t()
	{}

//
// timer_id_t
//
timer_id_t::timer_id_t()
	{}

timer_id_t::timer_id_t(
	so_5::intrusive_ptr_t< timer_t > && timer )
	:	m_timer( std::move( timer ) )
	{}

timer_id_t::timer_id_t(
	const timer_id_t & o )
	:	m_timer( o.m_timer )
	{}

timer_id_t::timer_id_t(
	timer_id_t && o )
	:	m_timer( std::move( o.m_timer ) )
	{}

timer_id_t::~timer_id_t()
	{}

timer_id_t &
timer_id_t::operator=( const timer_id_t & o )
	{
		timer_id_t t( o );
		t.swap( *this );
		return *this;
	}

timer_id_t &
timer_id_t::operator=( timer_id_t && o )
	{
		timer_id_t t( std::move( o ) );
		t.swap( *this );
		return *this;
	}

void
timer_id_t::swap( timer_id_t & o )
	{
		m_timer.swap( o.m_timer );
	}

bool
timer_id_t::is_active() const
	{
		return ( m_timer && m_timer->is_active() );
	}

void
timer_id_t::release()
	{
		if( m_timer )
			m_timer->release();
	}

//
// timer_thread_t
//

timer_thread_t::timer_thread_t()
	{}

timer_thread_t::~timer_thread_t()
	{}

//
// timer_manager_t::elapsed_timers_collector_t
//
timer_manager_t::elapsed_timers_collector_t::elapsed_timers_collector_t()
	{}

timer_manager_t::elapsed_timers_collector_t::~elapsed_timers_collector_t()
	{}

//
// timer_manager_t
//

timer_manager_t::timer_manager_t()
	{}

timer_manager_t::~timer_manager_t()
	{}

namespace timers_details
{

//
// actual_timer_t
//
/*!
 * \since
 * v.5.5.0
 *
 * \brief An actual implementation of timer interface.
 *
 * \note
 * Since v.5.5.19 this template can be used with timer_thread and
 * with timer_manager.
 * 
 * \tparam TIMER A type of timertt-based thread/manager which implements timers.
 */
template< class TIMER >
class actual_timer_t : public timer_t
	{
	public :
		//! The actual type of timer holder for timertt.
		using timer_holder_t = timertt::timer_object_holder<
				typename TIMER::thread_safety >;

		//! Initialized constructor.
		actual_timer_t(
			TIMER * thread )
			:	m_thread( thread )
			,	m_timer( thread->allocate() )
			{}
		virtual ~actual_timer_t() override
			{
				release();
			}

		timer_holder_t &
		timer_holder()
			{
				return m_timer;
			}

		virtual bool
		is_active() const override
			{
				return (m_thread != nullptr);
			}

		virtual void
		release() override
			{
				if( m_thread )
				{
					m_thread->deactivate( m_timer );
					m_thread = nullptr;
					m_timer.reset();
				}
			}

	private :
		//! Timer thread for the timer.
		/*!
		 * nullptr means that timer is deactivated.
		 */
		TIMER * m_thread;

		//! Underlying timer object reference.
		timer_holder_t m_timer;
	};

//
// actual_thread_t
//
/*!
 * \since
 * v.5.5.0
 *
 * \brief An actual implementation of timer thread.
 * 
 * \tparam TIMER_THREAD A type of timertt-based thread which implements timers.
 */
template< class TIMER_THREAD >
class actual_thread_t : public timer_thread_t
	{
		typedef actual_timer_t< TIMER_THREAD > timer_demand_t;

	public :
		//! Initializing constructor.
		actual_thread_t(
			//! Real timer thread.
			std::unique_ptr< TIMER_THREAD > thread )
			:	m_thread( std::move( thread ) )
			{}

		virtual void
		start() override
			{
				m_thread->start();
			}

		virtual void
		finish() override
			{
				m_thread->shutdown_and_join();
			}

		virtual timer_id_t
		schedule(
			const std::type_index & type_index,
			const mbox_t & mbox,
			const message_ref_t & msg,
			std::chrono::steady_clock::duration pause,
			std::chrono::steady_clock::duration period ) override
			{
				auto timer = stdcpp::make_unique< timer_demand_t >( m_thread.get() );

				m_thread->activate( timer->timer_holder(),
						pause,
						period,
						[type_index, mbox, msg]()
						{
							::so_5::rt::impl::mbox_iface_for_timers_t{ mbox }
									.deliver_message_from_timer( type_index, msg );
						} );

				return timer_id_t( timer.release() );
			}

		virtual void
		schedule_anonymous(
			const std::type_index & type_index,
			const mbox_t & mbox,
			const message_ref_t & msg,
			std::chrono::steady_clock::duration pause,
			std::chrono::steady_clock::duration period ) override
			{
				m_thread->activate(
						pause,
						period,
						[type_index, mbox, msg]()
						{
							::so_5::rt::impl::mbox_iface_for_timers_t{ mbox }
									.deliver_message_from_timer( type_index, msg );
						} );
			}

		virtual timer_thread_stats_t
		query_stats() override
			{
				auto d = m_thread->get_timer_quantities();

				return timer_thread_stats_t{
						d.m_single_shot_count,
						d.m_periodic_count
					};
			}

	private :
		std::unique_ptr< TIMER_THREAD > m_thread;
	};

//
// actual_manager_t
//
/*!
 * \brief An actual implementation of timer_manager.
 * 
 * \tparam TIMER_MANAGER A type of timertt-based manager which implements timers.
 *
 * \since
 * v.5.5.19
 */
template< class TIMER_MANAGER >
class actual_manager_t : public timer_manager_t
	{
		typedef actual_timer_t< TIMER_MANAGER > timer_demand_t;

	public :
		//! Initializing constructor.
		actual_manager_t(
			//! Real timer thread.
			std::unique_ptr< TIMER_MANAGER > manager,
			//! Collector for elapsed timers.
			outliving_reference_t< timer_manager_t::elapsed_timers_collector_t >
				collector )
			:	m_manager( std::move( manager ) )
			,	m_collector( std::move( collector ) )
			{}

		virtual void
		process_expired_timers() override
			{
				m_manager->process_expired_timers();
			}

		virtual std::chrono::steady_clock::duration
		timeout_before_nearest_timer(
			std::chrono::steady_clock::duration default_timer ) override
			{
				return m_manager->timeout_before_nearest_timer( default_timer );
			}

		virtual timer_id_t
		schedule(
			const std::type_index & type_index,
			const mbox_t & mbox,
			const message_ref_t & msg,
			std::chrono::steady_clock::duration pause,
			std::chrono::steady_clock::duration period ) override
			{
				auto timer = stdcpp::make_unique< timer_demand_t >( m_manager.get() );

				m_manager->activate( timer->timer_holder(),
						pause,
						period,
						[this, type_index, mbox, msg]()
						{
							m_collector.get().accept( type_index, mbox, msg );
						} );

				return timer_id_t( timer.release() );
			}

		virtual void
		schedule_anonymous(
			const std::type_index & type_index,
			const mbox_t & mbox,
			const message_ref_t & msg,
			std::chrono::steady_clock::duration pause,
			std::chrono::steady_clock::duration period ) override
			{
				m_manager->activate(
						pause,
						period,
						[this, type_index, mbox, msg]()
						{
							m_collector.get().accept( type_index, mbox, msg );
						} );
			}

		virtual bool
		empty() override
			{
				return m_manager->empty();
			}

		virtual timer_thread_stats_t
		query_stats() override
			{
				auto d = m_manager->get_timer_quantities();

				return timer_thread_stats_t{
						d.m_single_shot_count,
						d.m_periodic_count
					};
			}

	private :
		std::unique_ptr< TIMER_MANAGER > m_manager;
		outliving_reference_t< timer_manager_t::elapsed_timers_collector_t >
				m_collector;
	};

//
// error_logger_for_timertt_t
//
/*!
 * \since
 * v.5.5.0
 *
 * \brief Type of error_logger for timertt stuff.
 */
using error_logger_for_timertt_t = std::function< void(const std::string &) >;

//
// create_error_logger_for_timertt
//

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

error_logger_for_timertt_t
create_error_logger_for_timertt( error_logger_shptr_t logger )
	{
		return [logger]( const std::string & msg ) {
			SO_5_LOG_ERROR( *logger, stream ) {
				stream << "error inside timer_thread: " << msg;
			}
		};
	}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

//
// exception_handler_for_timertt_t
//
/*!
 * \since
 * v.5.5.0
 *
 * \brief Type of actor_exception_handler for timertt stuff.
 */
using exception_handler_for_timertt_t =
	std::function< void(const std::exception &) >;

//
// create_exception_handler_for_timertt
//

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

exception_handler_for_timertt_t
create_exception_handler_for_timertt_thread( error_logger_shptr_t logger )
	{
		return [logger]( const std::exception & x ) {
			so_5::details::abort_on_fatal_error( [&] {
				SO_5_LOG_ERROR( *logger, stream ) {
					stream << "exception has been thrown and caught inside "
							"timer_thread, application will be aborted. "
							"Exception: " << x.what();
				}
			} );
		};
	}

exception_handler_for_timertt_t
create_exception_handler_for_timertt_manager( error_logger_shptr_t logger )
	{
		return [logger]( const std::exception & x ) {
			so_5::details::abort_on_fatal_error( [&] {
				SO_5_LOG_ERROR( *logger, stream ) {
					stream << "exception has been thrown and caught inside "
							"timer_manager, application will be aborted. "
							"Exception: " << x.what();
				}
			} );
		};
	}
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

/*!
 * \name Short synonyms for timertt templates.
 * \{
 */
//! timer_wheel thread type.
using timer_wheel_thread_t = timertt::timer_wheel_thread_template<
		error_logger_for_timertt_t,
		exception_handler_for_timertt_t >;

//! timer_heap thread type.
using timer_heap_thread_t = timertt::timer_heap_thread_template<
		error_logger_for_timertt_t,
		exception_handler_for_timertt_t >;

//! timer_list thread type.
using timer_list_thread_t = timertt::timer_list_thread_template<
		error_logger_for_timertt_t,
		exception_handler_for_timertt_t >;

//! timer_wheel manager type.
using timer_wheel_manager_t = timertt::timer_wheel_manager_template<
		timertt::thread_safety::unsafe,
		error_logger_for_timertt_t,
		exception_handler_for_timertt_t >;

//! timer_heap manager type.
using timer_heap_manager_t = timertt::timer_heap_manager_template<
		timertt::thread_safety::unsafe,
		error_logger_for_timertt_t,
		exception_handler_for_timertt_t >;

//! timer_list manager type.
using timer_list_manager_t = timertt::timer_list_manager_template<
		timertt::thread_safety::unsafe,
		error_logger_for_timertt_t,
		exception_handler_for_timertt_t >;
/*!
 * \}
 */

} /* namespace timers_details */

SO_5_FUNC timer_thread_unique_ptr_t
create_timer_wheel_thread(
	error_logger_shptr_t logger )
	{
		using timertt_thread_t = timers_details::timer_wheel_thread_t;

		return create_timer_wheel_thread(
				logger,
				timertt_thread_t::default_wheel_size(),
				timertt_thread_t::default_granularity() );
	}

SO_5_FUNC timer_thread_unique_ptr_t
create_timer_wheel_thread(
	error_logger_shptr_t logger,
	unsigned int wheel_size,
	std::chrono::steady_clock::duration granuality )
	{
		using timertt_thread_t = timers_details::timer_wheel_thread_t;
		using namespace timers_details;

		std::unique_ptr< timertt_thread_t > thread(
				new timertt_thread_t(
						wheel_size,
						granuality,
						create_error_logger_for_timertt( logger ),
						create_exception_handler_for_timertt_thread( logger ) ) );

		return timer_thread_unique_ptr_t(
				new actual_thread_t< timertt_thread_t >( std::move( thread ) ) );
	}

SO_5_FUNC timer_thread_unique_ptr_t
create_timer_heap_thread(
	error_logger_shptr_t logger )
	{
		using timertt_thread_t = timers_details::timer_heap_thread_t;

		return create_timer_heap_thread(
				logger,
				timertt_thread_t::default_initial_heap_capacity() );
	}

SO_5_FUNC timer_thread_unique_ptr_t
create_timer_heap_thread(
	error_logger_shptr_t logger,
	std::size_t initial_heap_capacity )
	{
		using timertt_thread_t = timers_details::timer_heap_thread_t;
		using namespace timers_details;

		std::unique_ptr< timertt_thread_t > thread(
				new timertt_thread_t(
						initial_heap_capacity,
						create_error_logger_for_timertt( logger ),
						create_exception_handler_for_timertt_thread( logger ) ) );

		return timer_thread_unique_ptr_t(
				new actual_thread_t< timertt_thread_t >( std::move( thread ) ) );
	}

SO_5_FUNC timer_thread_unique_ptr_t
create_timer_list_thread(
	error_logger_shptr_t logger )
	{
		using timertt_thread_t = timers_details::timer_list_thread_t;
		using namespace timers_details;

		std::unique_ptr< timertt_thread_t > thread(
				new timertt_thread_t(
						create_error_logger_for_timertt( logger ),
						create_exception_handler_for_timertt_thread( logger ) ) );

		return timer_thread_unique_ptr_t(
				new actual_thread_t< timertt_thread_t >( std::move( thread ) ) );
	}

SO_5_FUNC timer_manager_unique_ptr_t
create_timer_wheel_manager(
	error_logger_shptr_t logger,
	outliving_reference_t<
			timer_manager_t::elapsed_timers_collector_t > collector )
	{
		using timertt_manager_t = timers_details::timer_wheel_manager_t;

		return create_timer_wheel_manager(
				logger,
				std::move(collector),
				timertt_manager_t::default_wheel_size(),
				timertt_manager_t::default_granularity() );
	}

SO_5_FUNC timer_manager_unique_ptr_t
create_timer_wheel_manager(
	error_logger_shptr_t logger,
	outliving_reference_t<
			timer_manager_t::elapsed_timers_collector_t > collector,
	unsigned int wheel_size,
	std::chrono::steady_clock::duration granuality )
	{
		using timertt_manager_t = timers_details::timer_wheel_manager_t;
		using namespace timers_details;

		auto manager = stdcpp::make_unique< timertt_manager_t >(
				wheel_size,
				granuality,
				create_error_logger_for_timertt( logger ),
				create_exception_handler_for_timertt_manager( logger ) );

		return stdcpp::make_unique< actual_manager_t< timertt_manager_t > >(
						std::move( manager ),
						std::move( collector ) );
	}

SO_5_FUNC timer_manager_unique_ptr_t
create_timer_heap_manager(
	error_logger_shptr_t logger,
	outliving_reference_t<
			timer_manager_t::elapsed_timers_collector_t > collector )
	{
		using timertt_manager_t = timers_details::timer_heap_manager_t;

		return create_timer_heap_manager(
				logger,
				std::move(collector),
				timertt_manager_t::default_initial_heap_capacity() );
	}

SO_5_FUNC timer_manager_unique_ptr_t
create_timer_heap_manager(
	error_logger_shptr_t logger,
	outliving_reference_t<
			timer_manager_t::elapsed_timers_collector_t > collector,
	std::size_t initial_heap_capacity )
	{
		using timertt_manager_t = timers_details::timer_heap_manager_t;
		using namespace timers_details;

		auto manager = stdcpp::make_unique< timertt_manager_t >(
				initial_heap_capacity,
				create_error_logger_for_timertt( logger ),
				create_exception_handler_for_timertt_manager( logger ) );

		return stdcpp::make_unique< actual_manager_t< timertt_manager_t > >(
				std::move( manager ),
				std::move( collector ) );
	}

SO_5_FUNC timer_manager_unique_ptr_t
create_timer_list_manager(
	error_logger_shptr_t logger,
	outliving_reference_t<
			timer_manager_t::elapsed_timers_collector_t > collector )
	{
		using timertt_manager_t = timers_details::timer_list_manager_t;
		using namespace timers_details;

		auto manager = stdcpp::make_unique< timertt_manager_t >(
				create_error_logger_for_timertt( logger ),
				create_exception_handler_for_timertt_manager( logger ) );

		return stdcpp::make_unique< actual_manager_t< timertt_manager_t > >(
				std::move( manager ),
				std::move( collector ) );
	}

} /* namespace so_5 */

