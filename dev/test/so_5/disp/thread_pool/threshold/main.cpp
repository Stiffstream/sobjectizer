/*
 * Test for next_thread_wakeup_threshold param.
 */

#include <so_5/all.hpp>

#include <functional>
#include <sstream>
#include <thread>

#include <various_helpers_1/time_limited_execution.hpp>

using namespace std;

using namespace so_5;
using namespace so_5::disp::thread_pool;

using predicate_t = function< bool(thread::id, thread::id) >;

struct your_turn
	{
		thread::id m_id;
	};

class a_receiver_t final : public agent_t
	{
	public :
		a_receiver_t( context_t ctx, string case_name, predicate_t pred )
			:	agent_t{ ctx }
			,	m_case_name( move(case_name) )
			,	m_pred( move(pred) )
			{
				so_subscribe_self().event( &a_receiver_t::on_your_turn );
			}

	private :
		const string m_case_name;
		const predicate_t m_pred;

		void
		on_your_turn( const your_turn & msg )
			{
				if( !m_pred( this_thread::get_id(), msg.m_id ) )
					{
						ostringstream ss;
						ss << m_case_name << ": predicate failed, self id: "
								<< this_thread::get_id()
								<< ", foreign id: " << msg.m_id;
						throw runtime_error( ss.str() );
					}
				else
					so_deregister_agent_coop_normally();
			}
	};

class a_sender_t final : public agent_t
	{
		struct pause : public signal_t {};

	public :
		a_sender_t( context_t ctx, mbox_t receiver )
			:	agent_t{ ctx }, m_receiver{ receiver }
			{
				so_subscribe_self().event< pause >( [this] {
					send< your_turn >( m_receiver, this_thread::get_id() );
					this_thread::sleep_for( chrono::milliseconds(500) );
				} );
			}

		virtual void
		so_evt_start() override
			{
				send_delayed< pause >( *this, chrono::milliseconds(500) );
			}

	private :
		const mbox_t m_receiver;
	};

template< typename QUEUE_PARAMS_TUNER >
private_dispatcher_handle_t
make_disp(
	environment_t & env,
	std::string disp_name,
	QUEUE_PARAMS_TUNER qp_tuner )
	{
		return create_private_disp(
			env,
			disp_name,
			disp_params_t{}
				.thread_count( 2 )
				.tune_queue_params(
					[&qp_tuner]( queue_traits::queue_params_t & qp ) {
						qp_tuner( qp );
					} )
			);
	}

void
do_check(
	environment_t & env,
	private_dispatcher_handle_t disp,
	string case_name,
	predicate_t pred )
	{
		env.introduce_coop(
			disp->binder( bind_params_t{}.fifo( fifo_t::individual ) ),
			[case_name, pred]( coop_t & coop ) {
				auto receiver_mbox = coop.make_agent< a_receiver_t >(
						case_name, pred )->so_direct_mbox();
				coop.make_agent< a_sender_t >( receiver_mbox );
			} );
	}

void
check_threshold_default( environment_t & env )
	{
		do_check(
			env,
			make_disp(
				env,
				"disp_threshold_default",
				[]( queue_traits::queue_params_t & ) {} ),
			"default",
			[]( thread::id a, thread::id b ) {
				return a != b;
			} );
	}

void
check_threshold_0( environment_t & env )
	{
		do_check(
			env,
			make_disp(
				env,
				"disp_threshold_0",
				[]( queue_traits::queue_params_t & qp ) {
					qp.next_thread_wakeup_threshold( 0 );
				} ),
			"threshold_0",
			[]( thread::id a, thread::id b ) {
				return a != b;
			} );
	}

void
check_threshold_1( environment_t & env )
	{
		do_check(
			env,
			make_disp(
				env,
				"disp_threshold_1",
				[]( queue_traits::queue_params_t & qp ) {
					qp.next_thread_wakeup_threshold( 1 );
				} ),
			"threshold_1",
			[]( thread::id a, thread::id b ) {
				return a == b;
			} );
	}

void
do_test()
	{
		run_with_time_limit( [] {
				so_5::launch( []( environment_t & env ) {
					check_threshold_default( env );
					check_threshold_0( env );
					check_threshold_1( env );
				} );
			},
			5 );
	}


int
main()
	{
		try
			{
				do_test();
				return 0;
			}
		catch( const exception & ex )
			{
				cerr << "Error: " << ex.what() << endl;
			}

		return 2;
	}

