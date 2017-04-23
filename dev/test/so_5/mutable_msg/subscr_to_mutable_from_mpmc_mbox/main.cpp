/*
 * A test for subscription to mutable message from mpmc mbox.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;

template< typename L >
void
ensure_subscription_error(
	const std::string & test_case,
	L && lambda )
{
	try
	{
		lambda();

		ensure( false,
				test_case + ": an exception must be thrown before this point" );
	}
	catch( const so_5::exception_t & x )
	{
		ensure( so_5::rc_subscription_to_mutable_msg_from_mpmc_mbox ==
				x.error_code(),
				test_case +
				"an rc_subscription_to_mutable_msg_from_mpmc_mbox expected" );
	}
}

class sobj_message_tester final : public so_5::agent_t
{
	struct first final : public so_5::message_t {};

public :
	sobj_message_tester(context_t ctx)
		:	so_5::agent_t(std::move(ctx))
		,	m_mbox( so_environment().create_mbox() )
	{
	}

	virtual void
	so_evt_start() override
	{
		ensure_subscription_error(
			"sobj_message_tester(lambda-event-handler)",
			[this]{
				so_subscribe(m_mbox).event( []( mutable_mhood_t<first> ) {} );
			} );

		ensure_subscription_error(
			"sobj_message_tester(method-event-handler)",
			[this]{
				so_subscribe(m_mbox).event( &sobj_message_tester::on_first );
			} );

		so_deregister_agent_coop_normally();
	}

private :
	const so_5::mbox_t m_mbox;

	void on_first( mutable_mhood_t<first> ) {}
};

class user_message_tester final : public so_5::agent_t
{
	struct first final {};

public :
	user_message_tester(context_t ctx)
		:	so_5::agent_t(std::move(ctx))
		,	m_mbox( so_environment().create_mbox() )
	{
	}

	virtual void
	so_evt_start() override
	{
		ensure_subscription_error(
			"user_message_tester(lambda-event-handler)",
			[this]{
				so_subscribe(m_mbox).event( []( mutable_mhood_t<first> ) {} );
			} );

		ensure_subscription_error(
			"user_message_tester(method-event-handler)",
			[this]{
				so_subscribe(m_mbox).event( &user_message_tester::on_first );
			} );

		so_deregister_agent_coop_normally();
	}

private :
	const so_5::mbox_t m_mbox;

	void on_first( mutable_mhood_t<first> ) {}
};

void
check_adhoc_agent_subscription_sobj_message( so_5::environment_t & env )
{
	ensure_subscription_error( "adhoc_agent(sobj-message)",
		[&env] {
			struct demo final : public so_5::message_t {};
			auto mbox = env.create_mbox();

			env.introduce_coop( [&]( so_5::coop_t & coop ) {
				auto a = coop.define_agent();
				a.event( mbox, []( so_5::mutable_mhood_t<demo> ) {} );
			} );
		} );
}

void
check_adhoc_agent_subscription_user_message( so_5::environment_t & env )
{
	ensure_subscription_error( "adhoc_agent(user-message)",
		[&env] {
			struct demo final {};
			auto mbox = env.create_mbox();

			env.introduce_coop( [&]( so_5::coop_t & coop ) {
				auto a = coop.define_agent();
				a.event( mbox, []( so_5::mutable_mhood_t<demo> ) {} );
			} );
		} );
}
int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				so_5::launch(
					[&](so_5::environment_t & env) {
						env.register_agent_as_coop(so_5::autoname,
								env.make_agent<sobj_message_tester>());

						env.register_agent_as_coop(so_5::autoname,
								env.make_agent<user_message_tester>());
						
						check_adhoc_agent_subscription_sobj_message( env );
						check_adhoc_agent_subscription_user_message( env );
					},
					[](so_5::environment_params_t & params) {
						(void)params;
#if 0
						params.message_delivery_tracer( so_5::msg_tracing::std_cout_tracer() );
#endif
					} );
			},
			5,
			"simple agent");
	}
	catch(const exception & ex)
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}

