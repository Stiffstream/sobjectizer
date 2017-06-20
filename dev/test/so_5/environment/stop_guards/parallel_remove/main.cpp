/*
 * A test for parallel remove of stop_guards.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;

struct shutdown_started final : public so_5::signal_t {};

class second_stop_guard_t final
	: public so_5::stop_guard_t
	, public std::enable_shared_from_this<second_stop_guard_t>
{
public :
	second_stop_guard_t()
	{}

	virtual void
	stop() SO_5_NOEXCEPT override
	{
		std::this_thread::sleep_for( std::chrono::milliseconds(50) );
	}
};

const std::size_t N = 4;

class first_worker_t final : public so_5::agent_t
{
public :
	struct worker_started final : public so_5::signal_t {};

	first_worker_t( context_t ctx, so_5::mbox_t notify_mbox )
		:	so_5::agent_t( std::move(ctx) )
		,	m_notify_mbox( std::move(notify_mbox) )
	{
		so_subscribe_self()
			.event( &first_worker_t::on_worker_started );
	}

private :
	const so_5::mbox_t m_notify_mbox;
	std::size_t m_active_workers = 0;

	void
	on_worker_started( mhood_t< worker_started > )
	{
		++m_active_workers;
		if( m_active_workers >= N )
		{
			so_5::send< shutdown_started >( m_notify_mbox );
			so_environment().stop();
		}
	}
};

class second_worker_t final : public so_5::agent_t
{
public :
	second_worker_t(
		context_t ctx,
		so_5::mbox_t manager_mbox,
		so_5::mbox_t notify_mbox )
		:	so_5::agent_t( std::move(ctx) )
		,	m_manager_mbox( std::move(manager_mbox) )
	{
		so_subscribe( notify_mbox ).event( &second_worker_t::on_shutdown_started );
	}

	virtual void
	so_evt_start() override
	{
		m_guard = std::make_shared< second_stop_guard_t >();
		so_environment().setup_stop_guard( m_guard );

		so_5::send< first_worker_t::worker_started >( m_manager_mbox );
	}

private :
	const so_5::mbox_t m_manager_mbox;
	std::shared_ptr< second_stop_guard_t > m_guard;

	void
	on_shutdown_started( mhood_t<shutdown_started> )
	{
		so_environment().remove_stop_guard( m_guard );
	}
};

void make_stuff( so_5::environment_t & env )
{
	auto notify_mbox = env.create_mbox();

	env.introduce_coop(
		so_5::disp::active_obj::create_private_disp( env )->binder(),
		[&]( so_5::coop_t & coop ) {
			auto first = coop.make_agent< first_worker_t >( notify_mbox );
			for( std::size_t i = 0; i != N; ++i )
				coop.make_agent< second_worker_t >(
						first->so_direct_mbox(),
						notify_mbox );
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
			5 );
	}
	catch(const exception & ex)
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}

