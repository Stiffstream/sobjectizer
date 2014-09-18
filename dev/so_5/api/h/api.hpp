/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Function for the SObjectizer starting.

	It is not necessary to derive a class from the so_5::rt::so_environment_t
	to start a SObjectizer Environment. SObjectizer contains several functions
	which make a SObjectizer Environment launching process easier.

	This file contains declarations of these functions.
*/

#if !defined( _SO_5__API__API_HPP_ )
#define _SO_5__API__API_HPP_

#include <functional>

#include <so_5/h/declspec.hpp>

#include <so_5/rt/h/so_environment.hpp>

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
template< class INIT >
class so_quick_environment_t
	:
		public so_5::rt::so_environment_t
{
		typedef so_5::rt::so_environment_t base_type_t;

	public:
		so_quick_environment_t(
			//! Initialization routine.
			INIT init,
			//! SObjectizer Environment parameters.
			so_5::rt::so_environment_params_t && env_params )
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
		INIT m_init;
};

} /* namespace impl */

//! Typedef for a simple SObjectizer-initialization function.
typedef void (*pfn_so_environment_init_t)(
		so_5::rt::so_environment_t & );

/*!
 * \since v.5.3.0
 * \brief Generic type for a simple SObjectizer-initialization function.
 */
typedef std::function< void(so_5::rt::so_environment_t &) >
	generic_simple_init_t;

/*!
 * \since v.5.3.0
 * \brief Generic type for a simple SO Environment paramenters tuning function.
 */
typedef std::function< void(so_5::rt::so_environment_params_t &) >
	generic_simple_so_env_params_tuner_t;


//! Launch a SObjectizer Environment with arguments.
/*!
Example:

\code
void
init( so_5::rt::so_environment_t & env )
{
	auto coop = env.create_coop( "main_coop" );
	coop->add_agent(
		so_5::rt::agent_ref_t(
			new a_main_t ) );

	env.register_coop( std::move( coop ) );
}

...

int
main( int argc, char * argv[] )
{
	so_5::api::run_so_environment(
		&init,
		std::move(
			so_5::rt::so_environment_params_t()
				.mbox_mutex_pool_size( 16 )
				.agent_coop_mutex_pool_size( 16 )
				.agent_event_queue_mutex_pool_size( 16 ) ) );

	return 0;
}
\endcode
*/
inline void
run_so_environment(
	//! Initialization routine.
	generic_simple_init_t init_routine,
	//! Environment's parameters.
	so_5::rt::so_environment_params_t && env_params )
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
init( so_5::rt::so_environment_t & env )
{
	auto coop = env.create_coop( "main_coop" );
	coop->add_agent(
		so_5::rt::agent_ref_t(
			new a_main_t ) );

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
*/
inline void
run_so_environment(
	//! Initialization routine.
	generic_simple_init_t init_routine )
{
	run_so_environment(
			init_routine,
			so_5::rt::so_environment_params_t() );
}

/*!
 * \since v.5.3.0
 *
 * \brief  Launch a SObjectizer Environment with arguments.
 *
 * A lambda-functions could be used as for starting actions and as
 * environment parameters tunning.

Example:

\code
void
init( so_5::rt::so_environment_t & env )
{
	auto coop = env.create_coop( "main_coop" );
	coop->add_agent(
		so_5::rt::agent_ref_t(
			new a_main_t ) );

	env.register_coop( std::move( coop ) );
}

...

int
main( int argc, char * argv[] )
{
	so_5::api::run_so_environment(
		&init,
		[](so_5::rt::so_environment_params_t & params ) {
			params.mbox_mutex_pool_size( 16 )
				.agent_coop_mutex_pool_size( 16 )
				.agent_event_queue_mutex_pool_size( 16 );
		} );

	return 0;
}
\endcode
*/
inline void
run_so_environment(
	//! Initialization routine.
	generic_simple_init_t init_routine,
	//! Environment's parameters tuner.
	generic_simple_so_env_params_tuner_t params_tuner )
{
	so_5::rt::so_environment_params_t params;
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
	so_5::rt::so_environment_t & env,
	const std::string & server_addr )
{
	// Make a cooperation.
	so_5::rt::agent_coop_unique_ptr_t coop = env.create_coop(
		so_5::rt::nonempty_name_t( "test_server_application" ),
		so_5::disp::active_obj::create_disp_binder(
			"active_obj" ) );

	so_5_transport::socket::acceptor_controller_creator_t
		acceptor_creator( env );

	using so_5_transport::a_server_transport_agent_t;
	std::unique_ptr< a_server_transport_agent_t > ta(
		new a_server_transport_agent_t(
			env,
			acceptor_creator.create( server_addr ) ) );

	so_5::rt::agent_ref_t serv(
		new a_main_t( env, ta->query_notificator_mbox() ) );

	coop->add_agent( serv );
	coop->add_agent( so_5::rt::agent_ref_t( ta.release() ) );

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
				so_5::rt::so_environment_params_t()
					.add_named_dispatcher(
						so_5::rt::nonempty_name_t( "active_obj" ),
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
*/
template< class INIT, class PARAM_TYPE >
void
run_so_environment_with_parameter(
	//! Initialization routine.
	/*!
		The prototype: <i>void init( env, my_param )</i> is required.
	*/
	INIT init_func,
	//! Initialization routine argument.
	const PARAM_TYPE & param,
	//! SObjectizer Environment parameters.
	so_5::rt::so_environment_params_t && env_params )
{
	auto init = [init_func, param]( so_5::rt::so_environment_t & env ) {
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
	so_5::rt::so_environment_t & env,
	const std::string & server_addr )
{
	// Make a cooperation.
	so_5::rt::agent_coop_unique_ptr_t coop = env.create_coop(
		so_5::rt::nonempty_name_t( "test_server_application" ),
		so_5::disp::active_obj::create_disp_binder(
			"active_obj" ) );

	so_5_transport::socket::acceptor_controller_creator_t
		acceptor_creator( env );

	using so_5_transport::a_server_transport_agent_t;
	std::unique_ptr< a_server_transport_agent_t > ta(
		new a_server_transport_agent_t(
			env,
			acceptor_creator.create( server_addr ) ) );

	so_5::rt::agent_ref_t serv(
		new a_main_t( env, ta->query_notificator_mbox() ) );

	coop->add_agent( serv );
	coop->add_agent( so_5::rt::agent_ref_t( ta.release() ) );

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
				so_5::rt::so_environment_params_t()
					.add_named_dispatcher(
						so_5::rt::nonempty_name_t( "active_obj" ),
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
*/

template< class INIT, class PARAM_TYPE >
void
run_so_environment_with_parameter(
	//! Initialization routine.
	/*!
		The prototype: <i>void init( env, my_param )</i> is required.
	*/
	INIT init_func,
	//! Initialization routine argument.
	const PARAM_TYPE & param )
{
	run_so_environment_with_parameter(
			init_func,
			param,
			so_5::rt::so_environment_params_t() );
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
	init( so_5::rt::so_environment_t & env )
	{
		// Make a cooperation.
		so_5::rt::agent_coop_unique_ptr_t coop = env.create_coop(
			so_5::rt::nonempty_name_t( "test_client_application" ),
			so_5::disp::active_obj::create_disp_binder(
				"active_obj" ) );

		so_5_transport::socket::connector_controller_creator_t
			connector_creator( env );

		using so_5_transport::a_client_transport_agent_t;
		std::unique_ptr< a_client_transport_agent_t > ta(
			new a_client_transport_agent_t(
				env,
				connector_creator.create( m_server_addr ) ) );

		so_5::rt::agent_ref_t client(
			new a_main_t(
				env,
				ta->query_notificator_mbox(),
				m_rest_of_argv ) );

		coop->add_agent( client );
		coop->add_agent( so_5::rt::agent_ref_t( ta.release() ) );

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
				so_5::rt::so_environment_params_t()
					.add_named_dispatcher(
						so_5::rt::nonempty_name_t( "active_obj" ),
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
*/
template< class OBJECT, class METHOD >
void
run_so_environment_on_object(
	//! Initialization object. Its method should be used as
	//! the initialization routine.
	OBJECT & obj,
	//! Initialization routine.
	METHOD init_func,
	//! SObjectizer Environment parameters.
	so_5::rt::so_environment_params_t && env_params )
{
	auto init = [&obj, init_func]( so_5::rt::so_environment_t & env ) {
			(obj.*init_func)( env );
		};

	impl::so_quick_environment_t< decltype(init) > env(
			init, std::move(env_params) );

	env.run();
}

//! Launch a SObjectizer Environment by a class method.
template< class OBJECT, class METHOD >
void
run_so_environment_on_object(
	//! Initialization object. Its method should be used as
	//! the initialization routine.
	OBJECT & obj,
	//! Initialization routine.
	METHOD init_func )
{
	run_so_environment_on_object(
			obj,
			init_func,
			so_5::rt::so_environment_params_t() );
}

} /* namespace api */

} /* namespace so_5 */

#endif
