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

#pragma once

#include <so_5/api.hpp>

#include <so_5/declspec.hpp>

#include <memory>

namespace so_5 {

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace wrapped_env_details
{

/*!
 * \brief Style of handling init-functor in the constructor of wrapped_env.
 *
 * \since v.5.8.2
 */
enum class init_style_t
	{
		//! The init-functor has to be handled synchronously. The constructor
		//! of the wrapped_env_t will block the caller thread until the
		//! init-functor completes it's work.
		sync,
		//! The init-functor has to be handled asynchronously. The constructor
		//! of the wrapped_env_t may complete its work before the completion
		//! of the init-functor.
		async
	};

} /* namespace wrapped_env_details */

/*!
 * \brief A wrapped environment.
 *
 * \note Starts a SObjectizer Environment in the constructor. Automatically
 * stops it in the destructor (via stop_then_join() call). An Environment
 * will be started on the context of new thread. This thread is also
 * created in the constructor of wrapped_env_t.
 *
 * \note SObjectizer Environment is started with autoshutdown disabled.
 * It means that the Environment won't be stopped when the last coop will
 * be deregistered. Autoshutdown will be disabled even if a constructor
 * with custom Environment's params will be used.
 *
 * The wrapped_env_t may handle init-function (if it's passed to the constructor)
 * in two modes:
 *
 * - asynchronous (the default). In that case the wrapped_env_t constructor
 *   may complete its work even before the start of init-function;
 * - synchronous. In that case the wrapped_env_t constructor will block
 *   the caller thread until the init-function completes.
 *
 * \par Usage examples for the default asynchronous mode
 * \code
	// Start Environment without initialization function.
	int main()
	{
		so_5::wrapped_env_t env;
		... // Some user code.
		// Add a cooperation to environment.
		env.environment().introduce_coop( []( so_5::coop_t & coop ) {
			coop.make_agent< some_agent >(...);
			...
		} );
		... // Some user code.

		return 0; // env.stop_then_join() will be called in env destructor.
	}

	// Start Environment with initialization function but with
	// default parameters.
	int main()
	{
		so_5::wrapped_env_t env{
			[]( so_5::environment_t & env ) {
				... // Some initialization stuff.
			}
		};
		... // Some user code.

		return 0; // env.stop_then_join() will be called in env destructor.
	}

	// Start Environment with initialization function and custom
	// parameters.
	so_5::environment_params_t make_params()
	{
		so_5::environment_params_t params;
		params.exception_reaction( so_5::shutdown_sobjectizer_on_exception );
		...
		return params;
	}

	int main()
	{
		so_5::wrapped_env_t env{
			[]( so_5::environment_t & env ) {
				... // Some initialization stuff.
			},
			make_params()
		};
		... // Some user code.

		return 0; // env.stop_then_join() will be called in env destructor.
	}

	// Start Environment with initialization function and custom
	// parameters tuner function.
	int main()
	{
		so_5::wrapped_env_t env{
			[]( so_5::environment_t & env ) {
				... // Some initialization stuff.
			},
			[]( so_5::environment_params_t & params ) {
				params.exception_reaction( so_5::shutdown_sobjectizer_on_exception );
				...
			}
		};
		... // Some user code.

		return 0; // env.stop_then_join() will be called in env destructor.
	}

	// Explicit stop and join.
	int main()
	{
		so_5::wrapped_env_t env{ ... };
		... // Some user code.
		// Stopping environment.
		env.stop();
		... // Some user code.
		// Waiting for finish of environment's work.
		env.join();
		... // Some user code.
	}
 * \endcode
 *
 * \attention
 * Please note that if an init function is passed to the constructor
 * of wrapped_env_t objects then it is possible that this init function
 * will work longer than lifetime of wrapped_env_t object. For example:
 * \code
 * int some_func() {
 * 	so_5::wrapped_env_t env{
 * 		[](so_5::environment_t & env) { ... } // Some long-running code inside.
 * 	};
 * 	... // Some very fast actions.
 * 	return 42; // It is possible that init-function is not finished yet.
 * }
 * \endcode
 * This can lead to some nasty surprises. For example:
 * \code
 * so_5::wrapped_env_t env{
 * 	[](so_5::environment & env) {
 * 		env.introduce_coop(...); // Creation of one coop.
 * 		env.introduce_coop(...); // Creation of another coop.
 * 		...
 * 		env.introduce_coop(...); // Creation of yet another coop.
 * 	}
 * };
 * ... // Some very fact actions.
 * env.stop(); // Several coops could not be registered yet.
 * \endcode
 * Another example that may lead to null-pointer dereference:
 * \code
 * so_5::mbox_t target; // null by default.
 * so_5::wrapped_env_t env{
 * 	[&](so_5::environment_t & env) {
 * 	env.introduce_coop([&](so_5::coop_t & coop) {
 * 			target = coop.make_agent<some_agent>(...)->so_direct_mbox();
 * 		});
 * 	}
 * };
*  so_5::send<msg_start_doing_work>(target); // target may still be null!
 * \endcode
 *
 * \attention
 * If init-function throws an exception in synchronous mode then the whole
 * application will be terminated because this exception won't be handled.
 *
 * \par Usage examples for the default synchronous mode
 * Please note that there is no overloads of the wrapped_env_t contructor
 * without the init-function parameter.
 * \code
 * // Start Environment with initialization function but with
 * // default parameters.
 * int main()
 * {
 * 	so_5::wrapped_env_t env{
 * 		so_5::wrapped_env_t::wait_init_completion,
 * 		[]( so_5::environment_t & env ) {
 * 			... // Some initialization stuff.
 * 		}
 * 	};
 * 	... // Some user code.
 *
 * 	return 0; // env.stop_then_join() will be called in env destructor.
 * }
 *
 * // Start Environment with initialization function and custom
 * // parameters.
 * so_5::environment_params_t make_params()
 * {
 * 	so_5::environment_params_t params;
 * 	params.exception_reaction( so_5::shutdown_sobjectizer_on_exception );
 * 	...
 * 	return params;
 * }
 *
 * int main()
 * {
 * 	so_5::wrapped_env_t env{
 * 		so_5::wrapped_env_t::wait_init_completion,
 * 		[]( so_5::environment_t & env ) {
 * 			... // Some initialization stuff.
 * 		},
 * 		make_params()
 * 	};
 * 	... // Some user code.
 *
 * 	return 0; // env.stop_then_join() will be called in env destructor.
 * }
 *
 * // Start Environment with initialization function and custom
 * // parameters tuner function.
 * int main()
 * {
 * 	so_5::wrapped_env_t env{
 * 		so_5::wrapped_env_t::wait_init_completion,
 * 		[]( so_5::environment_t & env ) {
 * 			... // Some initialization stuff.
 * 		},
 * 		[]( so_5::environment_params_t & params ) {
 * 			params.exception_reaction( so_5::shutdown_sobjectizer_on_exception );
 * 			...
 * 		}
 * 	};
 * 	... // Some user code.
 *
 * 	return 0; // env.stop_then_join() will be called in env destructor.
 * }
 * \endcode
 *
 * \attention
 * If the init-function throws in synchronous mode then the exception will
 * be rethrown from the constructor of the wrapped_env_t:
 * \code
 * struct my_exception final : public std::exception {...};
 * ...
 * try {
 * 	so_5::wrapped_env_t sobjectizer{
 * 		so_5::wrapped_env_t::wait_init_completion,
 * 		[](so_5::environment_t & env) {
 * 			...
 * 			if(some_condition)
 * 				// In the synchronous mode we can throw from init-function.
 * 				throw my_exception{...};
 * 		}
 * 	}
 * 	...
 * }
 * catch( const my_exception & x ) {
 * 	... // exception handling.
 * }
 * \endcode
 *
 * \since v.5.5.9
 */
class SO_5_TYPE wrapped_env_t
	{
		/*!
		 * \brief The main initializing constructor.
		 *
		 * All other constructors just delegate work to this constructor.
		 *
		 * \since v.5.8.2
		 */
		wrapped_env_t(
			//! Init-function to be called.
			so_5::generic_simple_init_t init_func,
			//! Parameters for SOEnv to be used.
			environment_params_t && params,
			//! Asynchronous or synchronous mode.
			wrapped_env_details::init_style_t init_style );

	public :
		/*!
		 * \brief Helper type to be used as indicator of synchronous mode.
		 *
		 * \since v.5.8.2.
		 */
		enum class wait_init_completion_t { sync };

		/*!
		 * \brief Special indicator that tells that synchronous mode
		 * has to be used for calling init-function.
		 *
		 * Usage example:
		 * \code
		 * so_5::wrapped_env_t sobjectizer{
		 * 		so_5::wrapped_env_t::wait_init_completion,
		 * 		...
		 * 	};
		 * ...
		 * \endcode
		 *
		 * \since v.5.8.2
		 */
		static constexpr wait_init_completion_t wait_init_completion{
				wait_init_completion_t::sync
			};

		wrapped_env_t( const wrapped_env_t & ) = delete;
		wrapped_env_t( wrapped_env_t && ) = delete;

		/*!
		 * \brief Default constructor.
		 *
		 * Starts environment without any initialization actions.
		 */
		wrapped_env_t();

		/*!
		 * \brief A constructor which receives only initialization function.
		 *
		 * Default environment parameters will be used.
		 *
		 * \attention
		 * This constructor runs \a init_func in asynchronous mode.
		 */
		wrapped_env_t(
			//! Initialization function.
			so_5::generic_simple_init_t init_func );

		/*!
		 * \brief A constructor which receives initialization function and a
		 * function for environment's params tuning.
		 *
		 * \attention
		 * This constructor runs \a init_func in asynchronous mode.
		 */
		wrapped_env_t(
			//! Initialization function.
			so_5::generic_simple_init_t init_func,
			//! Function for environment's params tuning.
			so_5::generic_simple_so_env_params_tuner_t params_tuner );

		//! A constructor which receives initialization function and
		//! already prepared environment's params.
		/*!
		 * \attention
		 * This constructor runs \a init_func in asynchronous mode.
		 */
		wrapped_env_t(
			//! Initialization function.
			so_5::generic_simple_init_t init_func,
			//! Environment's params.
			environment_params_t && params );

		/*!
		 * \brief A constructor for synchronous mode which receives only
		 * initialization function.
		 *
		 * Default environment parameters will be used.
		 *
		 * \attention
		 * This constructor runs \a init_func in synchronous mode.
		 *
		 * \note
		 * This construtor rethrows an exception thrown in \a init_func.
		 *
		 * \since v.5.8.2
		 */
		wrapped_env_t(
			//! Indicator of the synchronous mode.
			wait_init_completion_t wait_init_completion_indicator,
			//! Initialization function.
			so_5::generic_simple_init_t init_func );

		/*!
		 * \brief A constructor for synchronous mode which receives
		 * initialization function and already prepared environment's params.
		 *
		 * \attention
		 * This constructor runs \a init_func in synchronous mode.
		 *
		 * \note
		 * This construtor rethrows an exception thrown in \a init_func.
		 *
		 * \since v.5.8.2
		 */
		wrapped_env_t(
			//! Indicator of the synchronous mode.
			wait_init_completion_t wait_init_completion_indicator,
			//! Initialization function.
			so_5::generic_simple_init_t init_func,
			//! Environment's params.
			environment_params_t && params );

		/*!
		 * \brief A constructor for synchronous mode which receives
		 * initialization function and a function for environment's params
		 * tuning.
		 *
		 * \attention
		 * This constructor runs \a init_func in synchronous mode.
		 *
		 * \note
		 * This construtor rethrows an exception thrown in \a init_func.
		 *
		 * \since v.5.8.2
		 */
		wrapped_env_t(
			wait_init_completion_t wait_init_completion_indicator,
			//! Initialization function.
			so_5::generic_simple_init_t init_func,
			//! Function for environment's params tuning.
			so_5::generic_simple_so_env_params_tuner_t params_tuner );

		/*!
		 * \brief A constructor which receives already prepared
		 * environment's params.
		 *
		 * Usage example:
		 * \code
		 * #include <so_5/all.hpp>
		 *
		 * so_5::environment_params_t make_params() {
		 *     so_5::environment_params_t result;
		 *     ... // Parameters' tuning.
		 *     return result;
		 * }
		 *
		 * int main() {
		 *     so_5::wrapped_env_t sobj{ make_params() }; // SObjectizer is started on separate thread here.
		 *     ... // Some actions.
		 *     return 0; // SObjectizer will be stopped automatically here.
		 * }
		 * \endcode
		 *
		 * \since v.5.5.13
		 */
		wrapped_env_t(
			//! Environment's params.
			environment_params_t && params );

		//! Destructor.
		/*!
		 * Stops the environment and waits it.
		 */
		~wrapped_env_t();

		//! Access to wrapped environment.
		environment_t &
		environment() const;

		//! Send stop signal to environment.
		void
		stop();

		//! Wait for complete finish of environment's work.
		void
		join();

		//! Send stop signal and wait for complete finish of environment's work.
		void
		stop_then_join();

		struct details_t;

	private :
		//! Implementation details.
		std::unique_ptr< details_t > m_impl;
	};

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

} /* namespace so_5 */

