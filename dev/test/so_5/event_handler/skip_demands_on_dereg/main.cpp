/*
 * A simple test for so_5::skip_demands_on_dereg.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace test
{

class a_test_t final : public so_5::agent_t
{
	struct first final : public so_5::signal_t {};
	struct second final : public so_5::signal_t {};
	struct third final : public so_5::signal_t {};

public:
	a_test_t( context_t ctx )
		: so_5::agent_t{ ctx + so_5::skip_demands_on_dereg }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( [this]( mhood_t<first> ) {
					so_deregister_agent_coop_normally();
				} )
			.event( []( mhood_t<second> ) {
					throw std::runtime_error{ "second message received" };
				} )
			.event( []( mhood_t<third> ) {
					throw std::runtime_error{ "third message received" };
				} )
			;
	}

	void
	so_evt_start() override
	{
		so_5::send< first >( *this );
		so_5::send< second >( *this );
		so_5::send< third >( *this );
	}
};

} /* namespace test */

using namespace test;

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( []( so_5::environment_t & env ) {
						env.introduce_coop( []( so_5::coop_t & coop ) {
								coop.make_agent< a_test_t >();
							} );
					} );
			},
			5 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 2;
	}

	return 0;
}

