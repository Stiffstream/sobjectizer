/*
 * A simple test for so_has_subscription method.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

class a_test_t : public so_5::agent_t
{
	struct sig_1 : public so_5::signal_t {};
	struct sig_2 : public so_5::signal_t {};

	struct msg_1 : public so_5::message_t {};
	struct msg_2 : public so_5::message_t {};
	struct msg_3 : public so_5::message_t {};
	struct msg_4 : public so_5::message_t {};
	struct msg_5 : public so_5::message_t {};

	struct umsg_1 {};
	struct umsg_2 {};
	struct umsg_3 {};
	struct umsg_4 {};
	struct umsg_5 {};
	struct umsg_6 {};

	struct ret_msg_1 : public so_5::message_t {};
	struct ret_msg_2 : public so_5::message_t {};
	struct ret_msg_3 : public so_5::message_t {};
	struct ret_msg_4 : public so_5::message_t {};
	struct ret_msg_5 : public so_5::message_t {};

	struct ret_umsg_1 {};
	struct ret_umsg_2 {};
	struct ret_umsg_3 {};
	struct ret_umsg_4 {};
	struct ret_umsg_5 {};
	struct ret_umsg_6 {};

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( ctx )
	{}

	virtual void
	so_define_agent() override
	{
		so_subscribe_self()
				.event( &a_test_t::on_sig_1 )
				.event( &a_test_t::on_sig_2 )
				.event( &a_test_t::on_msg_1 )
				.event( &a_test_t::on_msg_2 )
				.event( &a_test_t::on_msg_3 )
				.event( &a_test_t::on_msg_4 )
				.event( &a_test_t::on_msg_5 )
				.event( &a_test_t::on_umsg_1 )
				.event( &a_test_t::on_umsg_2 )
				.event( &a_test_t::on_umsg_3 )
				.event( &a_test_t::on_umsg_4 )
				.event( &a_test_t::on_umsg_5 )
				.event( &a_test_t::on_umsg_6 )
				.event( &a_test_t::on_ret_msg_1 )
				.event( &a_test_t::on_ret_msg_2 )
				.event( &a_test_t::on_ret_msg_3 )
				.event( &a_test_t::on_ret_msg_4 )
				.event( &a_test_t::on_ret_msg_5 )
				.event( &a_test_t::on_ret_umsg_1 )
				.event( &a_test_t::on_ret_umsg_2 )
				.event( &a_test_t::on_ret_umsg_3 )
				.event( &a_test_t::on_ret_umsg_4 )
				.event( &a_test_t::on_ret_umsg_5 )
				.event( &a_test_t::on_ret_umsg_6 )
				;

#define ENSURE_SUBSCRIBED(handler) \
ensure( so_has_subscription(so_direct_mbox(), \
		so_default_state(), \
		&a_test_t:: handler), \
		#handler " must be subscribed" )

		ENSURE_SUBSCRIBED(on_sig_1);
		ENSURE_SUBSCRIBED(on_sig_2);
		ENSURE_SUBSCRIBED(on_msg_1);
		ENSURE_SUBSCRIBED(on_msg_2);
		ENSURE_SUBSCRIBED(on_msg_3);
		ENSURE_SUBSCRIBED(on_msg_4);
		ENSURE_SUBSCRIBED(on_msg_5);

		ENSURE_SUBSCRIBED(on_umsg_1);
		ENSURE_SUBSCRIBED(on_umsg_2);
		ENSURE_SUBSCRIBED(on_umsg_3);
		ENSURE_SUBSCRIBED(on_umsg_4);
		ENSURE_SUBSCRIBED(on_umsg_5);
		ENSURE_SUBSCRIBED(on_umsg_6);

		ENSURE_SUBSCRIBED(on_ret_msg_1);
		ENSURE_SUBSCRIBED(on_ret_msg_2);
		ENSURE_SUBSCRIBED(on_ret_msg_3);
		ENSURE_SUBSCRIBED(on_ret_msg_4);
		ENSURE_SUBSCRIBED(on_ret_msg_5);

		ENSURE_SUBSCRIBED(on_ret_umsg_1);
		ENSURE_SUBSCRIBED(on_ret_umsg_2);
		ENSURE_SUBSCRIBED(on_ret_umsg_3);
		ENSURE_SUBSCRIBED(on_ret_umsg_4);
		ENSURE_SUBSCRIBED(on_ret_umsg_5);
		ENSURE_SUBSCRIBED(on_ret_umsg_6);

#undef ENSURE_SUBSCRIBED

		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_sig_1 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_sig_2 );

		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_msg_1 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_msg_2 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_msg_3 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_msg_4 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_msg_5 );

		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_umsg_1 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_umsg_2 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_umsg_3 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_umsg_4 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_umsg_5 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_umsg_6 );

		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_ret_msg_1 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_ret_msg_2 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_ret_msg_3 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_ret_msg_4 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_ret_msg_5 );

		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_ret_umsg_1 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_ret_umsg_2 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_ret_umsg_3 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_ret_umsg_4 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_ret_umsg_5 );
		so_drop_subscription(
				so_direct_mbox(), so_default_state(), &a_test_t::on_ret_umsg_6 );

#define ENSURE_UNSUBSCRIBED(handler) \
ensure( !so_has_subscription(so_direct_mbox(), \
		so_default_state(), \
		&a_test_t:: handler), \
		#handler " must not be subscribed" )

		ENSURE_UNSUBSCRIBED(on_sig_1);
		ENSURE_UNSUBSCRIBED(on_sig_2);
		ENSURE_UNSUBSCRIBED(on_msg_1);
		ENSURE_UNSUBSCRIBED(on_msg_2);
		ENSURE_UNSUBSCRIBED(on_msg_3);
		ENSURE_UNSUBSCRIBED(on_msg_4);
		ENSURE_UNSUBSCRIBED(on_msg_5);

		ENSURE_UNSUBSCRIBED(on_umsg_1);
		ENSURE_UNSUBSCRIBED(on_umsg_2);
		ENSURE_UNSUBSCRIBED(on_umsg_3);
		ENSURE_UNSUBSCRIBED(on_umsg_4);
		ENSURE_UNSUBSCRIBED(on_umsg_5);
		ENSURE_UNSUBSCRIBED(on_umsg_6);

		ENSURE_UNSUBSCRIBED(on_ret_msg_1);
		ENSURE_UNSUBSCRIBED(on_ret_msg_2);
		ENSURE_UNSUBSCRIBED(on_ret_msg_3);
		ENSURE_UNSUBSCRIBED(on_ret_msg_4);
		ENSURE_UNSUBSCRIBED(on_ret_msg_5);

		ENSURE_UNSUBSCRIBED(on_ret_umsg_1);
		ENSURE_UNSUBSCRIBED(on_ret_umsg_2);
		ENSURE_UNSUBSCRIBED(on_ret_umsg_3);
		ENSURE_UNSUBSCRIBED(on_ret_umsg_4);
		ENSURE_UNSUBSCRIBED(on_ret_umsg_5);
		ENSURE_UNSUBSCRIBED(on_ret_umsg_6);

#undef ENSURE_UNSUBSCRIBED
	}

	virtual void
	so_evt_start() override
	{
		so_deregister_agent_coop_normally();
	}

private:
	void
	on_sig_1(const mhood_t<sig_1> &) const {}

	void
	on_sig_2(mhood_t<sig_2>) const {}

	void
	on_msg_1(const msg_1 &) const {}

	void
	on_msg_2(const mhood_t<msg_2> &) const {}

	void
	on_msg_3(mhood_t<msg_3>) const {}

	void
	on_msg_4(mutable_mhood_t<msg_4>) const {}

	void
	on_msg_5(const mutable_mhood_t<msg_5> &) const {}

	void
	on_umsg_1(const umsg_1 &) const {}

	void
	on_umsg_2(const mhood_t<umsg_2> &) const {}

	void
	on_umsg_3(mhood_t<umsg_3>) const {}

	void
	on_umsg_4(mutable_mhood_t<umsg_4>) const {}

	void
	on_umsg_5(const mutable_mhood_t<umsg_5> &) const {}

	void
	on_umsg_6(umsg_6) const {}

	int
	on_ret_msg_1(const ret_msg_1 &) const
	{
		return 0;
	}

	int
	on_ret_msg_2(const mhood_t<ret_msg_2> &) const
	{
		return 0;
	}

	int
	on_ret_msg_3(mhood_t<ret_msg_3>) const
	{
		return 0;
	}

	int
	on_ret_msg_4(mutable_mhood_t<ret_msg_4>) const
	{
		return 0;
	}

	int
	on_ret_msg_5(const mutable_mhood_t<ret_msg_5> &) const
	{
		return 0;
	}

	int
	on_ret_umsg_1(const ret_umsg_1 &) const
	{
		return 0;
	}

	int
	on_ret_umsg_2(const mhood_t<ret_umsg_2> &) const
	{
		return 0;
	}

	int
	on_ret_umsg_3(mhood_t<ret_umsg_3>) const
	{
		return 0;
	}

	int
	on_ret_umsg_4(mutable_mhood_t<ret_umsg_4>) const
	{
		return 0;
	}

	int
	on_ret_umsg_5(const mutable_mhood_t<ret_umsg_5> &) const
	{
		return 0;
	}

	int
	on_ret_umsg_6(ret_umsg_6) const
	{
		return 0;
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
			20,
			"so_has_subscription test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

