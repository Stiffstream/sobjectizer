/*
 * A simple test for so_deactivate_agent().
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

class a_terminator_t final : public so_5::agent_t
{
public:
	struct kill final : public so_5::signal_t {};

	using so_5::agent_t::agent_t;

	void
	so_define_agent() override
	{
		so_subscribe( so_environment().create_mbox( "terminator" ) )
			.event( [this]( mhood_t<kill> ) {
					so_deregister_agent_coop_normally();
				} );
	}
};

class a_test_t final : public so_5::agent_t
{
	struct first final : public so_5::signal_t {};
	struct second final : public so_5::signal_t {};

public:
	using so_5::agent_t::agent_t;

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( [this]( mhood_t<first> ) {
					so_5::send< a_terminator_t::kill >(
							so_environment().create_mbox( "terminator" ) );
					so_deactivate_agent();
				} )
			.event( [this]( mhood_t<second> ) {
					throw std::runtime_error{ "second message received" };
				} );
	}

	void
	so_evt_start() override
	{
		so_5::send< first >( *this );
		so_5::send< second >( *this );
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
						env.introduce_coop( []( so_5::coop_t & coop ) {
								coop.make_agent< a_terminator_t >();
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

