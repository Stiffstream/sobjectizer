/*
 * A test for trasformation of a message from a timer
 * with redirection to a full mchain.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct msg_with_limit final : public so_5::signal_t {};
struct msg_transformed_signal final : public so_5::signal_t {};
struct msg_control_delayed_msg final : public so_5::signal_t {};
struct msg_dummy final : public so_5::signal_t {};

class agent_with_limit_t final : public so_5::agent_t
{
public :
	agent_with_limit_t(
		context_t ctx,
		so_5::mchain_t redirect_ch )
		:	so_5::agent_t{ ctx
				+ limit_then_transform< msg_with_limit >( 1u,
						[redirect_ch]() {
							return make_transformed< msg_transformed_signal >(
									redirect_ch->as_mbox() );
						} )
			}
		,	m_redirect_ch{ std::move(redirect_ch) }
	{}

	virtual void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( []( mhood_t<msg_with_limit> ) { /* nothing to do */ } )
			;
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< msg_with_limit >( *this );
		so_5::send< msg_dummy >( m_redirect_ch );
		so_5::send_delayed< msg_with_limit >(
				*this, std::chrono::milliseconds{ 50 } );

		// Block the current thread and the current agent for some time.
		std::this_thread::sleep_for( std::chrono::milliseconds{ 500 } );
	}

private :
	const so_5::mchain_t m_redirect_ch;
};

class time_checker_t final : public so_5::agent_t
{
public:
	time_checker_t( context_t ctx )
		:	so_5::agent_t{ std::move(ctx) }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &time_checker_t::evt_delayed_msg )
			;
	}

	void
	so_evt_start() override
	{
		m_sent_at = std::chrono::steady_clock::now();
		so_5::send_delayed< msg_control_delayed_msg >(
				*this,
				std::chrono::milliseconds{ 100 } );
	}

private:
	std::chrono::steady_clock::time_point m_sent_at;

	void
	evt_delayed_msg( mhood_t< msg_control_delayed_msg > )
	{
		const auto received_at = std::chrono::steady_clock::now();
		const auto diff = (received_at - m_sent_at);
		std::cout << "msg_control_delayed_msg actual delay: "
				<< std::chrono::duration_cast< std::chrono::milliseconds >(diff).count()
				<< "ms" << std::endl;

		if( diff > std::chrono::milliseconds{ 300 } )
		{
			std::abort();
		}
		else
		{
			so_deregister_agent_coop_normally();
		}
	}
};

void
init( so_5::environment_t & env )
{
	env.introduce_coop(
			// Every agent should work on a separate worker thread.
			so_5::disp::active_obj::make_dispatcher( env ).binder(),
			[]( so_5::coop_t & coop ) {
				coop.make_agent< time_checker_t >();

				auto redirect_ch = so_5::create_mchain(
						coop.environment(),
						std::chrono::milliseconds{ 500 },
						1, // Very limited capacity.
						so_5::mchain_props::memory_usage_t::preallocated,
						so_5::mchain_props::overflow_reaction_t::drop_newest );
				coop.make_agent< agent_with_limit_t >( redirect_ch );
			} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init,
					[](so_5::environment_params_t & params) {
						params.message_delivery_tracer(
								so_5::msg_tracing::std_cout_tracer() );
					} );
			},
			5 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

