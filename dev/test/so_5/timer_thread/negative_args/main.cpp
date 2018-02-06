/*
 * Test of using negative values for send_delayed and send_periodic.
 */

#include <iostream>
#include <stdexcept>
#include <string>

#include <so_5/all.hpp>

#include <various_helpers_1/ensure.hpp>
#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t final : public so_5::agent_t
	{
		struct sig_A : public so_5::signal_t {};

	public :
		a_test_t( context_t ctx )
			:	so_5::agent_t( ctx )
			{}

		virtual void
		so_evt_start() override
			{
				try
					{
						(void)so_5::send_delayed<sig_A>( *this,
								-std::chrono::milliseconds(200) );
						ensure_or_die( false, "an exeption must be thrown!" );
					}
				catch( const so_5::exception_t & x )
					{
						ensure_or_die( x.error_code() ==
								so_5::rc_negative_value_for_pause,
								"rc_negative_value_for_pause expected!" );
					}

				try
					{
						(void)so_5::send_periodic<sig_A>( *this,
								-std::chrono::milliseconds(200),
								std::chrono::milliseconds(300) );
						ensure_or_die( false, "an exeption must be thrown!" );
					}
				catch( const so_5::exception_t & x )
					{
						ensure_or_die( x.error_code() ==
								so_5::rc_negative_value_for_pause,
								"rc_negative_value_for_pause expected!" );
					}

				try
					{
						(void)so_5::send_periodic<sig_A>( *this,
								std::chrono::milliseconds(200),
								-std::chrono::milliseconds(300) );
						ensure_or_die( false, "an exeption must be thrown!" );
					}
				catch( const so_5::exception_t & x )
					{
						ensure_or_die( x.error_code() ==
								so_5::rc_negative_value_for_period,
								"rc_negative_value_for_pause expected!" );
					}

				so_deregister_agent_coop_normally();
			}
	};

int
main()
	{
		try
			{
				run_with_time_limit(
					[]()
					{
						so_5::launch( []( so_5::environment_t & env ) {
							env.register_agent_as_coop( so_5::autoname,
								env.make_agent< a_test_t >() );
						} );
					},
					10 );
			}
		catch( const std::exception & ex )
			{
				std::cerr << "Error: " << ex.what() << std::endl;
				return 1;
			}

		return 0;
	}

