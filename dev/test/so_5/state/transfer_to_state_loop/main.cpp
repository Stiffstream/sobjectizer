/*
 * A very simple test case for checking transfer_to_state.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <so_5/h/stdcpp.hpp>

#include <various_helpers_1/ensure.hpp>
#include <various_helpers_1/time_limited_execution.hpp>

class special_exception_logger_t : public so_5::event_exception_logger_t
{
	so_5::optional<int> & m_storage;
	so_5::event_exception_logger_unique_ptr_t m_prev;

public:
	special_exception_logger_t(
		so_5::outliving_reference_t< so_5::optional<int> > storage)
		:	m_storage( storage.get() )
	{}

	void
	log_exception(
		const std::exception & ex,
		const std::string & coop_name ) override
	{
		if( m_prev )
			m_prev->log_exception( ex, coop_name );

		const auto p = dynamic_cast<const so_5::exception_t *>(&ex);
		if( p )
			m_storage = p->error_code();
	}

	void
	on_install(
		so_5::event_exception_logger_unique_ptr_t prev ) override
	{
		m_prev = std::move(prev);
	}
};

template<typename Agent_Type>
void
run_with_expected_error(
	int expected_error )
{
	so_5::optional<int> actual_error;

	run_with_time_limit(
		[&]()
		{
			so_5::launch( []( so_5::environment_t & env ) {
					env.introduce_coop( []( so_5::coop_t & coop ) {
							coop.make_agent< Agent_Type >();
						} );
				},
				[&]( so_5::environment_params_t & params ) {
					params.exception_reaction(
							so_5::exception_reaction_t::shutdown_sobjectizer_on_exception );

					params.event_exception_logger(
							so_5::stdcpp::make_unique< special_exception_logger_t >(
									so_5::outliving_mutable(actual_error) ) );

					params.message_delivery_tracer(
							so_5::msg_tracing::std_cout_tracer() );
				} );

			ensure_or_die( actual_error && expected_error == *actual_error,
					"transfer_to_state must fail with error: "
							+ std::to_string(expected_error) );
		},
		20 );
}

class a_simple_case_t final : public so_5::agent_t
{
	state_t st_base{ this, "base" };
	state_t st_disconnected{ initial_substate_of{st_base}, "disconnected" };
	state_t st_connected{ substate_of{st_base}, "connected" };

	struct message {};

public :
	a_simple_case_t(context_t ctx) : so_5::agent_t{ctx} {
		this >>= st_base;

		st_base.transfer_to_state<message>(st_disconnected);
	}

	virtual void so_evt_start() override {
		so_5::send<message>(*this);
	}
};

class a_tricky_loop_t final : public so_5::agent_t
{
	state_t st_base{ this, "base" };
	state_t st_first{ initial_substate_of{st_base}, "first" };

	struct message {};

public :
	a_tricky_loop_t(context_t ctx) : so_5::agent_t{ctx} {
		this >>= st_first;

		st_base.transfer_to_state<message>(st_base);
	}

	virtual void so_evt_start() override {
		so_5::send<message>(*this);
	}
};

class a_two_state_loop_t final : public so_5::agent_t
{
	state_t st_one{ this, "one" };
	state_t st_two{ this, "two" };

	struct message {};

public :
	a_two_state_loop_t(context_t ctx) : so_5::agent_t{ctx} {
		this >>= st_one;

		st_one.transfer_to_state<message>(st_two);
		st_two.transfer_to_state<message>(st_one);
	}

	virtual void so_evt_start() override {
		so_5::send<message>(*this);
	}
};

int
main()
{
	try
	{
		run_with_expected_error< a_simple_case_t >(
				so_5::rc_transfer_to_state_loop );

		run_with_expected_error< a_tricky_loop_t >(
				so_5::rc_transfer_to_state_loop );

		run_with_expected_error< a_two_state_loop_t >(
				so_5::rc_transfer_to_state_loop );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

