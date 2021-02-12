/*
 * A test for simple scenario with upcastable messages.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

class msg_base : public so_5::upcastable_message_root_t<msg_base>
{
public:
	msg_base() = default;
};

class derived_one : public so_5::upcastable_message_t<derived_one, msg_base>
{
public:
	derived_one() = default;
};

class derived_two : public so_5::upcastable_message_t<derived_two, msg_base>
{
public:
	derived_two() = default;
};

class derived_three : public so_5::upcastable_message_t<derived_three, msg_base>
{
public:
	derived_three() = default;
};

class a_test final : public so_5::agent_t
{
	std::string & m_trace;

public:
	struct finish final : public so_5::signal_t {};

	a_test(context_t ctx, std::string & trace)
		:	so_5::agent_t{ std::move(ctx) }
		,	m_trace{ trace }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_test::on_derived_one )
			.event( &a_test::on_derived_two )
			.event( &a_test::on_base )
			.event( &a_test::on_finish );
	}

	void
	so_evt_start() override
	{
		so_5::send<derived_one>( *this );
		so_5::send<derived_two>( *this );
		so_5::send<derived_three>( *this );
		so_5::send<finish>( *this );
	}

private:
	void
	on_derived_one( mhood_t<derived_one> )
	{
		m_trace += "derived_one;";
	}

	void
	on_derived_two( mhood_t<derived_two> )
	{
		m_trace += "derived_two;";
	}

	void
	on_base( mhood_t<msg_base> )
	{
		m_trace += "msg_base;";
	}

	void
	on_finish( mhood_t<finish> )
	{
		so_deregister_agent_coop_normally();
	}
};

int
main()
{
	run_with_time_limit(
		[]() {
			std::string trace;
			so_5::launch(
				[&](so_5::environment_t & env) {
					env.register_agent_as_coop(
							env.make_agent< a_test >( std::ref(trace) ) );
				},
				[](so_5::environment_params_t & params) {
					(void)params;
#if 0
					params.message_delivery_tracer( so_5::msg_tracing::std_cout_tracer() );
#endif
				} );

			ensure_or_die(
				trace == "derived_one;derived_two;msg_base;",
				"unexpected value of trace: " + trace );
		},
		5);

	return 0;
}

