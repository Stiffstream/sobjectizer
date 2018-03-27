/*
 * A simple test for deadletter handlers.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include "../deadletter_handler_common.hpp"

class test_message final : public so_5::message_t {};

class test_signal final : public so_5::signal_t {};

template< typename Mbox_Case, typename Msg_Type >
class pfn_test_case_t final : public so_5::agent_t
{
	const Mbox_Case m_mbox_holder;

	state_t st_test{ this, "test" };

public:
	pfn_test_case_t( context_t ctx )
		:	so_5::agent_t( std::move(ctx) )
		,	m_mbox_holder( *self_ptr() )
	{}

	virtual void
	so_define_agent() override
	{
		this >>= st_test;

		so_subscribe_deadletter_handler( m_mbox_holder.mbox(),
				&pfn_test_case_t::on_deadletter );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send<Msg_Type>( m_mbox_holder.mbox() );
	}

private:
	void
	on_deadletter( mhood_t<Msg_Type> )
	{
		so_deregister_agent_coop_normally();
	}
};

template< typename Mbox_Case, typename Msg_Type >
class lambda_test_case_t final : public so_5::agent_t
{
	const Mbox_Case m_mbox_holder;

	state_t st_test{ this, "test" };

public:
	lambda_test_case_t( context_t ctx )
		:	so_5::agent_t( std::move(ctx) )
		,	m_mbox_holder( *self_ptr() )
	{}

	virtual void
	so_define_agent() override
	{
		this >>= st_test;

		so_subscribe_deadletter_handler( m_mbox_holder.mbox(),
				[this](mhood_t<Msg_Type>) {
					so_deregister_agent_coop_normally();
				} );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send<Msg_Type>( m_mbox_holder.mbox() );
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
				so_5::launch( [&]( so_5::environment_t & env ) {
						introduce_test_agent<
								direct_mbox_case_t,
								test_message,
								pfn_test_case_t >( env );
						introduce_test_agent<
								direct_mbox_case_t,
								so_5::mutable_msg<test_message>,
								pfn_test_case_t >( env );
						introduce_test_agent<
								direct_mbox_case_t,
								test_signal,
								pfn_test_case_t >( env );
						introduce_test_agent<
								mpmc_mbox_case_t,
								test_message,
								pfn_test_case_t >( env );
						introduce_test_agent<
								mpmc_mbox_case_t,
								test_signal,
								pfn_test_case_t >( env );

						introduce_test_agent<
								direct_mbox_case_t,
								test_message,
								lambda_test_case_t >( env );
						introduce_test_agent<
								direct_mbox_case_t,
								so_5::mutable_msg<test_message>,
								lambda_test_case_t >( env );
						introduce_test_agent<
								direct_mbox_case_t,
								test_signal,
								lambda_test_case_t >( env );
						introduce_test_agent<
								mpmc_mbox_case_t,
								test_message,
								lambda_test_case_t >( env );
						introduce_test_agent<
								mpmc_mbox_case_t,
								test_signal,
								lambda_test_case_t >( env );
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

