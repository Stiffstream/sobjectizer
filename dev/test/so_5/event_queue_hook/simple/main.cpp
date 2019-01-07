/*
 * Simple test for event_queue_hook.
 */

#include <so_5/all.hpp>

#include <so_5/h/stdcpp.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

class test_agent_t final : public so_5::agent_t
{
	struct hello final : public so_5::signal_t {};

public :
	test_agent_t( context_t ctx ) : so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self().event( [this]( mhood_t<hello> ) {
				so_deregister_agent_coop_normally();
			} );
	}

	void
	so_evt_start() override
	{
		so_5::send< hello >( *this );
	}
};

void
generate_agents(
	so_5::environment_t & env,
	std::size_t agent_count )
{
	using namespace so_5::disp::thread_pool;

	auto disp = create_private_disp( env );
	for( std::size_t i = 0u; i != agent_count; ++i )
	{
		env.register_agent_as_coop(
				so_5::autoname,
				env.make_agent< test_agent_t >(),
				disp->binder( bind_params_t{} ) );
	}
}

class test_event_queue_t final : public so_5::event_queue_t
{
	so_5::event_queue_t * m_actual;

public :
	static std::atomic< std::size_t > m_instances_created;
	static std::atomic< std::size_t > m_instances_destroyed;

	test_event_queue_t( so_5::event_queue_t * actual )
		:	m_actual( actual )
	{
		++m_instances_created;
	}

	~test_event_queue_t() override
	{
		++m_instances_destroyed;
	}

	void
	push( so_5::execution_demand_t demand ) override
	{
		m_actual->push( std::move(demand) );
	}
};

std::atomic< std::size_t > test_event_queue_t::m_instances_created( 0u );
std::atomic< std::size_t > test_event_queue_t::m_instances_destroyed( 0u );

class test_event_queue_hook_t final : public so_5::event_queue_hook_t
{
	std::atomic< std::size_t > m_created;
	std::atomic< std::size_t > m_destroyed;

public :
	test_event_queue_hook_t()
		:	m_created( 0u )
		,	m_destroyed( 0u )
	{}

	SO_5_NODISCARD
	so_5::event_queue_t *
	on_bind(
		so_5::agent_t * /*agent*/,
		so_5::event_queue_t * original_queue ) SO_5_NOEXCEPT override
	{
		++m_created;

		return new test_event_queue_t( original_queue );
	}

	void
	on_unbind(
		so_5::agent_t * /*agent*/,
		so_5::event_queue_t * queue ) SO_5_NOEXCEPT override
	{
		++m_destroyed;

		delete queue;
	}

	SO_5_NODISCARD
	std::size_t
	created() const SO_5_NOEXCEPT { return m_created.load(); }

	SO_5_NODISCARD
	std::size_t
	destroyed() const SO_5_NOEXCEPT { return m_destroyed.load(); }
};

void
do_test()
{
	std::size_t test_agents = 137;

	test_event_queue_hook_t hook;

	so_5::launch( [test_agents]( so_5::environment_t & env ) {
			generate_agents( env, test_agents );
		},
		[&]( so_5::environment_params_t & params ) {
			params.event_queue_hook(
					so_5::event_queue_hook_unique_ptr_t(
							&hook,
							&so_5::event_queue_hook_t::noop_deleter ) );
		} );

	const auto ensure_equal =
		[](const char * name, std::size_t expected, std::size_t actual) {
			std::ostringstream ss;
			ss << "check: " << name << "; expected=" << expected
				<< ", actual=" << actual;
			ensure_or_die( expected == actual, ss.str() );
		};

	// There is an additional agent created by SObjectizer itself.
	ensure_equal( "created", test_agents + 1, hook.created() );
	ensure_equal( "destroyed", test_agents + 1, hook.destroyed() );

	ensure_equal( "instances_created", test_agents + 1,
			test_event_queue_t::m_instances_created.load() );
	ensure_equal( "instances_destroyed", test_agents + 1,
			test_event_queue_t::m_instances_destroyed.load() );
}

int main()
{
	run_with_time_limit( do_test, 10 );
}

