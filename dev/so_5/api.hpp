/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Function for the SObjectizer starting.
*/

#pragma once

#include <so_5/declspec.hpp>
#include <so_5/compiler_features.hpp>

#include <so_5/environment.hpp>

#include <functional>

namespace so_5
{

namespace impl
{

//! Auxiliary class for the SObjectizer launching.
/*!
 * It is used as wrapper on various types of initialization routines.
 */
template< class Init >
class so_quick_environment_t : public so_5::environment_t
{
		using base_type_t = so_5::environment_t;

	public:
		so_quick_environment_t(
			//! Initialization routine.
			Init init,
			//! SObjectizer Environment parameters.
			so_5::environment_params_t && env_params )
			:
				base_type_t( std::move(env_params) ),
				m_init( std::move(init) )
		{}

		void
		init() override
		{
			m_init( *this );
		}

	private:
		//! Initialization routine.
		Init m_init;
};

} /* namespace impl */

/*!
 * \since
 * v.5.3.0
 *
 * \brief Generic type for a simple SObjectizer-initialization function.
 */
using generic_simple_init_t = std::function< void(so_5::environment_t &) >;

/*!
 * \since
 * v.5.3.0
 *
 * \brief Generic type for a simple SO Environment paramenters tuning function.
 */
using generic_simple_so_env_params_tuner_t =
		std::function< void(so_5::environment_params_t &) >;

//
// launch
//
//! Launch a SObjectizer Environment with default parameters.
/*!
Example with free function as initializer:

\code
void init( so_5::environment_t & env )
{
	env.register_agent_as_coop( "main_coop", new my_agent_t( env ) );
}

int main()
{
	so_5::launch( &init );

	return 0;
}
\endcode

Example with lambda-function as initializer:
\code
int main()
{
	so_5::launch( []( so_5::environment_t & env )
		{
			env.register_agent_as_coop( "main_coop", new my_agent_t( env ) );
		} );

	return 0;
}
\endcode

Example with object method as initializer:
\code
class application_t
{
public :
	void
	init( so_5::environment_t & env )
	{
		env.register_agent_as_coop( "main_coop", new my_agent_t( env ) );
	}
	...
};

int main()
{
	using namespace std;
	using namespace std::placeholders;

	application_t app;

	so_5::launch( bind( &application_t::init, &app, _1 ) );

	return 0;
}
\endcode

\tparam Init_Routine Type of initialization routine. It can be a pointer
to function or functional object that accepts a reference to
so_5::environment_t.

*/
template< typename Init_Routine >
void
launch(
	//! Initialization routine.
	Init_Routine && init_routine )
{
	impl::so_quick_environment_t<Init_Routine> env{
			std::forward<Init_Routine>(init_routine),
			environment_params_t()
	};

	env.run();
}

//! Launch a SObjectizer Environment with explicitely specified parameters.
/*!
Example with free functions as initializers:

\code
void init( so_5::environment_t & env )
{
	env.register_agent_as_coop(
			"main_coop",
			new my_agent_t( env ),
			so_5::disp::active_obj::create_disp_binder( "active_obj" ) );
}

void params_setter( so_5::environment_params_t & params )
{
	params.add_named_dispatcher( "active_obj",
			so_5::disp::active_obj::create_disp() );
}

int main()
{
	so_5::launch( &init, &params_setter );

	return 0;
}
\endcode

Example with lambda-functions as initializers:
\code
int main()
{
	so_5::launch(
		[]( so_5::environment_t & env )
		{
			env.register_agent_as_coop(
					"main_coop",
					new my_agent_t( env ),
					so_5::disp::active_obj::create_disp_binder( "active_obj" ) );
		},
		[]( so_5::environment_params_t & params )
		{
			params.add_named_dispatcher( "active_obj",
					so_5::disp::active_obj::create_disp() );
		} );

	return 0;
}
\endcode

\tparam Init_Routine Type of initialization routine. It can be a pointer
to function or functional object that accepts a reference to
so_5::environment_t.

\tparam Params_Tuner Type of routine for tuning environment's parameters.
It can be a pointer to function or functional object that accepts a reference
to so_5::environment_params_t.

*/
template< typename Init_Routine, typename Params_Tuner >
inline void
launch(
	//! Initialization routine.
	Init_Routine && init_routine,
	//! Parameters setting routine.
	Params_Tuner && params_tuner )
{
	so_5::environment_params_t params;
	params_tuner( params );

	so_5::impl::so_quick_environment_t<Init_Routine> env{
			std::forward<Init_Routine>(init_routine),
			std::move(params)
	};

	env.run();
}

} /* namespace so_5 */

