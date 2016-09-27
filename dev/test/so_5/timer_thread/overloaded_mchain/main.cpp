/*
 * Test of sending delayed message into overloaded mchain.
 */

#include <iostream>
#include <stdexcept>
#include <string>

#include <so_5/all.hpp>

#include <various_helpers_1/ensure.hpp>
#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t final : public so_5::agent_t
	{
		struct sig_A : public so_5::signal_t {};
		struct sig_B : public so_5::signal_t {};
		struct sig_C : public so_5::signal_t {};

	public :
		a_test_t( context_t ctx )
			:	so_5::agent_t( ctx )
			,	m_mchain( create_mchain(
					ctx.environment(),
					std::chrono::seconds(1),
					1u,
					so_5::mchain_props::memory_usage_t::preallocated,
					so_5::mchain_props::overflow_reaction_t::drop_newest ) )
			{
				so_subscribe_self().event< sig_C >( &a_test_t::on_sig_C );
			}

		virtual void
		so_evt_start() override
			{
				so_5::send< sig_A >( m_mchain );
				so_5::send_delayed< sig_B >( m_mchain,
						std::chrono::milliseconds(250) );

				m_sent_at = std::chrono::steady_clock::now();
				so_5::send_delayed< sig_C >( *this,
						std::chrono::milliseconds(260) );
			}

	private :
		so_5::mchain_t m_mchain;

		std::chrono::steady_clock::time_point m_sent_at;

		void
		on_sig_C()
			{
				const auto now = std::chrono::steady_clock::now();
				ensure_or_die( now - m_sent_at < std::chrono::seconds(1),
						"time interval must be less than 1s" );

				so_deregister_agent_coop_normally();
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
							env.register_agent_as_coop( so_5::autoname,
								env.make_agent< a_test_t >() );
						} );
					},
					10,
					"check delayed message for overloaded mchain" );
			}
		catch( const std::exception & ex )
			{
				std::cerr << "Error: " << ex.what() << std::endl;
				return 1;
			}

		return 0;
	}

