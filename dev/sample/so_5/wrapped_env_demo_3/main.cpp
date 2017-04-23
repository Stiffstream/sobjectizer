/*
 * An example of use wrapped_env with simple_mtsafe environment
 * and two working threads with mchains and delayed messages.
 */

#include <so_5/all.hpp>

// Messages for exchanges between threads.
struct tick final : public so_5::signal_t {};
struct tack final : public so_5::signal_t {};

// Special signal to stop the exchange.
struct stop final : public so_5::signal_t {};

// Helper class for removing code duplication in handling the state of one
// working thread.
class thread_state final
{
	std::chrono::milliseconds pause_{ 750 };
	bool must_stop_{ false };

public :
	bool must_stop() const { return must_stop_; }

	template<typename Reply>
	void reply_or_stop( const so_5::mchain_t & to )
	{
		if( pause_ > std::chrono::milliseconds(5) )
		{
			pause_ = std::chrono::milliseconds(
					static_cast< decltype(pause_.count()) >(pause_.count() / 1.5) );
			so_5::send_delayed< Reply >( to, pause_ );
		}
		else
		{
			so_5::send< stop >( to );
			must_stop_ = true;
		}
	}
};

// Body for thread.
void thread_body( so_5::mchain_t recv_chain, so_5::mchain_t write_chain )
{
	thread_state state;
	receive(
		from(recv_chain).stop_on( [&state]{ return state.must_stop(); } ),
		[&]( so_5::mhood_t<tick> ) {
			std::cout << "Tick!" << std::endl;
			state.reply_or_stop< tack >( write_chain );
		},
		[&]( so_5::mhood_t<tack> ) {
			std::cout << "Tack!" << std::endl;
			state.reply_or_stop< tick >( write_chain );
		} );
}

// Just a helper function for preparation of environment params.
// It is needed just for shortening of main() function code.
so_5::environment_params_t make_env_params()
{
	so_5::environment_params_t env_params;
	env_params.infrastructure_factory(
			so_5::env_infrastructures::simple_mtsafe::factory() );
	return env_params;
}

int main()
{
	so_5::wrapped_env_t sobj( make_env_params() );

	// An instance for second working thread.
	// The thread itself will be started later.
	std::thread second_thread;
	// This thread must be joined later.
	const auto thread_joiner = so_5::auto_join( second_thread );

	// We need two simple mchains for exchange.
	auto ch1 = create_mchain( sobj );
	auto ch2 = create_mchain( sobj );
	// The chains must be closed automatically.
	const auto ch_closer = so_5::auto_close_drop_content( ch1, ch2 );

	// Launch another thread.
	second_thread = std::thread( thread_body, ch2, ch1 );

	// The first message must be initiated.
	so_5::send< tick >( ch1 );

	// Launch the interchange on the context of main thread.
	thread_body( ch1, ch2 );

	return 0;
}

