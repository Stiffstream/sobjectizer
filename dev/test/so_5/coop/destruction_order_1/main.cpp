/*
 * A unit-test for testing order of destruction of binders and agents.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

namespace test
{

std::atomic<int> g_value{ 0 };

class custom_dispatcher_t final : public so_5::disp_binder_t
	{
		so_5::disp_binder_shptr_t m_actual_binder;

	public:
		custom_dispatcher_t( so_5::environment_t & env )
			:	m_actual_binder{ so_5::disp::one_thread::make_dispatcher(env).binder() }
			{}

		~custom_dispatcher_t() noexcept override
			{
				m_actual_binder.reset();
				g_value = 1;
			}

		void
		preallocate_resources(
			so_5::agent_t & agent ) override
			{
				m_actual_binder->preallocate_resources( agent );
			}

		void
		undo_preallocation(
			so_5::agent_t & agent ) noexcept override
			{
				m_actual_binder->undo_preallocation( agent );
			}

		void
		bind(
			so_5::agent_t & agent ) noexcept override
			{
				m_actual_binder->bind( agent );
			}

		void
		unbind(
			so_5::agent_t & agent ) noexcept override
			{
				m_actual_binder->unbind( agent );
			}
	};

class a_test_t final : public so_5::agent_t
	{
	public:
		using so_5::agent_t::agent_t;

		~a_test_t() noexcept override
			{
				const auto v = g_value.load( std::memory_order_acquire );
				if( 0 != v )
					{
						std::cerr << "Unexpected value of g_value: " << v << std::endl;
						std::abort();
					}
			}

		void
		so_evt_start() override
			{
				so_deregister_agent_coop_normally();
			}
	};

void
init( so_5::environment_t & env )
{
	env.introduce_coop(
			[]( so_5::coop_t & coop ) {
				coop.make_agent_with_binder< a_test_t >(
						std::make_shared< custom_dispatcher_t >( coop.environment() ) );
			} );
}

} /* namespace test */

using namespace test;

int
main()
{
	try
	{
		run_with_time_limit( []{ so_5::launch( init ); }, 20 );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}

