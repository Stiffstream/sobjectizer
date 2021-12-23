/*
 * A test for subscription after so_deactivate_agent().
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
			;
	}

	void
	so_evt_start() override
	{
		so_5::send< first >( *this );
	}

private:
	void
	evt_first( mhood_t<first> )
	{
		so_5::send< a_terminator_t::kill >(
				so_environment().create_mbox( "terminator" ) );

		so_deactivate_agent();

		if( so_has_subscription<first>(so_direct_mbox()) )
			throw std::runtime_error{ "subscription isn't dropped" };

		ensure_throws(
			[this]() {
				so_subscribe_self().event( &a_test_t::evt_first );
			},
			"resubscription completed successfully" );

		ensure_throws(
			[this]() {
				so_subscribe_deadletter_handler(
						so_direct_mbox(), &a_test_t::evt_first );
			},
			"deadletter setup completed successfully" );

		ensure_throws(
			[this]() {
				so_set_delivery_filter(
						so_environment().create_mbox( "dummy" ),
						[]( const dummy_msg & msg ) {
							return msg.m_key > 0;
						} );
			},
			"delivery_filter setup completed successfully" );
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

