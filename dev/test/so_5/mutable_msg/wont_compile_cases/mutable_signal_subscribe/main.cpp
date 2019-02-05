/*
 * A test for attempt to send a mutable signal.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

using namespace std;

class demo final : public so_5::agent_t
{
	struct hello final : public so_5::signal_t {};

public :
	demo( context_t ctx ) : so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event( &demo::on_hello );
	}

private :
	void on_hello( mutable_mhood_t<hello> ) {}
};

int
main()
{
	return 0;
}

