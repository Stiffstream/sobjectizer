/*
 * Test for message_holder_t.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

struct so5_message final : public so_5::message_t
{
	int m_a;
	std::string m_b;
	std::chrono::milliseconds m_c;

	so5_message( int a, std::string b, std::chrono::milliseconds c )
		:	m_a{ a }
		,	m_b{ std::move(b) }
		,	m_c{ c }
	{}
};

struct user_message final
{
	int m_a;
	std::string m_b;
	std::chrono::milliseconds m_c;

	user_message( int a, std::string b, std::chrono::milliseconds c )
		:	m_a{ a }
		,	m_b{ std::move(b) }
		,	m_c{ c }
	{}
};

const int expected_a{ 234 };
const std::string expected_b{ "Hello!" };
const std::chrono::milliseconds expected_c{ 12345 };

template< typename Msg, so_5::message_ownership_t Ownership >
struct make_then_send_case_t
{
	static constexpr const int expected = 1;

	static void
	run( const so_5::mbox_t & target )
	{
		so_5::send(
				target,
				so_5::message_holder_t< Msg, Ownership >::make(
						expected_a,
						expected_b,
						expected_c ) );
	}
};

template<
	typename Msg,
	so_5::message_ownership_t Ownership >
class test_t final : public so_5::agent_t
{
public :
	test_t( context_t ctx )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_values_to_receive{
				make_then_send_case_t<Msg, Ownership>::expected
			}
	{
		so_subscribe_self().event( &test_t::on_message );
	}

	void
	so_evt_start() override
	{
		make_then_send_case_t<Msg, Ownership>::run( so_direct_mbox() );
	}

private :
	const int m_values_to_receive;
	int m_values_received{};

	void
	on_message( mhood_t<Msg> cmd )
	{
		(void)cmd;

		m_values_received += 1;

		if( m_values_to_receive == m_values_received )
			so_deregister_agent_coop_normally();
	}
};

template<
	typename Msg,
	so_5::message_ownership_t Ownership >
void
do_test( std::string_view case_name )
{
	std::cout << case_name << "..." << std::flush;

	run_with_time_limit( [] {
		so_5::launch(
			[]( so_5::environment_t & env )
			{
				env.register_agent_as_coop(
						env.make_agent< test_t<Msg, Ownership> >() );
			}/*,
			[]( so_5::environment_params_t & params )
			{
				params.message_delivery_tracer(
						so_5::msg_tracing::std_cout_tracer() );
			}*/ );
		},
		10 );

	std::cout << " OK!" << std::endl; 
}

int
main()
{
	do_test< so5_message, so_5::message_ownership_t::autodetected >(
			"so5_message: immutable, autodetected" );
	do_test< so5_message, so_5::message_ownership_t::shared >(
			"so5_message: immutable, shared" );
	do_test< so5_message, so_5::message_ownership_t::unique >(
			"so5_message: immutable, unique" );

	do_test< so_5::mutable_msg<so5_message>, so_5::message_ownership_t::autodetected >(
			"so5_message: mutable, autodetected" );
	do_test< so_5::mutable_msg<so5_message>, so_5::message_ownership_t::shared >(
			"so5_message: mutable, shared" );
	do_test< so_5::mutable_msg<so5_message>, so_5::message_ownership_t::unique >(
			"so5_message: mutable, unique" );

	do_test< user_message, so_5::message_ownership_t::autodetected >(
			"user_message: immutable, autodetected" );
	do_test< user_message, so_5::message_ownership_t::shared >(
			"user_message: immutable, shared" );
	do_test< user_message, so_5::message_ownership_t::unique >(
			"user_message: immutable, unique" );

	do_test< so_5::mutable_msg<user_message>, so_5::message_ownership_t::autodetected >(
			"user_message: mutable, autodetected" );
	do_test< so_5::mutable_msg<user_message>, so_5::message_ownership_t::shared >(
			"user_message: mutable, shared" );
	do_test< so_5::mutable_msg<user_message>, so_5::message_ownership_t::unique >(
			"user_message: mutable, unique" );

	return 0;
}

