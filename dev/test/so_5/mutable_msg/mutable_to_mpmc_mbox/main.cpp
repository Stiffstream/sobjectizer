/*
 * A test for sending mutable message to mpmc mbox.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;

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
		try
		{
			so_5::send< so_5::mutable_msg<first> >(m_mbox);

			ensure( false,
					"an exception must be thrown before this point" );
		}
		catch( const so_5::exception_t & x )
		{
			ensure( so_5::rc_mutable_msg_cannot_be_delivered_via_mpmc_mbox ==
					x.error_code(),
					"an rc_mutable_msg_cannot_be_delivered_via_mpmc_mbox expected" );
		}

		so_deregister_agent_coop_normally();
	}

private :
	const so_5::mbox_t m_mbox;
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
		try
		{
			so_5::send< so_5::mutable_msg<first> >(m_mbox);

			ensure( false,
					"an exception must be thrown before this point" );
		}
		catch( const so_5::exception_t & x )
		{
			ensure( so_5::rc_mutable_msg_cannot_be_delivered_via_mpmc_mbox ==
					x.error_code(),
					"an rc_mutable_msg_cannot_be_delivered_via_mpmc_mbox expected" );
		}

		so_deregister_agent_coop_normally();
	}

private :
	const so_5::mbox_t m_mbox;
};

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

