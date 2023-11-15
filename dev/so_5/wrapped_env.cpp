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

#include <so_5/wrapped_env.hpp>

#include <so_5/details/invoke_noexcept_code.hpp>

#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>

namespace so_5 {

using wrapped_env_details::init_style_t;

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
			so_5::generic_simple_init_t init,
			//! SObjectizer Environment parameters.
			environment_params_t && env_params,
			//! How initialization is performed.
			init_style_t init_style )
			:	environment_t( std::move( env_params ) )
			,	m_init( std::move(init) )
			,	m_init_style{ init_style }
			{}

		void
		init() override
			{
				// Don't expect that this code throws, but if it does
				// we can't complete our work correctly, so it's better
				// to crash the application.
				so_5::details::invoke_noexcept_code( [this]{
						{
							std::lock_guard< std::mutex > lock{ m_status_lock };
							m_status = status_t::started;
							m_status_cond.notify_all();
						}

						switch( m_init_style )
							{
							case init_style_t::async:
								call_init_functor_async_style();
							break;

							case init_style_t::sync:
								call_init_functor_sync_style();
							break;
							}

						//FIXME: document this!
						bool should_stop = false;
						{
							std::lock_guard< std::mutex > lock{ m_status_lock };
							should_stop = static_cast<bool>( m_exception_from_init_functor );

							m_status = status_t::init_functor_completed;
							m_status_cond.notify_all();
						}

						if( should_stop )
							{
								this->stop();
							}
					} );
			}

		//FIXME: extend the description of this method!
		void
		ensure_started()
			{
				/*!
				 * \note This method is necessary because stop() can be
				 * called before run(). In that case there will be an
				 * infinite waiting on join() in wrapped_env_t.
				 */
				std::unique_lock< std::mutex > lock{ m_status_lock };
				if( status_t::not_started == m_status )
					{
						m_status_cond.wait( lock,
							[this]{ return status_t::not_started != m_status; } );
					}

				//FIXME: document this!
				if( init_style_t::sync == m_init_style )
					{
						if( status_t::init_functor_completed != m_status )
							{
								m_status_cond.wait( lock, [this]{
										return status_t::init_functor_completed == m_status;
									} );
							}

						if( m_exception_from_init_functor )
							{
								std::rethrow_exception(
										std::move(m_exception_from_init_functor) );
							}
					}
			}

	private:
		//! Initialization routine.
		so_5::generic_simple_init_t m_init;

		//FIXME: document this!
		const init_style_t m_init_style;

		//! Status of environment.
		enum class status_t
			{
				//! init() is not called yet.
				not_started,
				//! init() is called and is about to call init-functor.
				started,
				//! init-functor completed its work.
				init_functor_completed
			};

		//! Status of environment.
		status_t m_status = status_t::not_started;

		//! Lock object for defending status.
		std::mutex m_status_lock;
		//! Condition for waiting on status.
		std::condition_variable m_status_cond;

		//FIXME: document this!
		std::exception_ptr m_exception_from_init_functor;

		//FIXME: document this!
		void
		call_init_functor_async_style()
			{
				m_init( *this );
			}

		//FIXME: document this!
		void
		call_init_functor_sync_style() noexcept
			{
				try
					{
						m_init( *this );
					}
				catch( ... )
					{
						// Assume that these actions won't throw.
						std::lock_guard< std::mutex > lock{ m_status_lock };
						m_exception_from_init_functor = std::current_exception();
					}
			}
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
			so_5::generic_simple_init_t init_func,
			environment_params_t && params,
			init_style_t init_style )
			:	m_env{ std::move( init_func ), std::move( params ), init_style }
			{}

		~details_t()
			{
				if( m_env_thread.joinable() )
					{
						m_env_thread.join();
					}
			}

		void
		start()
			{
				m_env_thread = std::thread{ [this]{ m_env.run(); } };

				//FIXME: document how long ensure_started can wait.
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
	so_5::generic_simple_init_t init_func,
	environment_params_t && params,
	init_style_t init_style )
	{
		return std::unique_ptr< wrapped_env_t::details_t >(
				new wrapped_env_t::details_t{
						std::move( init_func ),
						std::move( params ),
						init_style
				} );
	}

environment_params_t
make_necessary_tuning( environment_params_t && params )
	{
		params.disable_autoshutdown();
		return std::move( params );
	}

environment_params_t
make_params_via_tuner( so_5::generic_simple_so_env_params_tuner_t tuner )
	{
		environment_params_t params;
		tuner( params );
		return params;
	}

} /* namespace anonymous */

wrapped_env_t::wrapped_env_t(
	so_5::generic_simple_init_t init_func,
	environment_params_t && params,
	wrapped_env_details::init_style_t init_style )
	:	m_impl{
			make_details_object(
					std::move( init_func ),
					make_necessary_tuning( std::move( params ) ),
					init_style )
		}
	{
		m_impl->start();
	}

wrapped_env_t::wrapped_env_t()
	:	wrapped_env_t{ []( environment_t & ) {}, environment_params_t{} }
	{}

wrapped_env_t::wrapped_env_t(
	so_5::generic_simple_init_t init_func )
	:	wrapped_env_t{
			std::move( init_func ),
			environment_params_t{} }
	{}

wrapped_env_t::wrapped_env_t(
	so_5::generic_simple_init_t init_func,
	so_5::generic_simple_so_env_params_tuner_t params_tuner )
	:	wrapped_env_t{
			std::move( init_func ),
			make_params_via_tuner( std::move( params_tuner ) ) }
	{}

wrapped_env_t::wrapped_env_t(
	so_5::generic_simple_init_t init_func,
	environment_params_t && params )
	:	wrapped_env_t{
			std::move( init_func ),
			std::move( params ),
			init_style_t::async }
	{}

wrapped_env_t::wrapped_env_t(
	wait_init_completion_t wait_init_completion_indicator,
	so_5::generic_simple_init_t init_func )
	:	wrapped_env_t{
			wait_init_completion_indicator,
			std::move( init_func ),
			environment_params_t{} }
	{}

wrapped_env_t::wrapped_env_t(
	wait_init_completion_t /*wait_init_completion_indicator*/,
	so_5::generic_simple_init_t init_func,
	environment_params_t && params )
	:	wrapped_env_t{
			std::move( init_func ),
			std::move( params ),
			init_style_t::sync }
	{}

wrapped_env_t::wrapped_env_t(
	wait_init_completion_t wait_init_completion_indicator,
	so_5::generic_simple_init_t init_func,
	so_5::generic_simple_so_env_params_tuner_t params_tuner )
	:	wrapped_env_t{
			wait_init_completion_indicator,
			std::move( init_func ),
			make_params_via_tuner( std::move( params_tuner ) ) }
	{}

wrapped_env_t::wrapped_env_t(
	environment_params_t && params )
	:	wrapped_env_t{
			[]( environment_t & ) {},
			std::move( params ) }
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

