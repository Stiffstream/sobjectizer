/*
 * A test for resending periodic signals via mhood.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

using namespace std;

class first_tester final : public so_5::agent_t
{
	struct hello final : public so_5::signal_t {};

public :
	first_tester(context_t ctx)
		:	so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event( &first_tester::on_hello );
	}

	virtual void
	so_evt_start() override
	{
		m_timer = so_5::send_periodic< so_5::immutable_msg<hello> >(
				*this,
				std::chrono::milliseconds(50),
				std::chrono::milliseconds::zero() );
	}

private :
	so_5::timer_id_t m_timer;

	int m_received{ 0 };

	void on_hello( mhood_t< hello > cmd )
	{
		++m_received;
		if( 1 == m_received )
		{
			m_timer = so_5::send_periodic(
					so_direct_mbox(),
					std::chrono::milliseconds(25),
					std::chrono::milliseconds::zero(),
					std::move(cmd) );
		}
		else
		{
			so_deregister_agent_coop_normally();
		}
	}
};

class second_tester final : public so_5::agent_t
{
	struct hello final : public so_5::signal_t {};

public :
	second_tester(context_t ctx)
		:	so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event( &second_tester::on_hello );
	}

	virtual void
	so_evt_start() override
	{
		m_timer = so_5::send_periodic< hello >(
				*this,
				std::chrono::milliseconds(50),
				std::chrono::milliseconds::zero() );
	}

private :
	so_5::timer_id_t m_timer;

	int m_received{ 0 };

	void on_hello( mhood_t< so_5::immutable_msg<hello> > cmd )
	{
		++m_received;
		if( 1 == m_received )
		{
			m_timer = so_5::send_periodic(
					so_direct_mbox(),
					std::chrono::milliseconds(25),
					std::chrono::milliseconds::zero(),
					std::move(cmd) );
		}
		else
		{
			so_deregister_agent_coop_normally();
		}
	}
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
						env.register_agent_as_coop(
								env.make_agent< first_tester >());

						env.register_agent_as_coop(
								env.make_agent< second_tester >());
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

