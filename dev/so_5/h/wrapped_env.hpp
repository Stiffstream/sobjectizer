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

#include <so_5/api/h/api.hpp>

#include <so_5/h/declspec.hpp>

#include <memory>

namespace so_5 {

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

/*!
 * \since
 * v.5.5.9
 *
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
 * \par Usage examples
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
 */
class SO_5_TYPE wrapped_env_t
	{
	public :
		wrapped_env_t( const wrapped_env_t & ) = delete;
		wrapped_env_t( wrapped_env_t && ) = delete;

		//! Default constructor.
		/*!
		 * Starts environment without any initialization actions.
		 */
		wrapped_env_t();

		//! A constructor which receives only initialization function.
		/*!
		 * Default environment parameters will be used.
		 */
		wrapped_env_t(
			//! Initialization function.
			so_5::api::generic_simple_init_t init_func );

		//! A constructor which receives initialization function and
		//! a function for environment's params tuning.
		wrapped_env_t(
			//! Initialization function.
			so_5::api::generic_simple_init_t init_func,
			//! Function for environment's params tuning.
			so_5::api::generic_simple_so_env_params_tuner_t params_tuner );

		//! A constructor which receives initialization function and
		//! already prepared environment's params.
		wrapped_env_t(
			//! Initialization function.
			so_5::api::generic_simple_init_t init_func,
			//! Environment's params.
			environment_params_t && params );

		/*!
		 * \since
		 * v.5.5.13
		 *
		 * \brief A constructor which receives already prepared
		 * environment's params.
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

