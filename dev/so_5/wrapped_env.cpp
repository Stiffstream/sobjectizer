/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.9
 *
 * \file
 * \brief Class wrapped_env and its details.
 */

#include <so_5/h/wrapped_env.hpp>

#include <thread>
#include <mutex>
#include <condition_variable>

namespace so_5 {

namespace
{

/*!
 * \since
 * v.5.5.9
 *
 * \brief Implementation of environment to be used inside wrapped_env.
 */
class actual_environment_t : public environment_t
	{
	public:
		//! Initializing constructor.
		actual_environment_t(
			//! Initialization routine.
			so_5::api::generic_simple_init_t init,
			//! SObjectizer Environment parameters.
			environment_params_t && env_params )
			:	environment_t( std::move( env_params ) )
			,	m_init( std::move(init) )
			{}

		virtual void
		init() override
			{
				{
					std::lock_guard< std::mutex > lock{ m_status_lock };
					m_status = status_t::started;
					m_status_cond.notify_all();
				}

				m_init( *this );
			}

		void
		ensure_started()
			{
				/*!
				 * \note This method is necessary because stop() can be
				 * called before run(). In that case there will be an
				 * infinite waiting on join() in wrapped_env_t.
				 */
				std::unique_lock< std::mutex > lock{ m_status_lock };
				m_status_cond.wait( lock,
					[this]{ return status_t::started == m_status; } );
			}

	private:
		//! Initialization routine.
		so_5::api::generic_simple_init_t m_init;

		//! Status of environment.
		enum class status_t
			{
				not_started,
				started
			};

		//! Status of environment.
		status_t m_status = status_t::not_started;

		//! Lock object for defending status.
		std::mutex m_status_lock;
		//! Condition for waiting on status.
		std::condition_variable m_status_cond;
	};

} /* namespace anonymous */

/*!
 * \since
 * v.5.5.9
 *
 * \brief Implementation details for wrapped_env.
 */
struct wrapped_env_t::details_t
	{
		//! Actual environment object.
		actual_environment_t m_env;

		//! Helper thread for calling run method.
		std::thread m_env_thread;

		//! Initializing constructor.
		details_t(
			so_5::api::generic_simple_init_t init_func,
			environment_params_t && params )
			:	m_env{ std::move( init_func ), std::move( params ) }
			{}

		void
		start()
			{
				m_env_thread = std::thread{ [this]{ m_env.run(); } };
				m_env.ensure_started();
			}

		void
		stop()
			{
				m_env.stop();
			}

		void
		join() { if( m_env_thread.joinable() ) m_env_thread.join(); }
	};

namespace
{

std::unique_ptr< wrapped_env_t::details_t >
make_details_object(
	so_5::api::generic_simple_init_t init_func,
	environment_params_t && params )
	{
		return std::unique_ptr< wrapped_env_t::details_t >(
				new wrapped_env_t::details_t{
						std::move( init_func ),
						std::move( params )
				} );
	}

environment_params_t
make_necessary_tuning( environment_params_t && params )
	{
		params.disable_autoshutdown();
		return std::move( params );
	}

environment_params_t
make_params_via_tuner( so_5::api::generic_simple_so_env_params_tuner_t tuner )
	{
		environment_params_t params;
		tuner( params );
		return params;
	}

} /* namespace anonymous */

wrapped_env_t::wrapped_env_t()
	:	wrapped_env_t{ []( environment_t & ) {}, environment_params_t{} }
	{}

wrapped_env_t::wrapped_env_t(
	so_5::api::generic_simple_init_t init_func )
	:	wrapped_env_t{
			std::move( init_func ),
			make_necessary_tuning( environment_params_t{} ) }
	{}

wrapped_env_t::wrapped_env_t(
	so_5::api::generic_simple_init_t init_func,
	so_5::api::generic_simple_so_env_params_tuner_t params_tuner )
	:	wrapped_env_t{
			std::move( init_func ),
			make_params_via_tuner( std::move( params_tuner ) ) }
	{}

wrapped_env_t::wrapped_env_t(
	so_5::api::generic_simple_init_t init_func,
	environment_params_t && params )
	:	m_impl{ make_details_object(
			std::move( init_func ),
			make_necessary_tuning( std::move( params ) ) ) }
	{
		m_impl->start();
	}

wrapped_env_t::wrapped_env_t(
	environment_params_t && params )
	:	wrapped_env_t{
			[]( environment_t & ) {},
			make_necessary_tuning( std::move( params ) ) }
	{}

wrapped_env_t::~wrapped_env_t()
	{
		stop_then_join();
	}

environment_t &
wrapped_env_t::environment() const
	{
		return m_impl->m_env;
	}

void
wrapped_env_t::stop()
	{
		m_impl->stop();
	}

void
wrapped_env_t::join()
	{
		m_impl->join();
	}

void
wrapped_env_t::stop_then_join()
	{
		stop();
		join();
	}

} /* namespace so_5 */


