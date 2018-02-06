/*
 * A test for formats of deadletter handlers.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

struct msg_1 final : public so_5::message_t {};
struct msg_2 final : public so_5::message_t {};
struct msg_3 final : public so_5::message_t {};
struct msg_4 final : public so_5::message_t {};
struct msg_5 final : public so_5::message_t {};
struct msg_6 final : public so_5::message_t {};
struct msg_7 final : public so_5::message_t {};
struct msg_8 final : public so_5::message_t {};
struct msg_9 final : public so_5::message_t {};
struct msg_10 final : public so_5::message_t {};
struct msg_11 final : public so_5::message_t {};
struct msg_12 final : public so_5::message_t {};

class a_test_t final : public so_5::agent_t
{
public :
	a_test_t( context_t ctx ) : so_5::agent_t( std::move(ctx) ) {}

	virtual void
	so_define_agent() override
	{
		so_subscribe_deadletter_handler(
			so_direct_mbox(), &a_test_t::on_msg_1 );

		so_subscribe_deadletter_handler(
			so_direct_mbox(), &a_test_t::on_msg_2 );

		so_subscribe_deadletter_handler(
			so_direct_mbox(), &a_test_t::on_msg_3 );

		so_subscribe_deadletter_handler(
			so_direct_mbox(), &a_test_t::on_msg_4 );

		so_subscribe_deadletter_handler(
			so_direct_mbox(), &a_test_t::on_msg_5 );

		so_subscribe_deadletter_handler(
			so_direct_mbox(), &a_test_t::on_msg_6 );

		so_subscribe_deadletter_handler(
			so_direct_mbox(), &a_test_t::on_msg_7 );

		so_subscribe_deadletter_handler(
			so_direct_mbox(), &a_test_t::on_msg_8 );

		so_subscribe_deadletter_handler(
			so_direct_mbox(), &a_test_t::on_msg_9 );

		so_subscribe_deadletter_handler(
			so_direct_mbox(), &a_test_t::on_msg_10 );

		so_subscribe_deadletter_handler(
			so_direct_mbox(), &a_test_t::on_msg_11 );

		so_subscribe_deadletter_handler(
			so_direct_mbox(), &a_test_t::on_msg_12 );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send<msg_1>( *this );
		so_5::send<msg_2>( *this );
		so_5::send<msg_3>( *this );
		so_5::send<msg_4>( *this );
		so_5::send<msg_5>( *this );
		so_5::send<msg_6>( *this );
		so_5::send<msg_7>( *this );
		so_5::send<msg_8>( *this );
		so_5::send<so_5::mutable_msg<msg_9>>( *this );
		so_5::send<so_5::mutable_msg<msg_10>>( *this );
		so_5::send<so_5::mutable_msg<msg_11>>( *this );
		so_5::send<so_5::mutable_msg<msg_12>>( *this );

		so_deregister_agent_coop_normally();
	}

	virtual void
	so_evt_finish() override
	{
		ensure_or_die( m_received == expected,
				"received != expected, received=" +
				std::to_string(m_received) + ", expected=" +
				std::to_string(expected) );
	}

private :
	mutable int m_received{ 0 };
	static const int expected = 12;

	void
	handle_received() const { ++m_received; }

	void
	on_msg_1( msg_1 ) { handle_received(); }

	void
	on_msg_2( msg_2 ) const { handle_received(); }

	void
	on_msg_3( const msg_3 & ) { handle_received(); }

	void
	on_msg_4( const msg_4 & ) const { handle_received(); }

	void
	on_msg_5( mhood_t<msg_5> ) { handle_received(); }

	void
	on_msg_6( mhood_t<msg_6> ) const { handle_received(); }

	void
	on_msg_7( const mhood_t<msg_7> & ) { handle_received(); }

	void
	on_msg_8( const mhood_t<msg_8> & ) const { handle_received(); }

	void
	on_msg_9( mutable_mhood_t<msg_9> ) { handle_received(); }

	void
	on_msg_10( mutable_mhood_t<msg_10> ) const { handle_received(); }

	void
	on_msg_11( const mutable_mhood_t<msg_11> & ) { handle_received(); }

	void
	on_msg_12( const mutable_mhood_t<msg_12> & ) const { handle_received(); }
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
						coop.make_agent< a_test_t >();
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

