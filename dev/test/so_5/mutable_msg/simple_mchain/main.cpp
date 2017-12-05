/*
 * A test for mutable/immutable messages and mchain.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;

void
do_test( so_5::environment_t & env )
{
	struct sobj_hello final : public so_5::message_t
	{
		string m_msg;
		sobj_hello( string msg ) : m_msg( std::move(msg) ) {}
	};

	struct user_hello final
	{
		string m_msg;
		user_hello( string msg ) : m_msg( std::move(msg) ) {}
	};

	struct sig_hello final : public so_5::signal_t {};

	auto ch = create_mchain( env );

	so_5::send< sobj_hello >( ch, "sh1" );
	so_5::send< so_5::mutable_msg<sobj_hello> >( ch, "sh2" );
	so_5::send< so_5::immutable_msg<sobj_hello> >( ch, "sh3" );

	so_5::send< user_hello >( ch, "uh1" );
	so_5::send< so_5::immutable_msg<user_hello> >( ch, "uh2" );
	so_5::send< so_5::mutable_msg<user_hello> >( ch, "uh3" );

	so_5::send< sig_hello >( ch );
	so_5::send< so_5::immutable_msg<sig_hello> >( ch );

	so_5::send_delayed< so_5::mutable_msg<sobj_hello> >(
			ch, std::chrono::milliseconds(200), "shd" );
	so_5::send_delayed< so_5::mutable_msg<user_hello> >(
			ch, std::chrono::milliseconds(205), "uhd" );

	auto t1 = so_5::send_periodic< so_5::mutable_msg<sobj_hello> >(
			ch,
			std::chrono::milliseconds(210),
			std::chrono::milliseconds::zero(),
			"shp" );

	auto t2 = so_5::send_periodic< so_5::mutable_msg<user_hello> >(
			ch,
			std::chrono::milliseconds(215),
			std::chrono::milliseconds::zero(),
			"uhp" );

	const auto send_mutable_periodic_helper =
		[](so_5::mchain_t chain_to_send,
			std::chrono::milliseconds pause,
			std::chrono::milliseconds period,
			const char * str) {
			auto t = so_5::send_periodic< so_5::mutable_msg<sobj_hello> >(
					chain_to_send,
					pause,
					period,
					str );
		};

	UT_CHECK_THROW( so_5::exception_t,
		send_mutable_periodic_helper(
				ch,
				std::chrono::milliseconds(220),
				std::chrono::milliseconds(200),
				"shp2" ) );

	UT_CHECK_THROW( so_5::exception_t,
		send_mutable_periodic_helper(
				ch,
				std::chrono::milliseconds(225),
				std::chrono::milliseconds(200),
				"uhp2" ) );

	string collector;

	auto append_immutable = [&collector](const string & what) {
		collector += "imm(" + what + ");";
	};
	auto append_mutable = [&collector](const string & what) {
		collector += "mut(" + what + ");";
	};

	receive( from(ch).handle_n(12),
		[&]( so_5::mhood_t<sobj_hello> cmd ) {
			append_immutable( cmd->m_msg );
		},
		[&]( so_5::mhood_t< so_5::mutable_msg<sobj_hello> > cmd ) {
			append_mutable( cmd->m_msg );
		},
		[&]( so_5::mhood_t<user_hello> cmd ) {
			append_immutable( cmd->m_msg );
		},
		[&]( so_5::mhood_t< so_5::mutable_msg<user_hello> > cmd ) {
			append_mutable( cmd->m_msg );
		},
		[&]( so_5::mhood_t<sig_hello> /*cmd*/ ) {
			append_immutable( "sig" );
		} );

	UT_CHECK_EQ( collector,
			"imm(sh1);mut(sh2);imm(sh3);"
			"imm(uh1);imm(uh2);mut(uh3);"
			"imm(sig);imm(sig);"
			"mut(shd);mut(uhd);"
			"mut(shp);mut(uhp);"
		);
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				so_5::wrapped_env_t sobj;
				do_test( sobj.environment() );
			},
			5,
			"simple agent");
	}
	catch(const exception & ex)
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}

