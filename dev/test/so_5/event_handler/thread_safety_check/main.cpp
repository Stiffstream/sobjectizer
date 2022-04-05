/*
 * A test for checking of actions inside thread_safe event-handlers.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

class a_test_t final : public so_5::agent_t
{
	struct first final : public so_5::signal_t {};
	struct second final : public so_5::signal_t {};
	struct quit final : public so_5::signal_t {};

	state_t st_dummy{ this };

	struct dummy_msg final : public so_5::message_t
	{
		int m_key;

		dummy_msg( int key ) : m_key{key} {}
	};

public:
	using so_5::agent_t::agent_t;

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_test_t::evt_first )
			.event( &a_test_t::evt_second, so_5::thread_safe )
			.event( &a_test_t::evt_quit )
			;
	}

	void
	so_evt_start() override
	{
		so_5::send< first >( *this );
		so_5::send< second >( *this );
		so_5::send< quit >( *this );
	}

private:
	void
	evt_first( mhood_t<first> )
	{
		so_subscribe_self()
			.in( st_dummy )
			.event( []( mhood_t<dummy_msg> ) {} );

		this >>= st_dummy;

		so_drop_subscription< dummy_msg >( so_direct_mbox(), st_dummy );

		this >>= so_default_state();
	}

	void
	evt_second( mhood_t<second> )
	{
		ensure_throws(
				[this]() {
					so_subscribe_self()
						.in( st_dummy )
						.event( []( mhood_t<dummy_msg> ) {} );
				},
				"create subscription should fail" );

		ensure_throws(
				[this]() {
					st_dummy.activate();
				},
				"change agent state should fail" );

		ensure_throws(
				[this]() {
					so_drop_subscription< dummy_msg >( so_direct_mbox(), st_dummy );
				},
				"dropping subscription should fail" );

		ensure_throws(
				[this]() {
					so_default_state().activate();
				},
				"change agent state should fail" );
	}

	void
	evt_quit( mhood_t<quit> )
	{
		so_deregister_agent_coop_normally();
	}

	template< typename Lambda >
	void
	ensure_throws(
		Lambda && lambda,
		const char * failure_description )
	{
		bool exception_thrown = false;
		try
		{
			lambda();
		}
		catch( const so_5::exception_t & )
		{
			exception_thrown = true;
		}

		if( !exception_thrown )
			throw std::runtime_error{ failure_description };
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

