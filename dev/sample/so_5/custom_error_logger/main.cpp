/*
 * A sample for the custom error logger and cooperation notifications.
 */

#include <iostream>
#include <stdexcept>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// A cooperation notificator which will not throw exceptions.
void normal_coop_reg_notificator(
	so_5::environment_t &,
	const so_5::coop_handle_t & coop )
{
	std::cout << "cooperation registered: " << coop << std::endl;
}

// A cooperation notificator which will throw exception.
void invalid_coop_reg_notificator(
	so_5::environment_t &,
	const so_5::coop_handle_t & coop )
{
	std::ostringstream ss;
	ss << "some problem during handling cooperation registration "
			"notification for cooperation: " << coop;

	throw std::runtime_error( ss.str() );
}

// A class of parent agent.
class a_parent_t : public so_5::agent_t
{
public :
	a_parent_t( context_t ctx ) :	so_5::agent_t( ctx )
	{}

	virtual void so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_parent_t::evt_child_created );
	}

	void so_evt_start() override
	{
		using namespace so_5;

		introduce_child_coop( *this, [this]( coop_t & coop ) {
			// Add necessary cooperation notificators for coop.
			coop.add_reg_notificator(
					make_coop_reg_notificator( so_direct_mbox() ) );
			coop.add_reg_notificator( normal_coop_reg_notificator );
			coop.add_reg_notificator( invalid_coop_reg_notificator );

			// A cooperation agent.
			class coop_agent_t final : public so_5::agent_t {
			public :
				using so_5::agent_t::agent_t;

				void so_evt_start() override {
					std::cout << "Child started!" << std::endl;
				}
			};
			coop.make_agent<coop_agent_t>();

			std::cout << "registering coop: " << coop.handle()
					<< std::endl;
		});
	}

	void evt_child_created(
		const so_5::msg_coop_registered & evt )
	{
		std::cout << "registration passed: " << evt.m_coop << std::endl;

		so_deregister_agent_coop_normally();
	}
};

// Custom error logger.
class custom_logger_t : public so_5::error_logger_t
{
public :
	custom_logger_t()
	{}

	virtual void log(
		const char * file_name,
		unsigned int line,
		const std::string & message ) override
	{
		std::clog << file_name << "(" << line << "): error: "
			<< message << std::endl;
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
				env.register_agent_as_coop( env.make_agent< a_parent_t >() );
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

