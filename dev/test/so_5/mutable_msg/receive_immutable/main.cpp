/*
 * A test for receive immutable message.
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

public :
	sobj_message_tester(context_t ctx)
		:	so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event(&sobj_message_tester::on_first);
		so_subscribe_self().event(&sobj_message_tester::on_second);
		so_subscribe_self().event(&sobj_message_tester::on_third);
	}

	virtual void
	so_evt_start() override
	{
		so_5::send<so_5::immutable_msg<first>>(*this);
	}

private :
	void
	on_first( const first & )
	{
		std::cout << "so-first" << std::endl;
		so_5::send<second>( *this );
	}

	void
	on_second( mhood_t<second> )
	{
		std::cout << "so-second" << std::endl;
		so_5::send<third>( *this );
	}

	void
	on_third( mhood_t<so_5::immutable_msg<third>> )
	{
		std::cout << "so-third" << std::endl;
		so_deregister_agent_coop_normally();
	}
};

class user_message_tester final : public so_5::agent_t
{
	struct first final {};
	struct second final {};
	struct third final {};

public :
	user_message_tester(context_t ctx)
		:	so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event(&user_message_tester::on_first);
		so_subscribe_self().event(&user_message_tester::on_second);
		so_subscribe_self().event(&user_message_tester::on_third);
	}

	virtual void
	so_evt_start() override
	{
		so_5::send<so_5::immutable_msg<first>>(*this);
	}

private :
	void
	on_first( const first & )
	{
		std::cout << "u-first" << std::endl;
		so_5::send<second>( *this );
	}

	void
	on_second( mhood_t<second> )
	{
		std::cout << "u-second" << std::endl;
		so_5::send<third>( *this );
	}

	void
	on_third( mhood_t<so_5::immutable_msg<third>> )
	{
		std::cout << "u-third" << std::endl;
		so_deregister_agent_coop_normally();
	}
};

class signal_tester final : public so_5::agent_t
{
	struct first final : public so_5::signal_t {};
	struct second final : public so_5::signal_t {};

public :
	signal_tester(context_t ctx)
		:	so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event(&signal_tester::on_first);
		so_subscribe_self().event(&signal_tester::on_second);
	}

	virtual void
	so_evt_start() override
	{
		so_5::send<so_5::immutable_msg<first>>(*this);
	}

private :
	void
	on_first( mhood_t<first> )
	{
		std::cout << "s-first" << std::endl;
		so_5::send<second>( *this );
	}

	void
	on_second( mhood_t<so_5::immutable_msg<second>> )
	{
		std::cout << "s-second" << std::endl;
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

						env.register_agent_as_coop(so_5::autoname,
								env.make_agent<signal_tester>());
					});
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

