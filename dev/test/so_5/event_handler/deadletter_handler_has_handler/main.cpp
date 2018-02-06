/*
 * A test for so_has_deadletter_handler.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

struct first_request final : public so_5::signal_t {};

struct second_request final : public so_5::signal_t {};

class provider_t final : public so_5::agent_t
{
	state_t st_test{ this, "test" };

public :
	provider_t( context_t ctx ) : so_5::agent_t( std::move(ctx) ) {}

	virtual void
	so_define_agent() override
	{
		this >>= st_test;

		st_test.event( [](mhood_t<first_request>) -> std::string {
				return "first";
			} );

		so_subscribe_deadletter_handler(
			so_direct_mbox(),
			[](mhood_t<second_request>) -> std::string {
				return "second";
			} );
	}

	virtual void
	so_evt_start() override
	{
		ensure_or_die(
				!so_has_deadletter_handler< first_request >( so_direct_mbox() ),
				"should have no deadletter handler for first_request" );

		ensure_or_die(
				so_has_deadletter_handler< second_request >( so_direct_mbox() ),
				"should have deadletter handler for second_request" );

		so_drop_deadletter_handler< second_request >( so_direct_mbox() );
		ensure_or_die(
				!so_has_deadletter_handler< second_request >( so_direct_mbox() ),
				"should have no deadletter handler for second_request" );

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
				so_5::launch( [&]( so_5::environment_t & env ) {
					env.introduce_coop( []( so_5::coop_t & coop ) {
						coop.make_agent< provider_t >();
					} );
				} );
			},
			20 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 2;
	}

	return 0;
}

