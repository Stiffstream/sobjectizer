/*
 * A sample for the custom error logger and cooperation notifications.
 */

#include <iostream>
#include <stdexcept>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// A cooperation notificator which will not throw exceptions.
void
normal_coop_reg_notificator(
	so_5::rt::environment_t & env,
	const std::string & coop_name )
{
	std::cout << "cooperation registered: " << coop_name << std::endl;
}

// A cooperation notificator which will throw exception.
void
invalid_coop_reg_notificator(
	so_5::rt::environment_t & env,
	const std::string & coop_name )
{
	throw std::runtime_error(
			"some problem during handling cooperation registration "
			"notification for cooperation: " + coop_name );
}

// A class of parent agent.
class a_parent_t : public so_5::rt::agent_t
{
public :
	a_parent_t(
		so_5::rt::environment_t & env )
		:	so_5::rt::agent_t( env )
	{}

	virtual void
	so_define_agent() override
	{
		so_subscribe( so_direct_mbox() )
			.event( &a_parent_t::evt_child_created );
	}

	void
	so_evt_start() override
	{
		auto coop = so_environment().create_coop( "child" );
		// Cooperation has parent.
		coop->set_parent_coop_name( so_coop_name() );

		// Add necessary cooperation notificators for coop.
		coop->add_reg_notificator(
				so_5::rt::make_coop_reg_notificator( so_direct_mbox() ) );
		coop->add_reg_notificator( normal_coop_reg_notificator );
		coop->add_reg_notificator( invalid_coop_reg_notificator );

		// A cooperation agent.
		coop->define_agent()
			.on_start( []() { std::cout << "Child started!" << std::endl; } );

		std::cout << "registering coop: " << coop->query_coop_name()
				<< std::endl;

		// Registering cooperation.
		so_environment().register_coop( std::move( coop ) );
	}

	void
	evt_child_created(
		const so_5::rt::msg_coop_registered & evt )
	{
		std::cout << "registration passed: " << evt.m_coop_name << std::endl;

		so_deregister_agent_coop_normally();
	}
};

// Custom error logger.
class custom_logger_t : public so_5::error_logger_t
{
public :
	custom_logger_t()
	{}

	virtual void
	log(
		const char * file_name,
		unsigned int line,
		const std::string & message ) override
	{
		std::clog << file_name << "(" << line << "): error: "
			<< message << std::endl;
	}
};

int
main( int, char ** )
{
	try
	{
		so_5::launch(
			// The SObjectizer Environment initialization.
			[]( so_5::rt::environment_t & env )
			{
				// Creating and registering a cooperation.
				env.register_agent_as_coop( "parent", new a_parent_t( env ) );
			},
			// Parameters for SObjectizer Environment.
			[]( so_5::rt::environment_params_t & params )
			{
				params.error_logger(
					so_5::error_logger_shptr_t( new custom_logger_t() ) );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

