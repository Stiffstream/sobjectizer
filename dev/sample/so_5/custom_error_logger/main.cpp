/*
 * A sample for the custom error logger.
 */

#include <iostream>
#include <stdexcept>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// A class that uses limit_then_redirect incorrectly.
// Message is redirected too many times and this lead to error message.
class actor_t final : public so_5::agent_t
{
	struct hello final : public so_5::signal_t {};
	struct bye final : public so_5::signal_t {};

public :
	actor_t( context_t ctx )
		:	so_5::agent_t{ ctx
				// There is a mistake: message is redirected to the same mbox.
				+ limit_then_redirect< hello >(
						1, [this]{ return so_direct_mbox(); } )
				+ limit_then_abort< bye >( 1 ) }
	{}

	void so_define_agent() override
	{
		so_subscribe_self()
			.event( []( mhood_t<hello> ) { std::cout << "Hello!" << std::endl; } )
			.event( [this]( mhood_t<bye> ) { so_deregister_agent_coop_normally(); } );
	}

	void so_evt_start() override
	{
		so_5::send< hello >( *this );
		so_5::send< hello >( *this ); // This message will be redirected.
		so_5::send< bye >( *this );
	}
};

// Custom error logger.
class custom_logger_t final : public so_5::error_logger_t
{
public :
	custom_logger_t()
	{}

	void log(
		const char * file_name,
		unsigned int line,
		const std::string & message ) override
	{
		std::clog
			<< "############################################################\n"
			<< file_name << "(" << line << "): error: "
			<< message << "\n"
				"############################################################"
			<< std::endl;
	}
};

int main()
{
	try
	{
		so_5::launch(
			// The SObjectizer Environment initialization.
			[]( so_5::environment_t & env )
			{
				// Creating and registering a cooperation.
				env.register_agent_as_coop( env.make_agent< actor_t >() );
			},
			// Parameters for SObjectizer Environment.
			[]( so_5::environment_params_t & params )
			{
				params.error_logger(
					std::make_shared< custom_logger_t >() );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

