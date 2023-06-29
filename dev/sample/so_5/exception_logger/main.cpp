/*
 * A sample of the exception logger.
 */

#include <iostream>
#include <stdexcept>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// A class of the exception logger.
class sample_event_exception_logger_t final
	: public so_5::event_exception_logger_t
{
	public:
		// A reaction to an exception.
		void log_exception(
			const std::exception & event_exception,
			const so_5::coop_handle_t & coop ) noexcept override
		{
			std::cerr
				<< "Event_exception, coop:"
				<< coop << "; "
				" error: "
				<< event_exception.what()
				<< std::endl;
		}
};

// A class of an agent which will throw an exception.
class a_hello_t final : public so_5::agent_t
{
	public:
		a_hello_t( context_t ctx ) : so_5::agent_t( ctx )
		{}

		// A reaction to start work in SObjectizer.
		void so_evt_start() override
		{
			so_environment().install_exception_logger(
				so_5::event_exception_logger_unique_ptr_t(
					new sample_event_exception_logger_t ) );

			throw std::runtime_error( "sample exception" );
		}

		// An instruction to SObjectizer for unhandled exception.
		so_5::exception_reaction_t so_exception_reaction() const noexcept override
		{
			return so_5::deregister_coop_on_exception;
		}
};

int main()
{
	try
	{
		so_5::launch(
			// SObjectizer initialization code.
			[]( so_5::environment_t & env )
			{
				// Creating and registering cooperation with a single agent.
				env.register_agent_as_coop( env.make_agent< a_hello_t >() );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
