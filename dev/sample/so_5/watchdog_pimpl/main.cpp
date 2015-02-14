#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <cstdlib>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// Watchdog public interface.
#include <sample/so_5/watchdog_pimpl/h/watchdog.hpp>

// Sample configuration.
struct cfg_t
{
	// Virtual operation duration.
	std::chrono::steady_clock::duration m_op_duration =
		std::chrono::milliseconds( 2015 );

	// Operation watchdog timeout.
	std::chrono::steady_clock::duration m_op_timeout =
		std::chrono::milliseconds( 2500 );

	// Check timedout operation interval.
	std::chrono::steady_clock::duration m_check_interval =
		std::chrono::seconds( 1 );
};

cfg_t
try_parse_cmdline(
	int argc,
	char ** argv )
	{
		auto is_arg =
			[]( const char * value, const char * v1, const char * v2 ){
				return 0 == std::strcmp( value, v1 ) ||
						0 == std::strcmp( value, v2 );
			};

		cfg_t result;

		char ** current = argv + 1;
		char ** last = argv + argc;

		while( current != last )
		{
			if( is_arg( *current, "-o", "--operation-duration" ) )
			{
				++current;
				if( current == last )
					throw std::runtime_error( "-o requires argument" );

				result.m_op_duration =
					std::chrono::milliseconds( std::atoi( *current ) );
			}
			else if( is_arg( *current, "-t", "--operation-timeout" ) )
			{
				++current;
				if( current == last )
					throw std::runtime_error( "-t requires argument" );

				result.m_op_timeout =
					std::chrono::milliseconds( std::atoi( *current ) );
			}
			else if( is_arg( *current, "-i", "--check-interval" ) )
			{
				++current;
				if( current == last )
					throw std::runtime_error( "-i requires argument" );

				result.m_check_interval =
					std::chrono::milliseconds( std::atoi( *current ) );
			}
			else
			{
				std::cout << "usage:\n"
						"sample.so_5.watch_dog_pimpl <options>\n"
						"\noptions:\n"
						"-o, --operation-duration    Operation duration.\n"
						"-t, --operation-timeout     Operation timeout.\n"
						"-i, --check-interval        Timedout operations check interval.\n"
						<< std::endl;

				throw std::runtime_error( "No command-line errors" );

				throw std::runtime_error(
					std::string( "unknown argument: " ) + *current );
			}

			++current;
		}

		return result;
	}

void
show_cfg(
	const cfg_t & cfg )
{
	using namespace std::chrono;

	std::cout << "Configuration:\n"
		"operation duration: "
			<< duration_cast< milliseconds >( cfg.m_op_duration ).count() << " msec.\n"
		"operation timeout:  "
			<< duration_cast< milliseconds >( cfg.m_op_timeout ).count() << " msec.\n"
		"check interval:     "
			<< duration_cast< milliseconds >( cfg.m_check_interval ).count() << " msec."
		<< std::endl;
}

int
main( int argc, char ** argv )
{
	try
	{
		auto cfg = try_parse_cmdline( argc, argv );

		show_cfg( cfg );

		// Create SO Environment objects and run SO Run-Time inside it.
		so_5::launch(
			// SO Environment initialization routine.
			[&]( so_5::rt::environment_t & env )
			{
				// Creating a cooperation.
				auto coop = env.create_coop( "coop" );

				auto watchdog =
					coop->add_agent(
						new a_watchdog_t( env, cfg.m_check_interval ),
						// Watchdog must run in a separate thread.
						so_5::disp::active_obj::create_disp_binder( "active_obj" ) )
						->create_watchdog();

				coop->define_agent()
					.on_start(
						[&env, watchdog, cfg ]() {
							std::cout << "Start long operation..." << std::endl;

							{
								// Perfom watched operation.
								operation_watchdog_t op(
									watchdog,
									"Long operation",
									cfg.m_op_timeout );

								std::this_thread::sleep_for( cfg.m_op_duration );
							}
							std::cout << "Finish long operation..." << std::endl;

							// Shutting down SObjectizer.
							env.stop();
						} );

				// Registering the cooperation.
				env.register_coop( std::move( coop ) );
			},
			[]( so_5::rt::environment_params_t & p )
			{
				p.add_named_dispatcher(
					"active_obj",
					so_5::disp::active_obj::create_disp() );
			});
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}
}

