/*
 * A test for checking layer initialization.
 */

#include <iostream>
#include <map>
#include <exception>

#include <so_5/all.hpp>

#include <utest_helper_1/h/helper.hpp>

//
// test_layer_t
//

class test_layer_t
	:
		public so_5::layer_t
{
	private:
		int m_op_seq_counter;

		enum operations_t
		{
			OP_START = 0,
			OP_SHUTDOWN = 1,
			OP_WAIT = 2
		};

		static int op_calls[3];

	public:
		test_layer_t()
			:
				m_op_seq_counter( 0 )
		{}

		virtual ~test_layer_t()
		{}

		virtual void
		start()
		{
			op_calls[ OP_START ] = m_op_seq_counter++;
		}

		virtual void
		shutdown()
		{
			op_calls[ OP_SHUTDOWN ] = m_op_seq_counter++;
		}

		virtual void
		wait()
		{
			op_calls[ OP_WAIT ] = m_op_seq_counter++;
		}

	static void
	check_calls();

};

int test_layer_t::op_calls[3] = { -1, -1, -1 };
void
test_layer_t::check_calls()
{
	UT_CHECK_EQ( op_calls[ OP_START ], OP_START );
	UT_CHECK_EQ( op_calls[ OP_SHUTDOWN ], OP_SHUTDOWN );
	UT_CHECK_EQ( op_calls[ OP_WAIT ], OP_WAIT );
}


void
init( so_5::environment_t & env )
{
	env.stop();
}

UT_UNIT_TEST( check_layer_lifecircle_op_calls )
{
	so_5::launch(
			&init,
			[]( so_5::environment_params_t & params )
			{
				params.add_layer( new test_layer_t );
			} );

	test_layer_t::check_calls();
}

int
main()
{
	UT_RUN_UNIT_TEST( check_layer_lifecircle_op_calls );

	return 0;
}
