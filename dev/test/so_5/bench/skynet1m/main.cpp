#include <iostream>
#include <chrono>

#include <so_5/all.hpp>

using namespace std;
using namespace so_5;

const unsigned int divider = 10;

using number = unsigned long long;

using disp_handle = disp::thread_pool::dispatcher_handle_t;

disp::thread_pool::bind_params_t bind_params()
{
	return disp::thread_pool::bind_params_t{}
		.fifo( disp::thread_pool::fifo_t::cooperation )
		.max_demands_at_once( divider * 3 )
	;
}

class skynet final : public agent_t
{
public :
	skynet( context_t ctx, disp_handle disp, mbox_t parent, number num, unsigned int size )
		:	agent_t{ ctx }
		,	m_disp{ std::move(disp) }
		,	m_parent{ std::move(parent) }
		,	m_num{ num }
		,	m_size{ size }
	{}

	virtual void so_evt_start() override
	{
		if( 1u == m_size )
			send< number >( m_parent, m_num );
		else
		{
			so_subscribe_self().event( &skynet::on_number );
			create_agents();
		}
	}

private :
	const disp_handle m_disp;
	const mbox_t m_parent;
	const number m_num;
	const unsigned int m_size;
	number m_sum = 0;
	unsigned int m_received = 0;
	so_5::coop_handle_t m_child;

	void on_number( number v )
	{ 
		m_sum += v;
		if( ++m_received == divider )
		{
			so_environment().deregister_coop( m_child, dereg_reason::normal );
			send< number >( m_parent, m_sum );
		}
	}

	void create_agents()
	{
		so_environment().introduce_coop(
			m_disp.binder( bind_params() ),
			[&]( coop_t & coop ) {
				m_child = coop.handle();

				const auto subsize = m_size / divider;
				coop.reserve( divider );
				for( unsigned int i = 0; i != divider; ++i )
					coop.make_agent< skynet >( m_disp, so_direct_mbox(), m_num + i * subsize, subsize );
			} );
	}
};

size_t pool_size()
{
	const auto c = thread::hardware_concurrency();
	return c > 1 ? c - 1 : 1;
}

int main()
{
	number result = 0;

	using clock_type = std::chrono::high_resolution_clock;
	const auto start_at = clock_type::now();

	so_5::launch( [&result]( environment_t & env ) {
		auto tp_disp = disp::thread_pool::make_dispatcher( env, pool_size() );

		auto result_ch = env.create_mchain( make_unlimited_mchain_params() );

		env.introduce_coop(
			tp_disp.binder(),
			[&]( coop_t & coop ) {
				coop.make_agent< skynet >(
						tp_disp, result_ch->as_mbox(), 0u, 1000000u );
			} );

		receive( from(result_ch).handle_n(1), [&result]( number v ) { result = v; } );


		env.stop();
	} );

	const auto finish_at = clock_type::now();

	std::cout << "result: " << result
		<< ", time: " << chrono::duration_cast< chrono::milliseconds >(
				finish_at - start_at ).count() << "ms" << std::endl;
}

