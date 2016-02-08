/*
 * A simple test for various format of lambda handlers.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t : public so_5::agent_t
{
	struct s1 : public so_5::signal_t {};
	struct s2 : public so_5::signal_t {};
	struct s3 : public so_5::signal_t {};
	struct s4 : public so_5::signal_t {};
	struct s5 : public so_5::signal_t {};
	struct s6 : public so_5::signal_t {};

	struct stop : public so_5::signal_t {};

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( ctx )
	{}

	virtual void
	so_define_agent() override
	{
		so_subscribe_self().event< s1 >( [this] { m_result += "s1;"; } );
		so_subscribe_self().event< s2 >( [this]() mutable { m_result += "s2;"; } );
		so_subscribe_self().event(
				[this]( mhood_t< s3 > ) { m_result += "s3;"; } );
		so_subscribe_self().event(
				[this]( mhood_t< s4 > ) mutable { m_result += "s4;"; } );
		so_subscribe_self().event(
				[this]( const mhood_t< s5 > & ) { m_result += "s5;"; } );
		so_subscribe_self().event(
				[this]( const mhood_t< s6 > & ) mutable { m_result += "s6;"; } );

		so_subscribe_self().event< stop >( &a_test_t::on_stop );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< s1 >( *this );
		so_5::send< s2 >( *this );
		so_5::send< s3 >( *this );
		so_5::send< s4 >( *this );
		so_5::send< s5 >( *this );
		so_5::send< s6 >( *this );

		so_5::send< stop >( *this );
	}

private :
	std::string m_result;

	void
	on_stop()
	{
		const std::string expected = "s1;s2;s3;s4;s5;s6;";
		if( expected != m_result )
			throw std::runtime_error( "expected(" + expected + ") != result(" +
					m_result + ")" );

		so_deregister_agent_coop_normally();
	}
};

void
init( so_5::environment_t & env )
{
	env.register_agent_as_coop( so_5::autoname,
			env.make_agent< a_test_t >() );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init /*,
					[]( so_5::environment_params_t & p ) {
						p.message_delivery_tracer( so_5::msg_tracing::std_cout_tracer() );
					}*/ );
			},
			20,
			"simple test for various types of event handlers" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

