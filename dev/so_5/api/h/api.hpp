/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Function for the SObjectizer starting.

	It is not necessary to derive a class from the so_5::environment_t
	to start a SObjectizer Environment. SObjectizer contains several functions
	which make a SObjectizer Environment launching process easier.

	This file contains declarations of these functions.
*/

#pragma once

#include <functional>

#include <so_5/h/declspec.hpp>
#include <so_5/h/compiler_features.hpp>

#include <so_5/rt/h/environment.hpp>

namespace so_5
{

namespace api
{

namespace impl
{

//! Auxiliary class for the SObjectizer launching.
/*!
 * It is used as wrapper on various types of initialization routines.
 */
template< class Init >
class so_quick_environment_t
	:
		public so_5::environment_t
{
		typedef so_5::environment_t base_type_t;

	public:
		so_quick_environment_t(
			//! Initialization routine.
			Init init,
			//! SObjectizer Environment parameters.
			so_5::environment_params_t && env_params )
			:
				base_type_t( std::move( env_params ) ),
				m_init( init )
		{}
		virtual ~so_quick_environment_t()
		{}

		virtual void
		init()
		{
			m_init( *this );
		}

	private:
		//! Initialization routine.
		Init m_init;
};

} /* namespace impl */

//! Typedef for a simple SObjectizer-initialization function.
typedef void (*pfn_so_environment_init_t)(
		so_5::environment_t & );

/*!
 * \since
 * v.5.3.0
 *
 * \brief Generic type for a simple SObjectizer-initialization function.
 */
typedef std::function< void(so_5::environment_t &) >
	generic_simple_init_t;

/*!
 * \since
 * v.5.3.0
 *
 * \brief Generic type for a simple SO Environment paramenters tuning function.
 */
typedef std::function< void(so_5::environment_params_t &) >
	generic_simple_so_env_params_tuner_t;


//! Launch a SObjectizer Environment with arguments.
/*!
Example:

\code
void
init( so_5::environment_t & env )
{
	auto coop = env.create_coop( "main_coop" );
	coop->make_agent< a_main_t >();
	env.register_coop( std::move( coop ) );
}

...

int
main( int argc, char * argv[] )
{
	so_5::api::run_so_environment(
		&init,
		std::move(
			so_5::environment_params_t()
				.mbox_mutex_pool_size( 16 )
				.agent_coop_mutex_pool_size( 16 )
				.agent_event_queue_mutex_pool_size( 16 ) ) );

	return 0;
}
\endcode

\deprecated Obsolete in v.5.5.0. Use so_5::launch() instead.
*/
inline void
run_so_environment(
	//! Initialization routine.
	generic_simple_init_t init_routine,
	//! Environment's parameters.
	so_5::environment_params_t && env_params )
{
	impl::so_quick_environment_t< generic_simple_init_t > env(
			init_routine,
			std::move(env_params) );

	return env.run();
}

//! Launch a SObjectizer Environment with default arguments.
/*!
Example:
\code
void
init( so_5::environment_t & env )
{
	auto coop = env.create_coop( "main_coop" );
	coop->make_agent< a_main_t >();

	env.register_coop( std::move( coop ) );
}

...

int
main( int argc, char * argv[] )
{
	try
	{
		so_5::api::run_so_environment( &init );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
\endcode

\deprecated Obsolete in v.5.5.0. Use so_5::launch() instead.
*/
inline void
run_so_environment(
	//! Initialization routine.
	generic_simple_init_t init_routine )
{
	run_so_environment(
			init_routine,
			so_5::environment_params_t() );
}

/*!
 * \since
 * v.5.3.0
 *
 * \brief  Launch a SObjectizer Environment with arguments.
 *
 * A lambda-functions could be used as for starting actions and as
 * environment parameters tunning.

Example:

\code
void
init( so_5::environment_t & env )
{
	auto coop = env.create_coop( "main_coop" );
	coop->make_agent< a_main_t >();

	env.register_coop( std::move( coop ) );
}

...

int
main( int argc, char * argv[] )
{
	so_5::api::run_so_environment(
		&init,
		[](so_5::environment_params_t & params ) {
			params.mbox_mutex_pool_size( 16 )
				.agent_coop_mutex_pool_size( 16 )
				.agent_event_queue_mutex_pool_size( 16 );
		} );

	return 0;
}
\endcode

\deprecated Obsolete in v.5.5.0. Use so_5::launch() instead.
*/
inline void
run_so_environment(
	//! Initialization routine.
	generic_simple_init_t init_routine,
	//! Environment's parameters tuner.
	generic_simple_so_env_params_tuner_t params_tuner )
{
	so_5::environment_params_t params;
	params_tuner( params );

	run_so_environment( init_routine, std::move( params ) );
}

//! Launch a SObjectizer Environment with the parametrized 
//! initialization routine and Enviroment parameters.
/*!

Allows to pass an additional argument to the initialization process.
\code
void
init(
	so_5::environment_t & env,
	const std::string & server_addr )
{
	// Make a cooperation.
	so_5::coop_unique_ptr_t coop = env.create_coop(
		"test_server_application",
		so_5::disp::active_obj::create_disp_binder(
			"active_obj" ) );

	so_5_transport::socket::acceptor_controller_creator_t
		acceptor_creator( env );

	using so_5_transport::a_server_transport_agent_t;
	auto ta = coop->make_agent< a_server_transport_agent_t >(
		a_server_transport_agent_t(
			acceptor_creator.create( server_addr ) ) );

	coop->make_agent< a_main_t >( ta->query_notificator_mbox() );

	// Register the cooperation.
	env.register_coop( coop );
}

// ...

int
main( int argc, char ** argv )
{
	if( 2 == argc )
	{
		std::string server_addr( argv[ 1 ] );

		so_5::api::run_so_environment_with_parameter(
			&init,
			server_addr,
			std::move(
				so_5::environment_params_t()
					.add_named_dispatcher( "active_obj",
						so_5::disp::active_obj::create_disp() )
					.add_layer(
						std::unique_ptr< so_5_transport::reactor_layer_t >(
							new so_5_transport::reactor_layer_t ) ) ) );
	}
	else
		std::cerr << "sample.server <port>" << std::endl;

	return 0;
}
\endcode

\deprecated Obsolete in v.5.5.0. Use so_5::launch() instead.
*/
template< class Init, class Param_Type >
void
run_so_environment_with_parameter(
	//! Initialization routine.
	/*!
		The prototype: <i>void init( env, my_param )</i> is required.
	*/
	Init init_func,
	//! Initialization routine argument.
	const Param_Type & param,
	//! SObjectizer Environment parameters.
	so_5::environment_params_t && env_params )
{
	auto init = [init_func, param]( so_5::environment_t & env ) {
			init_func( env, param );
		};

	impl::so_quick_environment_t< decltype(init) > env(
			init,
			std::move(env_params) );

	env.run();
}

//! Launch a SObjectizer Environment with the parametrized initialization
//! routine.
/*!

Allows to pass an additional argument to the initialization process.
\code
void
init(
	so_5::environment_t & env,
	const std::string & server_addr )
{
	// Make a cooperation.
	so_5::coop_unique_ptr_t coop = env.create_coop(
		"test_server_application",
		so_5::disp::active_obj::create_disp_binder(
			"active_obj" ) );

	so_5_transport::socket::acceptor_controller_creator_t
		acceptor_creator( env );

	using so_5_transport::a_server_transport_agent_t;
	auto ta = coop->make_agent< a_server_transport_agent_t >(
		a_server_transport_agent_t(
			acceptor_creator.create( server_addr ) ) );

	coop->make_agent< a_main_t >( ta->query_notificator_mbox() );

	// Register the cooperation.
	env.register_coop( coop );
}

// ...

int
main( int argc, char ** argv )
{
	if( 2 == argc )
	{
		std::string server_addr( argv[ 1 ] );

		so_5::api::run_so_environment_with_parameter(
			&init,
			server_addr,
			std::move(
				so_5::environment_params_t()
					.add_named_dispatcher( "active_obj",
						so_5::disp::active_obj::create_disp() )
					.add_layer(
						std::unique_ptr< so_5_transport::reactor_layer_t >(
							new so_5_transport::reactor_layer_t ) ) ) );
	}
	else
		std::cerr << "sample.server <port>" << std::endl;

	return 0;
}
\endcode

\deprecated Obsolete in v.5.5.0. Use so_5::launch() instead.
*/
template< class Init, class Param_Type >
void
run_so_environment_with_parameter(
	//! Initialization routine.
	/*!
		The prototype: <i>void init( env, my_param )</i> is required.
	*/
	Init init_func,
	//! Initialization routine argument.
	const Param_Type & param )
{
	run_so_environment_with_parameter(
			init_func,
			param,
			so_5::environment_params_t() );
}

//! Launch a SObjectizer Environment by a class method and with
//! specified Environment parameters.
/*!

Example:
\code
struct client_data_t
{
	std::string m_server_addr;
	protocol_parser_t m_protocol_parser;

	void
	init( so_5::environment_t & env )
	{
		// Make a cooperation.
		so_5::coop_unique_ptr_t coop = env.create_coop(
			"test_client_application",
			so_5::disp::active_obj::create_disp_binder(
				"active_obj" ) );

		so_5_transport::socket::connector_controller_creator_t
			connector_creator( env );

		using so_5_transport::a_server_transport_agent_t;
		auto ta = coop->make_agent< a_server_transport_agent_t >(
			a_server_transport_agent_t(
				acceptor_creator.create( server_addr ) ) );

		coop->make_agent< a_main_t >(
				ta->query_notificator_mbox(),
				m_protocol_parser );

		// Register the cooperation.
		env.register_coop( coop );
	}
};

// ...

int
main( int argc, char ** argv )
{
	if( 3 == argc )
	{
		client_data_t client_data;
		client_data.m_server_addr = argv[ 1 ];
		client_data.m_protocol_parser = create_protocol( argv[ 2 ] );

		so_5::api::run_so_environment_on_object(
			client_data,
			&client_data_t::init,
			std::move(
				so_5::environment_params_t()
					.add_named_dispatcher( "active_obj",
						so_5::disp::active_obj::create_disp() )
					.add_layer(
						std::unique_ptr< so_5_transport::reactor_layer_t >(
							new so_5_transport::reactor_layer_t ) ) ) );
	}
	else
		std::cerr << "sample.client <port> <protocol_version>" << std::endl;

	return 0;
}
\endcode

\deprecated Obsolete in v.5.5.0. Use so_5::launch() instead.
*/
template< class Object, class Method >
void
run_so_environment_on_object(
	//! Initialization object. Its method should be used as
	//! the initialization routine.
	Object & obj,
	//! Initialization routine.
	Method init_func,
	//! SObjectizer Environment parameters.
	so_5::environment_params_t && env_params )
{
	auto init = [&obj, init_func]( so_5::environment_t & env ) {
			(obj.*init_func)( env );
		};

	impl::so_quick_environment_t< decltype(init) > env(
			init, std::move(env_params) );

	env.run();
}

//! Launch a SObjectizer Environment by a class method.
/*!
\deprecated Obsolete in v.5.5.0. Use so_5::launch() instead.
*/
template< class Object, class Method >
void
run_so_environment_on_object(
	//! Initialization object. Its method should be used as
	//! the initialization routine.
	Object & obj,
	//! Initialization routine.
	Method init_func )
{
	run_so_environment_on_object(
			obj,
			init_func,
			so_5::environment_params_t() );
}

} /* namespace api */

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
*/
inline void
launch(
	//! Initialization routine.
	so_5::api::generic_simple_init_t init_routine )
{
	so_5::api::impl::so_quick_environment_t< decltype(init_routine) > env(
			init_routine,
			so_5::environment_params_t() );

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

*/
inline void
launch(
	//! Initialization routine.
	so_5::api::generic_simple_init_t init_routine,
	//! Parameters setting routine.
	so_5::api::generic_simple_so_env_params_tuner_t params_tuner )
{
	so_5::environment_params_t params;
	params_tuner( params );

	so_5::api::impl::so_quick_environment_t< decltype(init_routine) > env(
			init_routine,
			std::move( params ) );

	env.run();
}

} /* namespace so_5 */
