/*
 * A test for receive mutable message.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;

class sobj_message_tester final : public so_5::agent_t
{
	struct first final : public so_5::message_t {};
	struct second final : public so_5::message_t {};
	struct third final : public so_5::message_t {};
	struct fourth final : public so_5::message_t {};

public :
	sobj_message_tester(context_t ctx)
		:	so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event(&sobj_message_tester::on_first);
		so_subscribe_self().event(&sobj_message_tester::on_second);
		so_subscribe_self().event(&sobj_message_tester::on_third);
		so_subscribe_self().event(&sobj_message_tester::on_fourth);
	}

	virtual void
	so_evt_start() override
	{
		so_5::send<so_5::mutable_msg<first>>(*this);
		so_5::send<so_5::mutable_msg<second>>(*this);
		so_5::send<so_5::mutable_msg<third>>(*this);
		so_5::send<so_5::mutable_msg<fourth>>(*this);
	}

private :
	void
	on_first( const first & )
	{
		throw std::runtime_error(
				"sobj_message_tester::on_first must not be called!" );
	}

	void
	on_second( mhood_t<second> )
	{
		throw std::runtime_error(
				"sobj_message_tester::on_second must not be called!" );
	}

	void
	on_third( mhood_t<so_5::immutable_msg<third>> )
	{
		throw std::runtime_error(
				"sobj_message_tester::on_third must not be called!" );
	}

	void
	on_fourth( mhood_t<so_5::mutable_msg<fourth>> )
	{
		so_deregister_agent_coop_normally();
	}
};

class user_message_tester final : public so_5::agent_t
{
	struct first final {};
	struct second final {};
	struct third final {};
	struct fourth final {};

public :
	user_message_tester(context_t ctx)
		:	so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event(&user_message_tester::on_first);
		so_subscribe_self().event(&user_message_tester::on_second);
		so_subscribe_self().event(&user_message_tester::on_third);
		so_subscribe_self().event(&user_message_tester::on_fourth);
	}

	virtual void
	so_evt_start() override
	{
		so_5::send<so_5::mutable_msg<first>>(*this);
		so_5::send<so_5::mutable_msg<second>>(*this);
		so_5::send<so_5::mutable_msg<third>>(*this);
		so_5::send<so_5::mutable_msg<fourth>>(*this);
	}

private :
	void
	on_first( const first & )
	{
		throw std::runtime_error(
				"user_message_tester::on_first must not be called!" );
	}

	void
	on_second( mhood_t<second> )
	{
		throw std::runtime_error(
				"user_message_tester::on_second must not be called!" );
	}

	void
	on_third( mhood_t<so_5::immutable_msg<third>> )
	{
		throw std::runtime_error(
				"user_message_tester::on_third must not be called!" );
	}

	void
	on_fourth( mhood_t<so_5::mutable_msg<fourth>> )
	{
		so_deregister_agent_coop_normally();
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
						env.register_agent_as_coop(so_5::autoname,
								env.make_agent<sobj_message_tester>());

						env.register_agent_as_coop(so_5::autoname,
								env.make_agent<user_message_tester>());
					},
					[](so_5::environment_params_t & params) {
						(void)params;
						params.message_delivery_tracer( so_5::msg_tracing::std_cout_tracer() );
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

