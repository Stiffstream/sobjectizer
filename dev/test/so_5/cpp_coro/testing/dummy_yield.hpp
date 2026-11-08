#if !defined( TEST_SO_5_CPP_CORO_TESTING_DUMMY_YIELD_HPP )
#define TEST_SO_5_CPP_CORO_TESTING_DUMMY_YIELD_HPP

#include <so_5/all.hpp>
#include <so_5/cpp_coro/mchain_select.hpp>

namespace test
{

inline
auto
dummy_yield(
	so_5::environment_t & env,
	std::chrono::steady_clock::duration timeout )
	{
		struct msg_resume final : public so_5::signal_t {};

		auto ch = so_5::create_mchain(
				env,
				1u,
				so_5::mchain_props::memory_usage_t::preallocated,
				so_5::mchain_props::overflow_reaction_t::abort_app );

		so_5::send_delayed< msg_resume >( ch, timeout );

		return so_5::cpp_coro::select( so_5::from_all().handle_n(1),
				so_5::receive_case( ch,
					[]( so_5::mhood_t< msg_resume > ) {
std::cout << "msg_resume received" << std::endl;
					} ) );
	}

} /* namespace test */

#endif

