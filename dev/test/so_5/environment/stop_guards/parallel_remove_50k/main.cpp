/*
 * A test for parallel remove of 5K stop_guards.
 *
 * NOTE: count of agents is reduced to 5K because it takes
 * to much time inside virtual machines.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

#include <random>

using namespace std;

int
random( int l, int r )
{
	std::default_random_engine engine;
	return std::uniform_int_distribution<int>( l, r )( engine );
}

struct shutdown_started final : public so_5::signal_t {};

class second_stop_guard_t final
	: public so_5::stop_guard_t
	, public std::enable_shared_from_this<second_stop_guard_t>
{
public :
	struct remove_me final : public so_5::signal_t {};

	second_stop_guard_t( so_5::mbox_t owner )
		:	m_owner( std::move(owner) )
	{}

	virtual void
	stop() SO_5_NOEXCEPT override
	{
		so_5::send< remove_me >( m_owner );
	}

private :
	const so_5::mbox_t m_owner;
};

const std::size_t N = 50000u;

class first_worker_t final : public so_5::agent_t
{
public :
	struct worker_started final : public so_5::signal_t {};

	first_worker_t( context_t ctx )
		:	so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self()
			.event( &first_worker_t::on_worker_started );
	}

	virtual void
	so_evt_finish() override
	{
		const auto finish_at = std::chrono::steady_clock::now();
		std::cout << "stop completed in: "
				<< std::chrono::duration_cast<
						std::chrono::milliseconds >( finish_at - m_started_at ).count()
				<< "ms" << std::endl;
	}

private :
	std::size_t m_active_workers = 0;
	std::chrono::steady_clock::time_point m_started_at;

	void
	on_worker_started( mhood_t< worker_started > )
	{
		++m_active_workers;
		if( m_active_workers >= N )
		{
			m_started_at = std::chrono::steady_clock::now();
			so_environment().stop();
		}
	}
};

class second_worker_t final : public so_5::agent_t
{
	struct do_init final : public so_5::signal_t {};

public :
	second_worker_t(
		context_t ctx,
		so_5::mbox_t manager_mbox )
		:	so_5::agent_t( std::move(ctx) )
		,	m_manager_mbox( std::move(manager_mbox) )
	{
		so_subscribe_self()
			.event( &second_worker_t::on_do_init )
			.event( &second_worker_t::on_remove_me );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send_delayed< do_init >( *this,
				std::chrono::milliseconds( random( 1, 50 ) ) );
	}

private :
	const so_5::mbox_t m_manager_mbox;
	std::shared_ptr< second_stop_guard_t > m_guard;

	void
	on_do_init( mhood_t<do_init> )
	{
		m_guard = std::make_shared< second_stop_guard_t >( so_direct_mbox() );
		so_environment().setup_stop_guard( m_guard );

		so_5::send< first_worker_t::worker_started >( m_manager_mbox );
	}

	void
	on_remove_me( mhood_t<second_stop_guard_t::remove_me> )
	{
		so_environment().remove_stop_guard( m_guard );
	}
};

void make_stuff( so_5::environment_t & env )
{
	namespace tpdisp = so_5::disp::thread_pool;

	auto notify_mbox = env.create_mbox();

	env.introduce_coop(
		tpdisp::create_private_disp( env )->binder(
				tpdisp::bind_params_t().fifo( tpdisp::fifo_t::individual ) ),
		[&]( so_5::coop_t & coop ) {
			auto first = coop.make_agent< first_worker_t >();
			for( std::size_t i = 0; i != N; ++i )
				coop.make_agent< second_worker_t >( first->so_direct_mbox() );
		} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				so_5::launch(
					[&](so_5::environment_t & env) {
						make_stuff( env );
					},
					[](so_5::environment_params_t & params) {
						(void)params;
					} );
			},
			30 );
	}
	catch(const exception & ex)
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}

