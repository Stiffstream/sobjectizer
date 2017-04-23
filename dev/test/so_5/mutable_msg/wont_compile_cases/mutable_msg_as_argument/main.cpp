/*
 * A test for attempt to use mutable_msg as argument type.
 */

#include <so_5/all.hpp>

using namespace std;

class demo final : public so_5::agent_t
{
	struct hello final : public so_5::message_t {};

public :
	demo( context_t ctx ) : so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event( &demo::on_hello );
	}

private :
	void on_hello( so_5::mutable_msg<hello> ) {}
};

int
main()
{
	return 0;
}

